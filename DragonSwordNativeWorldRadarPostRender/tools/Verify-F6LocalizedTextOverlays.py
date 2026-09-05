#!/usr/bin/env python3
"""Verify the immutable Korean/Traditional-Chinese F6 raster payload."""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont


EXPECTED_FONT_SHA256 = (
    "05D71B179EF97B82CF1BB91CEF290C600A510F77F39B4964359E3EF88378C79D"
)
EXPECTED_FILES = {
    "ko-fault.tga",
    "ko-off.tga",
    "ko-on.tga",
    "language-popup.tga",
    "zh-hant-fault.tga",
    "zh-hant-off.tga",
    "zh-hant-on.tga",
}
EXPECTED_WIDTH = 1360
EXPECTED_HEIGHT = 1320
TGA_FOOTER = b"\x00" * 8 + b"TRUEVISION-XFILE.\x00"
MAX_CENTER_ERROR = 0.5
MIN_SLOT_INK_HEIGHT_RATIO = 0.28
MAX_SLOT_INK_HEIGHT_RATIO = 0.80

Slot = tuple[str, int, int, int, int]

EXPECTED_MAIN_LAYOUT = (
    ("title", 28, 17, 360, 34, 0.70, False),
    ("bug_report", 411, 17, 126, 26, 0.40, True),
    ("close", 559, 17, 94, 26, 0.43, True),
    ("language", 176, 76, 328, 17, 0.34, True),
    ("language_name", 200, 92, 280, 27, 0.50, True),
    ("status", 32, 142, 98, 24, 0.40, False),
    ("status_value", 158, 142, 154, 24, 0.40, True),
    ("status_action", 435, 142, 208, 24, 0.38, True),
    ("marker_visibility", 32, 187, 350, 22, 0.47, False),
    ("radar", 426, 187, 92, 20, 0.46, True),
    ("map", 556, 187, 90, 20, 0.46, True),
    ("category_0", 32, 212, 370, 26, 0.52, False),
    ("category_1", 32, 242, 370, 26, 0.52, False),
    ("category_2", 32, 272, 370, 26, 0.52, False),
    ("category_3", 32, 302, 370, 26, 0.52, False),
    ("category_4", 32, 332, 370, 26, 0.52, False),
    ("category_5", 32, 362, 370, 26, 0.52, False),
    ("category_6", 32, 392, 370, 26, 0.52, False),
    ("height_indicators", 32, 430, 350, 22, 0.47, False),
    ("radar_only", 524, 430, 122, 20, 0.40, True),
    ("height_category_0", 32, 456, 500, 26, 0.52, False),
    ("height_category_1", 32, 486, 500, 26, 0.52, False),
    ("height_category_2", 32, 516, 500, 26, 0.52, False),
    ("filter_modes", 32, 554, 350, 22, 0.47, False),
    ("mode_label_0", 32, 580, 280, 26, 0.48, False),
    ("available_0", 334, 583, 146, 20, 0.37, True),
    ("all_0", 496, 583, 146, 20, 0.37, True),
    ("mode_label_1", 32, 612, 280, 26, 0.48, False),
    ("available_1", 334, 615, 146, 20, 0.37, True),
    ("all_1", 496, 615, 146, 20, 0.37, True),
)

EXPECTED_POPUP_LAYOUT = (
    ("ko", 448, 145, 180, 24, 0.42, True),
    ("zh-hant", 246, 185, 180, 24, 0.42, True),
)

MAIN_OVERLAY_SLOTS: tuple[Slot, ...] = tuple(
    entry[:5] for entry in EXPECTED_MAIN_LAYOUT
)

POPUP_OVERLAY_SLOTS: tuple[Slot, ...] = tuple(
    entry[:5] for entry in EXPECTED_POPUP_LAYOUT
)

CENTERED_SLOT_NAMES = {
    entry[0]
    for entry in (*EXPECTED_MAIN_LAYOUT, *EXPECTED_POPUP_LAYOUT)
    if entry[6]
}

LANGUAGE_CODES = (
    "en", "ja", "ko", "zh-hans", "zh-hant", "fr",
    "de", "es-es", "ru", "th", "pt-br",
)
LANGUAGE_ENUM = (
    "English", "Japanese", "Korean", "SimplifiedChinese",
    "TraditionalChinese", "French", "German", "SpanishSpain",
    "Russian", "Thai", "PortugueseBrazil", "Count",
)


@dataclass(frozen=True)
class TgaRaster:
    width: int
    height: int
    alpha: bytearray


def fail(message: str) -> None:
    raise SystemExit(f"F6 localized overlay verification failed: {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def canonical_sha256(value: Any) -> str:
    encoded = json.dumps(
        value, ensure_ascii=False, separators=(",", ":")
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest().upper()


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        fail(f"cannot read {path}: {error}")
    if not isinstance(value, dict):
        fail(f"JSON root is not an object: {path}")
    return value


def expression_shape(value: ast.expr) -> str:
    return ast.dump(value, annotate_fields=True, include_attributes=False)


def expected_expression(source: str) -> str:
    return expression_shape(ast.parse(source, mode="eval").body)


def find_function(module: ast.Module, name: str) -> ast.FunctionDef:
    for node in module.body:
        if isinstance(node, ast.FunctionDef) and node.name == name:
            return node
    fail(f"overlay generator has no {name} function")


def find_assignment(function: ast.FunctionDef, name: str) -> ast.expr:
    for node in ast.walk(function):
        if (
            isinstance(node, ast.Assign)
            and len(node.targets) == 1
            and isinstance(node.targets[0], ast.Name)
            and node.targets[0].id == name
        ):
            return node.value
    fail(f"{function.name} has no {name} assignment")


def top_level_literals(module: ast.Module) -> dict[str, Any]:
    values: dict[str, Any] = {}
    for node in module.body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        if not isinstance(target, ast.Name):
            continue
        try:
            values[target.id] = ast.literal_eval(node.value)
        except (ValueError, TypeError):
            continue
    return values


def negative_number(value: ast.expr) -> float | None:
    try:
        literal = ast.literal_eval(value)
    except (ValueError, TypeError):
        return None
    if isinstance(literal, (int, float)) and not isinstance(literal, bool):
        return float(literal)
    return None


def localized_key(value: ast.expr) -> str | None:
    if (
        isinstance(value, ast.Subscript)
        and isinstance(value.value, ast.Name)
        and value.value.id == "text"
        and isinstance(value.slice, ast.Constant)
        and isinstance(value.slice.value, str)
    ):
        return value.slice.value
    return None


def draw_slot_calls(function: ast.FunctionDef) -> list[ast.Call]:
    calls = [
        node
        for node in ast.walk(function)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "draw_slot"
    ]
    return sorted(calls, key=lambda call: (call.lineno, call.col_offset))


def explicit_baseline(call: ast.Call) -> ast.expr | None:
    if len(call.args) >= 10:
        return call.args[9]
    for keyword in call.keywords:
        if keyword.arg == "baseline_offset":
            return keyword.value
    return None


def verify_generator_render_contract(path: Path) -> None:
    try:
        module = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    except (OSError, UnicodeError, SyntaxError) as error:
        fail(f"cannot parse overlay generator render contract: {error}")

    constants = top_level_literals(module)
    expected_constants = {
        "REFERENCE_WIDTH": 680,
        "REFERENCE_HEIGHT": 660,
        "SCALE": 2,
        "TEXT_COLOR": (247, 253, 255, 255),
        "SHADOW_COLOR": (8, 26, 38, 150),
        "STROKE_COLOR": (247, 253, 255, 128),
        "STROKE_WIDTH": 1,
    }
    for name, expected in expected_constants.items():
        if constants.get(name) != expected:
            fail(
                f"overlay generator {name} differs: "
                f"expected={expected!r} actual={constants.get(name)!r}"
            )

    target_font_size = find_function(module, "target_font_size")
    expected_assignments = {
        "rounded": "round(32.0 * role_scale)",
        "line_limit": "max(1, int(slot_height // 1.45))",
    }
    for name, expected in expected_assignments.items():
        actual = find_assignment(target_font_size, name)
        if expression_shape(actual) != expected_expression(expected):
            fail(f"target_font_size {name} no longer enforces {expected}")
    returns = [
        node.value
        for node in ast.walk(target_font_size)
        if isinstance(node, ast.Return) and node.value is not None
    ]
    if (
        len(returns) != 1
        or expression_shape(returns[0])
        != expected_expression("max(1, min(rounded, line_limit)) * SCALE")
    ):
        fail("target_font_size return no longer preserves the 32 px/clamped contract")

    fitted_font = find_function(module, "fitted_font")
    fitted_assignments = {
        "maximum_width": "max(1, round((width - 4.0) * SCALE))",
        "maximum_height": "max(1, round((height - 2.0) * SCALE))",
    }
    for name, expected in fitted_assignments.items():
        actual = find_assignment(fitted_font, name)
        if expression_shape(actual) != expected_expression(expected):
            fail(f"fitted_font {name} no longer enforces {expected}")
    getbbox_calls = [
        node
        for node in ast.walk(fitted_font)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "getbbox"
    ]
    if len(getbbox_calls) != 1:
        fail("fitted_font must contain exactly one glyph-bounds check")
    getbbox_keywords = {keyword.arg: keyword.value for keyword in getbbox_calls[0].keywords}
    if (
        "stroke_width" not in getbbox_keywords
        or expression_shape(getbbox_keywords["stroke_width"])
        != expected_expression("STROKE_WIDTH")
    ):
        fail("fitted_font bounds must include STROKE_WIDTH")

    draw_slot = find_function(module, "draw_slot")
    positional = [*draw_slot.args.posonlyargs, *draw_slot.args.args]
    default_names = [argument.arg for argument in positional[-len(draw_slot.args.defaults) :]]
    defaults = dict(zip(default_names, draw_slot.args.defaults))
    if (
        "baseline_offset" not in defaults
        or negative_number(defaults["baseline_offset"]) != -3.0
    ):
        fail("draw_slot default baseline_offset must remain -3.0")
    draw_assignments = {
        "px": "((x + width * 0.5) * SCALE if center else x * SCALE + 1.0)",
        "py": "(y + height * 0.5 + baseline_offset) * SCALE",
        "anchor": '("mm" if center else "lm")',
    }
    for name, expected in draw_assignments.items():
        actual = find_assignment(draw_slot, name)
        if expression_shape(actual) != expected_expression(expected):
            fail(f"draw_slot {name} no longer enforces {expected}")
    text_calls = sorted(
        (
            node
            for node in ast.walk(draw_slot)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and isinstance(node.func.value, ast.Name)
            and node.func.value.id == "draw"
            and node.func.attr == "text"
        ),
        key=lambda call: (call.lineno, call.col_offset),
    )
    if len(text_calls) != 2:
        fail("draw_slot must contain exactly one shadow and one foreground draw")
    shadow_keywords = {keyword.arg: keyword.value for keyword in text_calls[0].keywords}
    foreground_keywords = {
        keyword.arg: keyword.value for keyword in text_calls[1].keywords
    }
    if (
        "fill" not in shadow_keywords
        or expression_shape(shadow_keywords["fill"])
        != expected_expression("SHADOW_COLOR")
        or "stroke_width" in shadow_keywords
        or "stroke_fill" in shadow_keywords
    ):
        fail("draw_slot shadow must remain un-stroked SHADOW_COLOR")
    expected_foreground = {
        "fill": "TEXT_COLOR",
        "stroke_width": "STROKE_WIDTH",
        "stroke_fill": "STROKE_COLOR",
    }
    for name, expected in expected_foreground.items():
        if (
            name not in foreground_keywords
            or expression_shape(foreground_keywords[name])
            != expected_expression(expected)
        ):
            fail(f"draw_slot foreground {name} no longer uses {expected}")

    main_calls = draw_slot_calls(find_function(module, "build_main_overlay"))
    if len(main_calls) != 19:
        fail(f"main overlay draw-call topology differs: {len(main_calls)} != 19")
    explicit_main: dict[str, float] = {}
    for call in main_calls:
        baseline = explicit_baseline(call)
        if baseline is None:
            continue
        if len(call.args) < 3:
            fail("main overlay draw_slot call lacks its text value")
        key = localized_key(call.args[2])
        value = negative_number(baseline)
        if key is None or value is None or key in explicit_main:
            fail("main overlay has an unknown or duplicate explicit baseline")
        explicit_main[key] = value
    if explicit_main != {
        "close": -6.0,
        "language": -2.0,
        "language_name": -4.0,
    }:
        fail(f"main overlay explicit baselines differ: {explicit_main!r}")

    popup_calls = draw_slot_calls(find_function(module, "build_popup_overlay"))
    if len(popup_calls) != 1:
        fail(f"popup overlay draw-call topology differs: {len(popup_calls)} != 1")
    popup_baseline = explicit_baseline(popup_calls[0])
    if popup_baseline is None or negative_number(popup_baseline) != -4.0:
        fail("popup overlay baseline must remain -4.0")


def verify_current_generator_contract(path: Path) -> dict[str, Any]:
    try:
        source = path.read_text(encoding="utf-8")
        module = ast.parse(source, filename=str(path))
    except (OSError, UnicodeError, SyntaxError) as error:
        fail(f"cannot parse current overlay generator contract: {error}")
    constants = top_level_literals(module)
    expected_constants = {
        "REFERENCE_WIDTH": 680,
        "REFERENCE_HEIGHT": 660,
        "SCALE": 2,
        "TEXT_COLOR": (247, 253, 255, 255),
        "SHADOW_COLOR": (8, 26, 38, 150),
        "STROKE_COLOR": (247, 253, 255, 128),
        "STROKE_WIDTH": 1,
        "SHADOW_OFFSET": 1,
        "HORIZONTAL_PADDING": 2,
        "VERTICAL_PADDING": 1,
        "MIN_FIT_SCALE": 0.80,
        "FONT_SOURCE": "RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf",
        "EXPECTED_FONT_SHA256": EXPECTED_FONT_SHA256,
    }
    for name, expected in expected_constants.items():
        if constants.get(name) != expected:
            fail(
                f"overlay generator {name} differs: expected={expected!r} "
                f"actual={constants.get(name)!r}"
            )
    if constants.get("MAIN_LAYOUT") != EXPECTED_MAIN_LAYOUT:
        fail("generator main layout differs from the current hub geometry")
    if constants.get("POPUP_LAYOUT") != EXPECTED_POPUP_LAYOUT:
        fail("generator popup layout differs from the current hub geometry")
    localized = constants.get("LOCALIZED")
    if not isinstance(localized, dict) or set(localized) != {"ko", "zh-hant"}:
        fail("generator must rasterize exactly ko and zh-hant")

    parse_args_function = find_function(module, "parse_args")
    output_defaults = [
        keyword.value
        for node in ast.walk(parse_args_function)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "add_argument"
        and node.args
        and isinstance(node.args[0], ast.Constant)
        and node.args[0].value == "--output"
        for keyword in node.keywords
        if keyword.arg == "default"
    ]
    if (
        len(output_defaults) != 1
        or expression_shape(output_defaults[0])
        != expected_expression(
            'project_root / "assets" / "ui" / "f6"'
        )
    ):
        fail("generator output is not pinned to assets/ui/f6")

    draw_slot = find_function(module, "draw_slot")
    if "baseline_offset" in {argument.arg for argument in draw_slot.args.args}:
        fail("draw_slot must not apply a hand-tuned baseline offset")
    expected_assignments = {
        "px": "(slot_left + (slot_width - run.width) // 2 "
        "if center else slot_left + HORIZONTAL_PADDING * SCALE)",
        "py": "slot_top + (slot_height - run.height) // 2",
    }
    for name, expected in expected_assignments.items():
        if expression_shape(find_assignment(draw_slot, name)) != expected_expression(expected):
            fail(f"draw_slot {name} no longer centers a tight glyph run")
    composites = [
        node
        for node in ast.walk(draw_slot)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "alpha_composite"
    ]
    if len(composites) != 1:
        fail("draw_slot must alpha-composite exactly one tight glyph run")

    render_run = find_function(module, "render_glyph_run")
    alpha_bounds = [
        node
        for node in ast.walk(render_run)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "getbbox"
        and isinstance(node.func.value, ast.Call)
        and isinstance(node.func.value.func, ast.Attribute)
        and node.func.value.func.attr == "getchannel"
    ]
    crops = [
        node
        for node in ast.walk(render_run)
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Attribute)
        and node.func.attr == "crop"
    ]
    if len(alpha_bounds) != 1 or len(crops) != 1:
        fail("render_glyph_run must crop once to actual alpha bounds")
    return localized


def verify_tga(
    path: Path, expected_width: int, expected_height: int
) -> TgaRaster:
    data = path.read_bytes()
    if len(data) < 44:
        fail(f"TGA is too small: {path.name}")
    try:
        (
            id_length,
            color_map_type,
            image_type,
            color_map_first,
            color_map_length,
            color_map_depth,
            x_origin,
            y_origin,
            width,
            height,
            pixel_depth,
            descriptor,
        ) = struct.unpack_from("<BBBHHBHHHHBB", data, 0)
    except struct.error as error:
        fail(f"invalid TGA header in {path.name}: {error}")

    if (
        color_map_type != 0
        or image_type != 10
        or color_map_first != 0
        or color_map_length != 0
        or color_map_depth != 0
        or x_origin != 0
        or y_origin != 0
        or width != expected_width
        or height != expected_height
        or pixel_depth != 32
        or descriptor & 0x0F != 8
    ):
        fail(
            f"{path.name} is not the expected 32-bit RLE true-color TGA "
            f"({width}x{height}, type={image_type}, depth={pixel_depth})"
        )
    if data[-26:] != TGA_FOOTER:
        fail(f"{path.name} lacks the canonical TGA 2.0 footer")

    position = 18 + id_length
    payload_end = len(data) - 26
    expected_pixels = width * height
    decoded_pixels = 0
    transparent_pixels = 0
    visible_pixels = 0
    file_order_alpha = bytearray()
    while decoded_pixels < expected_pixels:
        if position >= payload_end:
            fail(f"{path.name} RLE stream ended before all pixels decoded")
        packet = data[position]
        position += 1
        count = (packet & 0x7F) + 1
        if decoded_pixels + count > expected_pixels:
            fail(f"{path.name} RLE packet exceeds the declared dimensions")
        if packet & 0x80:
            if position + 4 > payload_end:
                fail(f"{path.name} has a truncated RLE pixel")
            alpha = data[position + 3]
            position += 4
            file_order_alpha.extend(bytes((alpha,)) * count)
            if alpha == 0:
                transparent_pixels += count
            else:
                visible_pixels += count
        else:
            byte_count = count * 4
            if position + byte_count > payload_end:
                fail(f"{path.name} has a truncated raw packet")
            for alpha_offset in range(position + 3, position + byte_count, 4):
                alpha = data[alpha_offset]
                file_order_alpha.append(alpha)
                if alpha == 0:
                    transparent_pixels += 1
                else:
                    visible_pixels += 1
            position += byte_count
        decoded_pixels += count

    if position != payload_end:
        fail(f"{path.name} contains bytes after its decoded RLE stream")
    if len(file_order_alpha) != expected_pixels:
        fail(f"{path.name} decoded alpha size differs from its dimensions")
    if transparent_pixels == 0 or visible_pixels == 0:
        fail(f"{path.name} is not a mixed transparent text overlay")

    top_origin = bool(descriptor & 0x20)
    right_origin = bool(descriptor & 0x10)
    visual_alpha = bytearray(expected_pixels)
    for file_y in range(height):
        row_start = file_y * width
        row = file_order_alpha[row_start : row_start + width]
        if right_origin:
            row.reverse()
        visual_y = file_y if top_origin else height - 1 - file_y
        visual_start = visual_y * width
        visual_alpha[visual_start : visual_start + width] = row
    return TgaRaster(width, height, visual_alpha)


def verify_overlay_slots(
    path: Path, raster: TgaRaster, slots: tuple[Slot, ...]
) -> tuple[float, float]:
    allowed = bytearray(raster.width * raster.height)
    scaled_slots: list[tuple[str, int, int, int, int]] = []
    for name, x, y, width, height in slots:
        left = x * 2
        top = y * 2
        right = (x + width) * 2
        bottom = (y + height) * 2
        if (
            left < 0
            or top < 0
            or right > raster.width
            or bottom > raster.height
            or left >= right
            or top >= bottom
        ):
            fail(f"{path.name} slot {name} is outside the overlay canvas")
        scaled_slots.append((name, left, top, right, bottom))
        allowed_row = bytes((1,)) * (right - left)
        for visual_y in range(top, bottom):
            start = visual_y * raster.width + left
            allowed[start : start + right - left] = allowed_row

    for index, alpha in enumerate(raster.alpha):
        if alpha != 0 and allowed[index] == 0:
            x = index % raster.width
            y = index // raster.width
            fail(f"{path.name} has visible pixels outside every slot at ({x},{y})")

    maximum_center_error = 0.0
    minimum_ink_height_ratio = 1.0
    for name, left, top, right, bottom in scaled_slots:
        visible_x: list[int] = []
        visible_y: list[int] = []
        significant_edge_alpha = 0
        for y in range(top, bottom):
            row_start = y * raster.width
            for x in range(left, right):
                alpha = raster.alpha[row_start + x]
                if alpha == 0:
                    continue
                visible_x.append(x)
                visible_y.append(y)
                if x in (left, right - 1) or y in (top, bottom - 1):
                    significant_edge_alpha = max(significant_edge_alpha, alpha)
        if not visible_x:
            fail(f"{path.name} slot {name} has no visible glyph pixels")
        if significant_edge_alpha != 0:
            fail(
                f"{path.name} slot {name} has alpha "
                f"({significant_edge_alpha}) on its clipping edge"
            )
        min_x = min(visible_x)
        max_x = max(visible_x)
        min_y = min(visible_y)
        max_y = max(visible_y)
        vertical_error = abs(
            (min_y + max_y) / 2.0 - (top + bottom - 1) / 2.0
        )
        maximum_center_error = max(maximum_center_error, vertical_error)
        if vertical_error > MAX_CENTER_ERROR:
            fail(
                f"{path.name} slot {name} is not vertically centered: "
                f"error={vertical_error:.2f}px"
            )
        if name in CENTERED_SLOT_NAMES:
            horizontal_error = abs(
                (min_x + max_x) / 2.0 - (left + right - 1) / 2.0
            )
            maximum_center_error = max(maximum_center_error, horizontal_error)
            if horizontal_error > MAX_CENTER_ERROR:
                fail(
                    f"{path.name} slot {name} is not horizontally centered: "
                    f"error={horizontal_error:.2f}px"
                )
        elif min_x - left != 4:
            fail(
                f"{path.name} slot {name} left inset differs: "
                f"actual={min_x - left}px expected=4px"
            )
        ink_height = max(visible_y) - min(visible_y) + 1
        slot_height = bottom - top
        ink_height_ratio = ink_height / slot_height
        minimum_ink_height_ratio = min(
            minimum_ink_height_ratio, ink_height_ratio
        )
        if not (
            MIN_SLOT_INK_HEIGHT_RATIO
            <= ink_height_ratio
            <= MAX_SLOT_INK_HEIGHT_RATIO
        ):
            fail(
                f"{path.name} slot {name} ink-height ratio differs: "
                f"{ink_height_ratio:.3f} not in "
                f"[{MIN_SLOT_INK_HEIGHT_RATIO:.2f},"
                f"{MAX_SLOT_INK_HEIGHT_RATIO:.2f}]"
            )
    return maximum_center_error, minimum_ink_height_ratio


def load_generator_localizations(path: Path) -> dict[str, Any]:
    try:
        module = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    except (OSError, UnicodeError, SyntaxError) as error:
        fail(f"cannot parse overlay generator: {error}")
    for node in module.body:
        if isinstance(node, ast.Assign) and any(
            isinstance(target, ast.Name) and target.id == "LOCALIZED"
            for target in node.targets
        ):
            try:
                value = ast.literal_eval(node.value)
            except (ValueError, TypeError, SyntaxError) as error:
                fail(f"LOCALIZED is not a literal mapping: {error}")
            if not isinstance(value, dict):
                fail("LOCALIZED is not a mapping")
            return value
    fail("overlay generator has no LOCALIZED mapping")


def flatten_overlay_text(value: dict[str, Any]) -> list[str]:
    try:
        result = [
            value["language_name"],
            value["title"],
            value["language"],
            value["marker_visibility"],
            value["radar"],
            value["map"],
            *value["categories"],
            value["height_indicators"],
            value["radar_only"],
            *value["height_categories"],
            value["filter_modes"],
            value["available"],
            value["all"],
            value["close"],
            value["status"],
            value["status_values"]["off"],
            value["status_values"]["on"],
            value["status_values"]["fault"],
            value["status_actions"]["off"],
            value["status_actions"]["on"],
            value["status_actions"]["fault"],
            value["bug_report"],
        ]
    except (KeyError, TypeError) as error:
        fail(f"overlay localization shape is incomplete: {error}")
    if len(result) != 30 or not all(isinstance(item, str) and item for item in result):
        fail("overlay localization must contain exactly 30 non-empty strings")
    return result


def verify_localization_source(
    generator_path: Path, localization_header_path: Path
) -> tuple[str, int]:
    localized = load_generator_localizations(generator_path)
    if set(localized) != {"ko", "zh-hant"}:
        fail("generator must contain exactly ko and zh-hant raster localizations")
    try:
        header = localization_header_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read runtime localization header: {error}")
    literals = re.findall(r'L"([^"\\]*)"', header)
    if len(literals) != len(LANGUAGE_CODES) * 31:
        fail(
            "runtime localization table must contain exactly 11 complete "
            "31-string language blocks"
        )
    blocks = {
        language: literals[index * 31 : (index + 1) * 31]
        for index, language in enumerate(LANGUAGE_CODES)
    }
    if any(len(block) != 31 or not all(block) for block in blocks.values()):
        fail("runtime localization table contains an empty or truncated block")
    # The native header uses a 24 px reference font. At role scale 0.40 the
    # rounded em is 10 px. Counting every codepoint (including Thai combining
    # marks and spaces) as a full em is deliberately conservative.
    bug_report_widths = {
        language: len(block[30]) * round(24.0 * 0.40)
        for language, block in blocks.items()
    }
    widest_bug_report = max(bug_report_widths, key=bug_report_widths.get)
    if bug_report_widths[widest_bug_report] > 122:
        fail(
            "Bug Report text exceeds the conservative 122 px inner width: "
            f"{widest_bug_report}={bug_report_widths[widest_bug_report]}px"
        )
    for language in ("ko", "zh-hant"):
        expected = flatten_overlay_text(localized[language])
        runtime_block = blocks[language]
        # The fourth runtime string is the retained legacy Automatic label.
        # The explicit-language F6 raster deliberately does not draw it.
        runtime_overlay_text = runtime_block[:3] + runtime_block[4:]
        if runtime_overlay_text != expected:
            for index, (runtime_value, overlay_value) in enumerate(
                zip(runtime_overlay_text, expected)
            ):
                if runtime_value != overlay_value:
                    fail(
                        f"{language} text #{index} differs between runtime and "
                        f"overlay generator"
                    )
            fail(f"{language} runtime and overlay text counts differ")
    return widest_bug_report, bug_report_widths[widest_bug_report]


def normalize_source(value: str) -> str:
    return " ".join(value.split())


def require_source_snippet(source: str, snippet: str, label: str) -> None:
    if normalize_source(snippet) not in normalize_source(source):
        fail(f"runtime hub geometry/scope differs at {label}")


def verify_runtime_geometry_and_scope(
    preferences_path: Path, hub_path: Path
) -> None:
    try:
        preferences = preferences_path.read_text(encoding="utf-8")
        hub = hub_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read runtime geometry/scope source: {error}")
    enum_match = re.search(
        r"enum class RadarUiLanguage\s*:\s*std::uint8_t\s*\{([^}]*)\}",
        preferences,
        re.DOTALL,
    )
    if enum_match is None:
        fail("runtime RadarUiLanguage enum is missing")
    enum_values = tuple(
        value.strip()
        for value in enum_match.group(1).split(",")
        if value.strip()
    )
    if enum_values != LANGUAGE_ENUM:
        fail(f"runtime language order differs: {enum_values!r}")

    snippets = (
        (
            "LocalizedTextSlot::Title, localized.title, "
            "28.0, 17.0, 360.0, 34.0, 5, 0.70",
            "title",
        ),
        (
            "LocalizedTextSlot::BugReport, localized.bug_report, "
            "411.0, 17.0, 126.0, 26.0, 21, 0.40, kTextCenter",
            "Bug Report",
        ),
        (
            "LocalizedTextSlot::Close, localized.close, "
            "559.0, 17.0, 94.0, 26.0, 21, 0.43, kTextCenter",
            "Close",
        ),
        (
            "LocalizedTextSlot::Language, localized.language, "
            "176.0, 76.0, 328.0, 17.0, 6, 0.34, kTextCenter",
            "language label",
        ),
        (
            "LocalizedTextSlot::LanguageValue, language_display.data(), "
            "200.0, 92.0, 280.0, 27.0, 6, 0.50, kTextCenter",
            "language value",
        ),
        (
            "LocalizedTextSlot::StatusLabel, localized.status, "
            "32.0, 142.0, 98.0, 24.0, 7, 0.40",
            "status label",
        ),
        (
            "LocalizedTextSlot::StatusValue, status_value, "
            "158.0, 142.0, 154.0, 24.0, 7, 0.40, kTextCenter",
            "status value",
        ),
        (
            "LocalizedTextSlot::StatusAction, status_action, "
            "435.0, 142.0, 208.0, 24.0, 7, 0.38, kTextCenter",
            "status action",
        ),
        (
            "LocalizedTextSlot::MarkerVisibility, "
            "localized.marker_visibility, 32.0, 187.0, 350.0, 22.0, "
            "5, 0.47",
            "marker heading",
        ),
        (
            "LocalizedTextSlot::Radar, localized.radar, "
            "426.0, 187.0, 92.0, 20.0, 5, 0.46, kTextCenter",
            "Radar heading",
        ),
        (
            "LocalizedTextSlot::Map, localized.map, "
            "556.0, 187.0, 90.0, 20.0, 5, 0.46, kTextCenter",
            "Map heading",
        ),
        (
            "const double y = 212.0 + static_cast<double>(row) * 30.0",
            "marker rows",
        ),
        (
            "localized.marker_categories[category_index], "
            "32.0, y, 370.0, 26.0, 5, 0.52",
            "marker labels",
        ),
        (
            "LocalizedTextSlot::HeightIndicators, "
            "localized.height_indicators, 32.0, 430.0, 350.0, 22.0, "
            "5, 0.47",
            "height heading",
        ),
        (
            "LocalizedTextSlot::RadarOnly, localized.radar_only, "
            "524.0, 430.0, 122.0, 20.0, 5, 0.40, kTextCenter",
            "Radar Only",
        ),
        (
            "const double y = 456.0 + static_cast<double>(index) * 30.0",
            "height rows",
        ),
        (
            "localized.height_categories[index], "
            "32.0, y, 500.0, 26.0, 5, 0.52",
            "height labels",
        ),
        (
            "LocalizedTextSlot::FilterModes, localized.filter_modes, "
            "32.0, 554.0, 350.0, 22.0, 5, 0.47",
            "filter heading",
        ),
        (
            "const std::array<double, 2> mode_y{{580.0, 612.0}}",
            "filter row y",
        ),
        (
            "const std::array<double, 2> mode_x{{330.0, 492.0}}",
            "filter columns",
        ),
        (
            "const std::array<double, 2> mode_width{{154.0, 154.0}}",
            "filter widths",
        ),
        (
            "mode_x[0] + 4.0, y + 3.0, mode_width[0] - 8.0, "
            "20.0, 7, 0.37, kTextCenter",
            "Available option",
        ),
        (
            "mode_x[1] + 4.0, y + 3.0, mode_width[1] - 8.0, "
            "20.0, 7, 0.37, kTextCenter",
            "All option",
        ),
        (
            "const double x = 39.0 + "
            "static_cast<double>(column) * 202.0",
            "popup columns",
        ),
        (
            "const double y = 141.0 + static_cast<double>(row) * 40.0",
            "popup rows",
        ),
        (
            "choice_label, x + 5.0, y + 4.0, 180.0, 24.0, "
            "35, 0.42, kTextCenter",
            "popup labels",
        ),
        (
            "return language == dswros::RadarUiLanguage::Korean "
            "|| language == "
            "dswros::RadarUiLanguage::TraditionalChinese;",
            "two-language packaged-overlay scope",
        ),
        (
            "const bool packaged_choice = index == "
            "static_cast<std::size_t>( "
            "dswros::RadarUiLanguage::Korean) || index == "
            "static_cast<std::size_t>( "
            "dswros::RadarUiLanguage::TraditionalChinese);",
            "two-language popup-overlay scope",
        ),
    )
    for snippet, label in snippets:
        require_source_snippet(hub, snippet, label)


def overlay_values(text: dict[str, Any], status: str) -> dict[str, str]:
    result = {
        "title": text["title"],
        "bug_report": text["bug_report"],
        "close": text["close"],
        "language": text["language"],
        "language_name": text["language_name"],
        "status": text["status"],
        "status_value": text["status_values"][status],
        "status_action": text["status_actions"][status],
        "marker_visibility": text["marker_visibility"],
        "radar": text["radar"],
        "map": text["map"],
        "height_indicators": text["height_indicators"],
        "radar_only": text["radar_only"],
        "filter_modes": text["filter_modes"],
    }
    result.update(
        {f"category_{index}": value for index, value in enumerate(text["categories"])}
    )
    result.update(
        {
            f"height_category_{index}": value
            for index, value in enumerate(text["height_categories"])
        }
    )
    for index, category_index in enumerate((5, 3)):
        result[f"mode_label_{index}"] = text["categories"][category_index]
        result[f"available_{index}"] = text["available"]
        result[f"all_{index}"] = text["all"]
    return result


def glyph_signature(
    font: ImageFont.FreeTypeFont, value: str
) -> tuple[tuple[int, int], bytes]:
    mask = font.getmask(value)
    return mask.size, bytes(mask)


def tight_run_size(
    font: ImageFont.FreeTypeFont, value: str
) -> tuple[int, int]:
    left, top, right, bottom = font.getbbox(value, stroke_width=1)
    margin = 4
    image = Image.new(
        "RGBA",
        (max(1, right - left + 9), max(1, bottom - top + 9)),
        (0, 0, 0, 0),
    )
    draw = ImageDraw.Draw(image)
    origin = (margin - left, margin - top)
    draw.text(
        (origin[0] + 1, origin[1] + 1),
        value,
        font=font,
        fill=(8, 26, 38, 150),
    )
    draw.text(
        origin,
        value,
        font=font,
        fill=(247, 253, 255, 255),
        stroke_width=1,
        stroke_fill=(247, 253, 255, 128),
    )
    bounds = image.getchannel("A").getbbox()
    if bounds is None:
        fail(f"font rendered no pixels for {value!r}")
    return bounds[2] - bounds[0], bounds[3] - bounds[1]


def verify_font_glyphs_and_fit(
    font_path: Path, localized: dict[str, Any]
) -> tuple[int, float]:
    if not font_path.is_file() or sha256(font_path) != EXPECTED_FONT_SHA256:
        fail("DroidSansFallback build input is missing or differs from its pin")
    probe = ImageFont.truetype(str(font_path), 64)
    missing_signature = glyph_signature(probe, "\U0010FFFF")
    characters = sorted(
        {
            character
            for text in localized.values()
            for value in flatten_overlay_text(text)
            for character in value
            if not character.isspace()
        }
    )
    missing = [
        character
        for character in characters
        if glyph_signature(probe, character) == missing_signature
    ]
    if missing:
        fail(
            "pinned font lacks required overlay glyphs: "
            + ", ".join(f"U+{ord(value):04X}" for value in missing)
        )

    runs: list[tuple[str, int, int, float, str]] = []
    for language, text in localized.items():
        for status in ("off", "on", "fault"):
            values = overlay_values(text, status)
            for name, _, _, width, height, role_scale, _ in EXPECTED_MAIN_LAYOUT:
                runs.append(
                    (
                        values[name], width, height, role_scale,
                        f"{language}/{status}/{name}",
                    )
                )
    for language, _, _, width, height, role_scale, _ in EXPECTED_POPUP_LAYOUT:
        runs.append(
            (
                localized[language]["language_name"],
                width,
                height,
                role_scale,
                f"popup/{language}",
            )
        )

    minimum_fit = 1.0
    for value, width, height, role_scale, label in runs:
        rounded = round(32.0 * role_scale)
        line_limit = max(1, int(height // 1.45))
        target_size = max(1, min(rounded, line_limit)) * 2
        maximum_width = max(1, round((width - 4.0) * 2))
        maximum_height = max(1, round((height - 2.0) * 2))
        fitted_size = 0
        for size in range(target_size, 1, -1):
            run_width, run_height = tight_run_size(
                ImageFont.truetype(str(font_path), size), value
            )
            if run_width <= maximum_width and run_height <= maximum_height:
                fitted_size = size
                break
        if fitted_size == 0:
            fail(f"{label} cannot fit its slot")
        fit = fitted_size / target_size
        minimum_fit = min(minimum_fit, fit)
        if fit < 0.80:
            fail(f"{label} would be excessively compressed: {fit:.3f}")
    return len(characters), minimum_fit


def parse_args() -> argparse.Namespace:
    default_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, default=default_root)
    return parser.parse_args()


def main() -> int:
    root = parse_args().project_root.resolve()
    overlay_root = root / "assets" / "ui" / "f6"
    manifest_path = overlay_root / "manifest.json"
    font_path = (
        root
        / ".sdk"
        / "RE-UE4SS"
        / "deps"
        / "fonts"
        / "droid"
        / "DroidSansFallback.ttf"
    )
    generator_path = root / "tools" / "Build-F6LocalizedTextOverlays.py"
    localization_header = root / "include" / "dswros" / "radar_localization.hpp"

    localized = verify_current_generator_contract(generator_path)
    bug_report_language, bug_report_width = verify_localization_source(
        generator_path, localization_header
    )
    verify_runtime_geometry_and_scope(
        root / "include" / "dswros" / "radar_preferences.hpp",
        root / "src" / "native" / "radar_visibility_hub.cpp",
    )
    glyph_count, minimum_fit = verify_font_glyphs_and_fit(
        font_path, localized
    )
    if not overlay_root.is_dir():
        fail(f"overlay directory is missing: {overlay_root}")
    actual_entries = {path.name for path in overlay_root.iterdir() if path.is_file()}
    expected_entries = EXPECTED_FILES | {"manifest.json"}
    if actual_entries != expected_entries:
        fail(
            "overlay file set differs: "
            f"missing={sorted(expected_entries - actual_entries)} "
            f"unexpected={sorted(actual_entries - expected_entries)}"
        )
    if any(path.is_dir() or path.is_symlink() for path in overlay_root.iterdir()):
        fail("overlay directory contains a directory or symbolic link")

    manifest = read_json(manifest_path)
    if (
        manifest.get("schema_version") != 2
        or manifest.get("reference_size") != [680, 660]
        or manifest.get("raster_scale") != 2
        or manifest.get("generator_sha256") != sha256(generator_path)
        or manifest.get("layout_sha256")
        != canonical_sha256(
            {"main": EXPECTED_MAIN_LAYOUT, "popup": EXPECTED_POPUP_LAYOUT}
        )
        or manifest.get("localizations_sha256")
        != canonical_sha256(localized)
        or manifest.get("font_source")
        != "RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf"
        or manifest.get("font_sha256") != EXPECTED_FONT_SHA256
        or manifest.get("verified_codepoint_count") != glyph_count
    ):
        fail(
            "manifest identity, geometry, localization, or font pin is invalid"
        )

    entries = manifest.get("files")
    if not isinstance(entries, list) or len(entries) != len(EXPECTED_FILES):
        fail("manifest must contain exactly seven overlay entries")
    seen: set[str] = set()
    maximum_center_error = 0.0
    minimum_ink_height_ratio = 1.0
    for entry in entries:
        if not isinstance(entry, dict):
            fail("manifest overlay entry is not an object")
        name = entry.get("path")
        if not isinstance(name, str) or name not in EXPECTED_FILES or name in seen:
            fail(f"manifest contains an unsafe, unknown, or duplicate path: {name!r}")
        seen.add(name)
        if entry.get("width") != EXPECTED_WIDTH or entry.get("height") != EXPECTED_HEIGHT:
            fail(f"manifest geometry differs for {name}")
        path = overlay_root / name
        if entry.get("sha256") != sha256(path):
            fail(f"manifest SHA-256 differs for {name}")
        raster = verify_tga(path, EXPECTED_WIDTH, EXPECTED_HEIGHT)
        slots = (
            POPUP_OVERLAY_SLOTS
            if name == "language-popup.tga"
            else MAIN_OVERLAY_SLOTS
        )
        center_error, ink_height_ratio = verify_overlay_slots(
            path, raster, slots
        )
        maximum_center_error = max(maximum_center_error, center_error)
        minimum_ink_height_ratio = min(
            minimum_ink_height_ratio, ink_height_ratio
        )
    if seen != EXPECTED_FILES:
        fail(f"manifest overlay set differs: {sorted(EXPECTED_FILES - seen)}")

    print(
        "F6 localized overlay verification passed: all 11 runtime language "
        "blocks audited; ko/zh-Hant 30-slot and popup geometry exact; "
        f"{glyph_count} required codepoints; minimum fit {minimum_fit:.3f}; "
        f"widest 11-language Bug Report bound "
        f"{bug_report_language}={bug_report_width}/122px; "
        f"maximum optical-center error {maximum_center_error:.1f}px; "
        f"minimum ink-height ratio {minimum_ink_height_ratio:.3f}; "
        "7 unclipped RLE TGAs, manifest, font, and source pins exact."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

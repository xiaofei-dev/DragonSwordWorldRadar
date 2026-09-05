#!/usr/bin/env python3
"""Build deterministic 2x transparent F6 text overlays.

The game's optional DS_HYFont_P replacement maps Common/TC/Pretendard/Noto
assets to one Simplified-Chinese-only font.  Korean and some Traditional
Chinese strings therefore cannot be rendered by any UFont selected from that
package.  These tiny, fixed-label overlays are generated from the Apache-2.0
DroidSansFallback build input already pinned by RE-UE4SS.  Each glyph run is
cropped to its actual alpha bounds and composited into its UMG slot, avoiding
font-baseline heuristics and proving optical centering from the emitted pixels.
Runtime interaction, buttons, layout, and scaling remain owned by the native
UMG panel.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont


REFERENCE_WIDTH = 680
REFERENCE_HEIGHT = 660
SCALE = 2
TEXT_COLOR = (247, 253, 255, 255)
SHADOW_COLOR = (8, 26, 38, 150)
STROKE_COLOR = (247, 253, 255, 128)
STROKE_WIDTH = 1
SHADOW_OFFSET = 1
HORIZONTAL_PADDING = 2
VERTICAL_PADDING = 1
MIN_FIT_SCALE = 0.80
FONT_SOURCE = "RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf"
EXPECTED_FONT_SHA256 = (
    "05D71B179EF97B82CF1BB91CEF290C600A510F77F39B4964359E3EF88378C79D"
)


LOCALIZED = {
    "ko": {
        "language_name": "한국어",
        "title": "레이더 설정",
        "language": "언어",
        "marker_visibility": "마커 표시",
        "radar": "레이더",
        "map": "지도",
        "categories": [
            "시계", "보물", "보스", "돌발 임무", "미니게임",
            "지역 퀘스트", "새알",
        ],
        "height_indicators": "높이 표시",
        "radar_only": "레이더 전용",
        "height_categories": ["보물", "지역 퀘스트", "두더지 게임"],
        "filter_modes": "필터 모드",
        "available": "이용 가능",
        "all": "전체",
        "close": "닫기",
        "status": "상태",
        "status_values": {"off": "꺼짐", "on": "켜짐", "fault": "오류"},
        "status_actions": {"off": "켜기", "on": "끄기", "fault": "재시도"},
        "bug_report": "버그 신고",
    },
    "zh-hant": {
        "language_name": "繁體中文",
        "title": "雷達設定",
        "language": "語言",
        "marker_visibility": "標記顯示",
        "radar": "雷達",
        "map": "地圖",
        "categories": [
            "時鐘", "寶箱", "首領", "突發任務", "小遊戲", "區域任務", "鳥蛋",
        ],
        "height_indicators": "高度指示器",
        "radar_only": "僅雷達",
        "height_categories": ["寶箱", "區域任務", "土撥鼠遊戲"],
        "filter_modes": "篩選模式",
        "available": "可用",
        "all": "全部",
        "close": "關閉",
        "status": "模組狀態",
        "status_values": {"off": "未啟用", "on": "已啟用", "fault": "故障"},
        "status_actions": {"off": "啟用", "on": "停用", "fault": "重試"},
        "bug_report": "問題回報",
    },
}


# name, x, y, width, height, native role scale, horizontally centered
# Coordinates are the 680x660 reference-space TextBlock rectangles in
# radar_visibility_hub.cpp.  The raster itself is emitted at SCALE=2.
MAIN_LAYOUT = (
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

# language, x, y, width, height, native role scale, horizontally centered
POPUP_LAYOUT = (
    ("ko", 448, 145, 180, 24, 0.42, True),
    ("zh-hant", 246, 185, 180, 24, 0.42, True),
)


def parse_args() -> argparse.Namespace:
    project_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--font",
        type=Path,
        default=project_root
        / ".sdk"
        / "RE-UE4SS"
        / "deps"
        / "fonts"
        / "droid"
        / "DroidSansFallback.ttf",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=project_root / "assets" / "ui" / "f6",
    )
    return parser.parse_args()


def target_font_size(role_scale: float, slot_height: float) -> int:
    # DroidSansFallback is a regular face whose visible glyph body is roughly
    # three quarters of the game's bold UI face.  A 32 px reference base
    # matches the live UMG text metrics while the existing role scales retain
    # the intended title/body hierarchy.
    rounded = round(32.0 * role_scale)
    line_limit = max(1, int(slot_height // 1.45))
    return max(1, min(rounded, line_limit)) * SCALE


def render_glyph_run(
    font: ImageFont.FreeTypeFont,
    value: str,
) -> Image.Image:
    left, top, right, bottom = font.getbbox(
        value, stroke_width=STROKE_WIDTH
    )
    margin = STROKE_WIDTH + SHADOW_OFFSET + 2
    image = Image.new(
        "RGBA",
        (
            max(1, right - left + margin * 2 + SHADOW_OFFSET),
            max(1, bottom - top + margin * 2 + SHADOW_OFFSET),
        ),
        (0, 0, 0, 0),
    )
    draw = ImageDraw.Draw(image)
    origin = (margin - left, margin - top)
    draw.text(
        (origin[0] + SHADOW_OFFSET, origin[1] + SHADOW_OFFSET),
        value,
        font=font,
        fill=SHADOW_COLOR,
    )
    draw.text(
        origin,
        value,
        font=font,
        fill=TEXT_COLOR,
        stroke_width=STROKE_WIDTH,
        stroke_fill=STROKE_COLOR,
    )
    bounds = image.getchannel("A").getbbox()
    if bounds is None:
        raise ValueError(f"text produced no visible pixels: {value!r}")
    return image.crop(bounds)


def fitted_run(
    font_path: Path,
    value: str,
    width: float,
    height: float,
    role_scale: float,
) -> tuple[Image.Image, int, int]:
    target_size = target_font_size(role_scale, height)
    size = target_size
    maximum_width = max(
        1, round((width - HORIZONTAL_PADDING * 2.0) * SCALE)
    )
    maximum_height = max(
        1, round((height - VERTICAL_PADDING * 2.0) * SCALE)
    )
    while size > 1:
        font = ImageFont.truetype(str(font_path), size)
        run = render_glyph_run(font, value)
        if run.width <= maximum_width and run.height <= maximum_height:
            if size / target_size < MIN_FIT_SCALE:
                raise ValueError(
                    f"text requires excessive font compression: {value!r} "
                    f"({size}/{target_size})"
                )
            return run, size, target_size
        size -= 1
    raise ValueError(f"text cannot fit its slot: {value!r}")


def draw_slot(
    image: Image.Image,
    font_path: Path,
    value: str,
    x: float,
    y: float,
    width: float,
    height: float,
    role_scale: float,
    center: bool = False,
) -> None:
    run, _, _ = fitted_run(font_path, value, width, height, role_scale)
    slot_left = round(x * SCALE)
    slot_top = round(y * SCALE)
    slot_width = round(width * SCALE)
    slot_height = round(height * SCALE)
    px = (
        slot_left + (slot_width - run.width) // 2
        if center
        else slot_left + HORIZONTAL_PADDING * SCALE
    )
    py = slot_top + (slot_height - run.height) // 2
    if (
        px < slot_left
        or py < slot_top
        or px + run.width > slot_left + slot_width
        or py + run.height > slot_top + slot_height
    ):
        raise ValueError(f"rendered text escaped its slot: {value!r}")
    image.alpha_composite(run, (px, py))


def new_canvas() -> Image.Image:
    return Image.new(
        "RGBA", (REFERENCE_WIDTH * SCALE, REFERENCE_HEIGHT * SCALE),
        (0, 0, 0, 0),
    )


def main_text_values(text: dict[str, object], status: str) -> dict[str, str]:
    categories = text["categories"]
    height_categories = text["height_categories"]
    status_values = text["status_values"]
    status_actions = text["status_actions"]
    if (
        not isinstance(categories, list)
        or not isinstance(height_categories, list)
        or not isinstance(status_values, dict)
        or not isinstance(status_actions, dict)
    ):
        raise ValueError("localized text has an invalid collection shape")
    values = {
        "title": text["title"],
        "bug_report": text["bug_report"],
        "close": text["close"],
        "language": text["language"],
        "language_name": text["language_name"],
        "status": text["status"],
        "status_value": status_values[status],
        "status_action": status_actions[status],
        "marker_visibility": text["marker_visibility"],
        "radar": text["radar"],
        "map": text["map"],
        "height_indicators": text["height_indicators"],
        "radar_only": text["radar_only"],
        "filter_modes": text["filter_modes"],
    }
    values.update(
        {f"category_{index}": value for index, value in enumerate(categories)}
    )
    values.update(
        {
            f"height_category_{index}": value
            for index, value in enumerate(height_categories)
        }
    )
    for index, category_index in enumerate((5, 3)):
        values[f"mode_label_{index}"] = categories[category_index]
        values[f"available_{index}"] = text["available"]
        values[f"all_{index}"] = text["all"]
    if (
        set(values) != {entry[0] for entry in MAIN_LAYOUT}
        or not all(isinstance(value, str) and value for value in values.values())
    ):
        raise ValueError("localized text does not cover the complete main layout")
    return values


def build_main_overlay(font_path: Path, language: str, status: str) -> Image.Image:
    text = LOCALIZED[language]
    image = new_canvas()
    values = main_text_values(text, status)
    for name, x, y, width, height, role_scale, center in MAIN_LAYOUT:
        draw_slot(
            image, font_path, values[name], x, y, width, height,
            role_scale, center,
        )
    return image


def build_popup_overlay(font_path: Path) -> Image.Image:
    image = new_canvas()
    # Only these cells are hidden from game-font TextBlocks. Other choices
    # remain native and keep their existing visual metrics.
    for language, x, y, width, height, role_scale, center in POPUP_LAYOUT:
        draw_slot(
            image,
            font_path,
            LOCALIZED[language]["language_name"],
            x,
            y,
            width,
            height,
            role_scale,
            center,
        )
    return image


def write_tga(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="TGA", compression="tga_rle")


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


def glyph_signature(font: ImageFont.FreeTypeFont, value: str) -> tuple[tuple[int, int], bytes]:
    mask = font.getmask(value)
    return mask.size, bytes(mask)


def verify_glyph_coverage(font_path: Path) -> int:
    font = ImageFont.truetype(str(font_path), 32 * SCALE)
    missing_signature = glyph_signature(font, "\U0010FFFF")
    values: set[str] = set()
    for text in LOCALIZED.values():
        for status in ("off", "on", "fault"):
            values.update(main_text_values(text, status).values())
    characters = sorted(
        {character for value in values for character in value if not character.isspace()}
    )
    missing = [
        character
        for character in characters
        if glyph_signature(font, character) == missing_signature
    ]
    if missing:
        formatted = ", ".join(
            f"U+{ord(character):04X} {character!r}" for character in missing
        )
        raise ValueError(f"pinned font lacks required overlay glyphs: {formatted}")
    return len(characters)


def main() -> int:
    args = parse_args()
    font_path = args.font.resolve()
    output = args.output.resolve()
    if not font_path.is_file():
        raise SystemExit(f"font input not found: {font_path}")
    font_sha256 = sha256(font_path)
    if font_sha256 != EXPECTED_FONT_SHA256:
        raise SystemExit(
            "font input differs from the pinned DroidSansFallback payload: "
            f"{font_sha256}"
        )
    try:
        glyph_count = verify_glyph_coverage(font_path)
    except ValueError as error:
        raise SystemExit(str(error)) from error

    generated: list[dict[str, object]] = []
    for language in ("ko", "zh-hant"):
        for status in ("off", "on", "fault"):
            path = output / f"{language}-{status}.tga"
            write_tga(build_main_overlay(font_path, language, status), path)
            generated.append(
                {
                    "path": path.name,
                    "width": REFERENCE_WIDTH * SCALE,
                    "height": REFERENCE_HEIGHT * SCALE,
                    "sha256": sha256(path),
                }
            )

    popup = output / "language-popup.tga"
    write_tga(build_popup_overlay(font_path), popup)
    generated.append(
        {
            "path": popup.name,
            "width": REFERENCE_WIDTH * SCALE,
            "height": REFERENCE_HEIGHT * SCALE,
            "sha256": sha256(popup),
        }
    )
    manifest = {
        "schema_version": 2,
        "reference_size": [REFERENCE_WIDTH, REFERENCE_HEIGHT],
        "raster_scale": SCALE,
        "generator_sha256": sha256(Path(__file__).resolve()),
        "layout_sha256": canonical_sha256(
            {"main": MAIN_LAYOUT, "popup": POPUP_LAYOUT}
        ),
        "localizations_sha256": canonical_sha256(LOCALIZED),
        "font_source": FONT_SOURCE,
        "font_sha256": font_sha256,
        "verified_codepoint_count": glyph_count,
        "files": generated,
    }
    manifest_path = output / "manifest.json"
    with manifest_path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    print(f"generated {len(generated)} overlays in {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

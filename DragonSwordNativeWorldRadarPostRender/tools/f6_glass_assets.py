"""Deterministic SG-12 blue-gray skins and regular-weight localized text."""
from __future__ import annotations

import hashlib
import re
import unicodedata
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

THAI_FONT_SOURCE = "tools/f6-fonts/NotoSansThai-Variable.ttf"
THAI_FONT_SHA256 = "5A1C559BB539583C8A1FD99D1C5B9491E5E14478C9CD2BD0970D5C3096CC9EF8"
LANGUAGES = ("en", "ja", "ko", "zh-hans", "zh-hant", "fr", "de", "es-es", "ru", "th", "pt-br")
TOOLTIP_IDS = (
    "Treasure",
    "Boss",
    "Assault",
    "MiniGames",
    "AreaQuests",
    "BirdEggs",
    "Clock",
    "SceneTreasure",
    "SceneAreaQuests",
    "SceneMiniGames",
    "HeightTreasure",
    "HeightAreaQuests",
    "HeightMole",
    "HeightBoss",
    "HeightAssault",
    "AreaQuestAvailable",
    "AreaQuestAll",
    "AssaultAvailable",
    "AssaultAll",
    "SceneRange",
    "SceneLimit",
    "DistanceOff",
    "DistanceAim",
    "DistanceAuto",
    "DistanceAll",
    "Language",
    "ModStatus",
    "EnableDisable",
    "RestoreDefaults",
    "BugReport",
    "Close",
    "Endorse",
    "AllRadar",
    "AllMap",
)
CONFIRMATION_IDS = ("Endorse", "Title", "EndorseBody", "FeedbackBody", "RestoreBody", "Yes", "No")
CONFIRMATION_CROPS = ((206, 24), (320, 32), (320, 72), (320, 72), (320, 72), (144, 24), (144, 24))
NUMERIC_CHARACTERS = "0123456789 m-v"
NUMERIC_CROP = (16, 26)
NUMERIC_ADVANCES = (9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 5, 15, 9, 9)
TOOLTIP_SIZE = (640, 7920)
SKIN_SIZES = {
    "main-glass.tga": (1520, 1752), "popup-glass.tga": (1520, 1752),
    "chip-idle.tga": (444, 60), "chip-active.tga": (444, 60),
    "check-idle.tga": (88, 88), "check-active.tga": (88, 88),
}


def read_main_texts(root: Path) -> dict[str, dict]:
    source = (root / "include/dswros/radar_localization.hpp").read_text(encoding="utf-8")
    values = re.findall(r'L"([^"\\]*)"', source)
    if len(values) != len(LANGUAGES) * 78 or not all(values):
        raise ValueError("expected eleven complete localization records")
    result = {}
    for index, language in enumerate(LANGUAGES):
        block = values[index * 78:(index + 1) * 78]
        result[language] = {
            "language_name": block[0], "title": block[1], "language": block[2],
            "marker_visibility": block[4], "radar": block[5], "map": block[6],
            "scene": block[7], "categories": block[8:15],
            "height_indicators": block[15], "radar_only": block[16],
            "height_categories": block[17:22], "filter_modes": block[22],
            "available": block[23], "all": block[24], "close": block[25],
            "status": block[26],
            "status_values": dict(zip(("off", "on", "fault"), block[27:30])),
            "status_actions": dict(zip(("off", "on", "fault"), block[30:33])),
            "bug_report": block[33], "scene_settings": block[34],
            "scene_range": block[35], "scene_limit": block[36],
            "scene_distance": block[37], "scene_distance_modes": block[38:42],
            "restore_defaults": block[42], "all_markers": block[43],
        }
    return result


def read_tooltips(root: Path) -> dict[str, list[str]]:
    source = (root / "include/dswros/radar_localization.hpp").read_text(encoding="utf-8")
    values = re.findall(r'L"([^"\\]*)"', source)
    if len(values) != 11 * 78 or not all(values):
        raise ValueError("expected 11 complete 78-string localization blocks")
    enum = re.search(r"enum class RadarTooltipId[^\{]*\{([^}]+)\}", source)
    if enum is None or tuple(s.strip() for s in enum[1].split(",") if s.strip()) != TOOLTIP_IDS + ("Count",):
        raise ValueError("tooltip enum order differs from atlas tile order")
    return {language: values[index * 78 + 44:(index + 1) * 78]
            for index, language in enumerate(LANGUAGES)}


def read_confirmation_texts(root: Path) -> dict[str, list[str]]:
    source = (root / "include/dswros/radar_confirmation_localization.hpp").read_text(encoding="utf-8")
    values = re.findall(r'L"([^"\\]*)"', source)
    enum = re.search(r"enum class RadarConfirmationTextId[^\{]*\{([^}]+)\}", source)
    if (len(values) != 11 * 7 or not all(values) or enum is None
            or tuple(s.strip() for s in enum[1].split(",") if s.strip()) != CONFIRMATION_IDS + ("Count",)):
        raise ValueError("expected eleven complete seven-text confirmation blocks in exact crop order")
    return {language: values[index * 7:(index + 1) * 7] for index, language in enumerate(LANGUAGES)}


def tooltip_font(language: str, cjk: Path, latin: Path, thai: Path) -> Path:
    return thai if language == "th" else cjk if language in ("ja", "ko", "zh-hans", "zh-hant") else latin


def clusters(value: str) -> list[str]:
    result: list[str] = []
    for character in value:
        if result and (unicodedata.combining(character) or unicodedata.category(character) in ("Mn", "Mc", "Me")):
            result[-1] += character
        else:
            result.append(character)
    return result


def wrap_units(value: str) -> list[str]:
    """Keep embedded numbers/Latin names and closing punctuation together."""
    units: list[str] = []
    ascii_run = ""
    for cluster in clusters(value):
        if len(cluster) == 1 and cluster.isascii() and cluster.isalnum():
            ascii_run += cluster
            continue
        if ascii_run:
            units.append(ascii_run)
            ascii_run = ""
        if units and cluster in "，。；：！？、）》」』”’.;,!?%)]":
            units[-1] += cluster
        else:
            units.append(cluster)
    if ascii_run:
        units.append(ascii_run)
    return units


def wrap_text(value: str, font: ImageFont.FreeTypeFont, width: int) -> list[str]:
    # Prefer word boundaries. CJK and long Thai runs fall back to grapheme
    # clusters, keeping vowel/tone marks attached to their preceding base.
    words = re.findall(r"\S+\s*", value)
    lines: list[str] = []
    current = ""
    for word in words:
        if font.getlength(current + word.rstrip()) <= width:
            current += word
            continue
        if current.strip():
            lines.append(current.rstrip())
            current = ""
        if font.getlength(word.rstrip()) <= width:
            current = word
            continue
        for cluster in wrap_units(word):
            if current and font.getlength((current + cluster).rstrip()) > width:
                lines.append(current.rstrip())
                current = ""
            current += cluster
    if current.strip():
        lines.append(current.rstrip())
    # Avoid a single trailing CJK glyph after a long unspaced sentence. Move
    # complete clusters, preserving every character and the fixed font size.
    cjk = sum("\u3000" <= c <= "\u9fff" for c in value)
    if len(lines) > 1 and cjk > len(value) * 0.3:
        previous = wrap_units(lines[-2])
        while len(previous) > 2 and font.getlength(lines[-1]) < width * 0.35:
            lines[-1] = previous.pop() + lines[-1]
        lines[-2] = "".join(previous)
    return lines


def glass(image: Image.Image, box: tuple[float, float, float, float], radius: float,
          top=(43, 56, 71, 180), bottom=(29, 40, 54, 188), scale=2,
          gradient_height: float | None = None) -> None:
    """Round alpha mask, calm vertical tint and one-pixel specular edge."""
    x, y, width, height = (round(v * scale) for v in box)
    layer = Image.new("RGBA", (width, height))
    draw = ImageDraw.Draw(layer)
    for row in range(height):
        gradient_pixels = height if gradient_height is None else round(gradient_height * scale)
        mix = min(1.0, row / max(1, gradient_pixels - 1))
        color = tuple(round(a * (1 - mix) + b * mix) for a, b in zip(top, bottom))
        draw.line((0, row, width, row), fill=color)
    mask = Image.new("L", (width, height))
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, width - 1, height - 1), radius=round(radius * scale), fill=255)
    # Multiplying alpha preserves actual transparency instead of replacing it.
    from PIL import ImageChops
    layer.putalpha(ImageChops.multiply(layer.getchannel("A"), mask))
    edge = ImageDraw.Draw(layer)
    edge.rounded_rectangle((0, 0, width - 1, height - 1), radius=round(radius * scale), outline=(211, 226, 239, 58), width=1)
    image.alpha_composite(layer, (x, y))


def build_skins() -> dict[str, Image.Image]:
    def surface(image, box, radius, top, bottom, scale=2):
        """Quiet rounded fills; preserve opacity without a repeated bright rim."""
        from PIL import ImageChops
        x, y, width, height = (round(value * scale) for value in box)
        layer = Image.new("RGBA", (width, height))
        draw = ImageDraw.Draw(layer)
        for row in range(height):
            mix = row / max(1, height - 1)
            color = tuple(round(a * (1.0 - mix) + b * mix) for a, b in zip(top, bottom))
            draw.line((0, row, width, row), fill=color)
        mask = Image.new("L", (width, height))
        ImageDraw.Draw(mask).rounded_rectangle(
            (0, 0, width - 1, height - 1), radius=round(radius * scale), fill=255)
        layer.putalpha(ImageChops.multiply(layer.getchannel("A"), mask))
        image.alpha_composite(layer, (x, y))

    main = Image.new("RGBA", SKIN_SIZES["main-glass.tga"])
    # One calm sheet protects every label from busy scenery. Card fills vary
    # only slightly from it; rounded outlines do not compete with the text.
    surface(main, (0, 0, 760, 876), 22, (32, 43, 56, 231), (28, 38, 50, 233))
    for top, bottom in ((98, 344), (354, 562), (572, 688), (698, 808), (818, 868)):
        surface(main, (20, top, 720, bottom - top), 14,
                (94, 113, 132, 12), (76, 94, 113, 10))
    # Utility controls share a modest neutral fill. Their existing footprints
    # and the footer's fixed position remain unchanged.
    for box in ((36, 828, 222, 30), (270, 828, 222, 30), (504, 828, 222, 30), (652, 14, 88, 30),
                (24, 54, 344, 34), (380, 54, 220, 34)):
        surface(main, box, 10, (71, 87, 106, 38), (63, 79, 98, 34))
    separators = Image.new("RGBA", main.size)
    draw = ImageDraw.Draw(separators)
    for y in (349, 567, 693, 813):
        draw.line((72, y * 2, 1448, y * 2), fill=(164, 185, 207, 12), width=1)
    main.alpha_composite(separators)

    popup = Image.new("RGBA", SKIN_SIZES["popup-glass.tga"])
    ImageDraw.Draw(popup).rounded_rectangle((0, 0, 1519, 1751), radius=44, fill=(8, 13, 20, 80))
    surface(popup, (62, 85, 636, 192), 18, (43, 56, 72, 247), (34, 46, 61, 247))
    for index in range(12):
        surface(popup, (78 + 202 * (index % 3), 101 + 40 * (index // 3), 190, 32), 10,
                (76, 92, 111, 40), (62, 78, 97, 32))
    result = {"main-glass.tga": main, "popup-glass.tga": popup}
    for active in (False, True):
        image = Image.new("RGBA", (444, 60))
        # Ten reference-unit corner caps match the native nine-slice brush.
        # The idle fill also serves as the compact confirmation surface.
        surface(image, (0, 0, 222, 30), 10,
                (66, 104, 121, 244) if active else (48, 62, 79, 246),
                (53, 91, 109, 244) if active else (42, 55, 71, 246))
        result[f"chip-{'active' if active else 'idle'}.tga"] = image
        check = Image.new("RGBA", (88, 88))
        surface(check, (0, 0, 22, 22), 6,
                (79, 120, 141, 246) if active else (54, 70, 88, 246),
                (63, 100, 121, 246) if active else (46, 61, 79, 246), scale=4)
        if active:
            ImageDraw.Draw(check).line((23, 44, 38, 59, 65, 29), fill=(247, 251, 255, 255), width=8, joint="curve")
        result[f"check-{'active' if active else 'idle'}.tga"] = check
    return result


def build_tooltip_atlas(values: list[str], font_path: Path,
                        confirmation_values: list[str]) -> tuple[Image.Image, list[dict], list[dict]]:
    if len(values) != 34 or len(confirmation_values) != 7:
        raise ValueError("atlas requires exactly 34 tooltip and seven confirmation text tiles")
    atlas = Image.new("RGBA", TOOLTIP_SIZE)
    metrics = []
    for index, value in enumerate(values):
        # Measure complete shaped runs. Never clip, ellipsize or remove text.
        for font_size in (26,):
            font = ImageFont.truetype(str(font_path), font_size)
            lines = wrap_text(value, font, 592)
            line_height = max(font.getbbox(line)[3] - font.getbbox(line)[1] for line in lines) + 6
            total_height = line_height * len(lines) - 6
            if len(lines) <= 4 and total_height <= 116:
                break
        else:
            raise ValueError(f"tooltip cannot fit its complete 320x72 slot: {value}")
        tile = Image.new("RGBA", (640, 144))
        glass(tile, (0, 0, 320, 72), 12, (40, 54, 71, 246), (29, 41, 57, 250))
        draw = ImageDraw.Draw(tile)
        origin_y = (144 - total_height) // 2
        bounds = []
        for row, line in enumerate(lines):
            left, top, right, bottom = font.getbbox(line)
            x, y = 24 - left, origin_y + row * line_height - top
            draw.text((x, y), line, font=font, fill=(247, 253, 255, 255))
            bounds.append([x + left, y + top, x + right, y + bottom])
        if any(x0 < 24 or y0 < 12 or x1 > 616 or y1 > 132 for x0, y0, x1, y1 in bounds):
            raise ValueError(f"tooltip escaped safe text area: {value}")
        atlas.alpha_composite(tile, (0, index * 144))
        metrics.append({"id": TOOLTIP_IDS[index], "font_pixels": font_size,
                        "lines": lines, "bounds": bounds,
                        "text_sha256": hashlib.sha256(value.encode("utf-8")).hexdigest().upper()})
    confirmation_metrics = []
    for index, value in enumerate(confirmation_values):
        width, height = (size * 2 for size in CONFIRMATION_CROPS[index])
        body = index in (2, 3, 4)
        max_size = 32 if index == 1 else 26
        for font_size in (max_size,):
            font = ImageFont.truetype(str(font_path), font_size)
            lines = wrap_text(value, font, width - 24) if body else [value]
            ascent, descent = font.getmetrics()
            line_height = font_size + 8
            total_height = ascent + descent + line_height * (len(lines) - 1)
            # Thai font metrics include unused leading. Preserve the shared
            # baseline; the actual glyph bounds below enforce crop safety.
            if (len(lines) <= (4 if body else 1)
                    and all(font.getlength(line) <= width - 16 for line in lines)):
                break
        else:
            raise ValueError(f"confirmation text cannot fit its complete {width // 2}x{height // 2} crop: {value}")
        tile = Image.new("RGBA", (640, 144))
        draw = ImageDraw.Draw(tile)
        bounds = []
        origin_y = (height - total_height) // 2
        for row, line in enumerate(lines):
            left, top, right, bottom = font.getbbox(line, anchor="ls")
            run = Image.new("RGBA", (right - left + 4, bottom - top + 4))
            ImageDraw.Draw(run).text((2 - left, 2 - top), line, font=font,
                anchor="ls", fill=(237, 244, 249, 255))
            ink = run.getchannel("A").getbbox()
            if ink is None:
                raise ValueError(f"confirmation line has no visible text: {line}")
            run = run.crop(ink)
            x = 12 if body else (width - run.width) // 2
            baseline = origin_y + ascent + row * line_height
            y = baseline + top + ink[1] - 2
            tile.alpha_composite(run, (x, y))
            bounds.append([x, y, x + run.width, y + run.height])
        if any(x0 < 8 or y0 < 6 or x1 > width - 8 or y1 > height - 6 for x0, y0, x1, y1 in bounds):
            raise ValueError(f"confirmation text escaped crop: {value}")
        atlas.alpha_composite(tile, (0, (len(TOOLTIP_IDS) + index) * 144))
        confirmation_metrics.append({"id": CONFIRMATION_IDS[index], "crop": list(CONFIRMATION_CROPS[index]),
            "font_pixels": font_size, "lines": lines, "bounds": bounds,
            "text_sha256": hashlib.sha256(value.encode("utf-8")).hexdigest().upper()})
    numeric_tiles, _ = build_numeric_tiles(font_path)
    for index, tile in enumerate(numeric_tiles):
        atlas.alpha_composite(tile, (0, (41 + index) * 144))
    return atlas, metrics, confirmation_metrics


def build_numeric_tiles(font_path: Path) -> tuple[list[Image.Image], list[dict]]:
    tiles, metrics = [], []
    for index, character in enumerate(NUMERIC_CHARACTERS):
        font_size = 26 if character == "v" else 28
        font = ImageFont.truetype(str(font_path), font_size)
        tile = Image.new("RGBA", (640, 144))
        bounds = None
        if character != " ":
            left, top, right, bottom = font.getbbox(character, anchor="ls")
            width, height = right - left, bottom - top
            if width > NUMERIC_ADVANCES[index] * 2 or width > 32 or height > 52:
                raise ValueError(f"numeric character exceeds its unscaled advance/crop: {font_path.name}/{character}")
            ascent, descent = font.getmetrics()
            x = (32 - width) // 2 - left
            baseline = (52 - ascent - descent) // 2 + ascent
            ImageDraw.Draw(tile).text((x, baseline), character, font=font, anchor="ls", fill=(237, 244, 249, 255))
            bounds = list(tile.getchannel("A").getbbox())
        tiles.append(tile)
        metrics.append({"character": character, "crop": list(NUMERIC_CROP),
                        "advance": NUMERIC_ADVANCES[index], "font_pixels": font_size,
                        "bounds": bounds})
    return tiles, metrics


def verify_tooltip_glyphs(values: dict[str, list[str]], cjk: Path, latin: Path, thai: Path) -> int:
    if hashlib.sha256(thai.read_bytes()).hexdigest().upper() != THAI_FONT_SHA256:
        raise ValueError("Thai font differs from the unmodified source pin")
    characters: set[str] = set()
    for language, tips in values.items():
        font = ImageFont.truetype(str(tooltip_font(language, cjk, latin, thai)), 32)
        missing_mask = font.getmask("\U0010ffff")
        missing = (missing_mask.size, bytes(missing_mask))
        required = {c for value in tips for c in value if not c.isspace()}
        for character in required:
            mask = font.getmask(character)
            if (mask.size, bytes(mask)) == missing or not mask.getbbox():
                raise ValueError(f"{language} tooltip font lacks U+{ord(character):04X}")
        characters.update(required)
    return len(characters)

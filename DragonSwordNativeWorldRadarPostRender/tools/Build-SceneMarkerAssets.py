#!/usr/bin/env python3
"""Rasterize the accepted Scene geometry; no generated artwork/font.

Only six deterministic, transparent, uncompressed TGA files and their manifest
enter the runtime payload. The reference canvas and projection center preserve
the native projection center and UI-only chevron. SG-09 adds only a translucent
blue-gray backing inside SG-06's Area Quest rails. Pillow is build-only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from PIL import Image, ImageDraw, ImageFont

REFERENCE = (32, 36)
ANCHOR = (16, 16)
SCALE = 4
SUPERSAMPLE = 8
SG04_SOURCE_SHA256 = "80B6D324C8D090541DF40382D2FA0632C0F5C377E717FC3DF92B91FF60891AD0"
AREA_GLYPH_SCALE = 1.18
AREA_GREY_STROKE_WIDTH = 1.8
AREA_BACKING = {"points": [[16, 6.4], [25.6, 16], [16, 25.6], [6.4, 16]],
                "rgba": [49, 66, 85, 156]}
PALETTE = {
    "outline": (9, 14, 19, 240), "white": (255, 255, 255, 255),
    "TreasureOther": (255, 255, 255, 255),
    "TreasureMiniGame": (63, 230, 122, 255),
    "TreasureMap": (255, 162, 47, 255),
    "TreasurePuzzle": (67, 159, 255, 255),
    "AreaQuest": (171, 176, 184, 255),
    "MiniGame": (190, 110, 240, 255),
}
FILES = {
    "TreasureOther": "treasure-other.tga",
    "TreasureMiniGame": "treasure-mini-game.tga",
    "TreasureMap": "treasure-map.tga",
    "TreasurePuzzle": "treasure-puzzle.tga",
    "AreaQuest": "area-quest.tga",
    "MiniGame": "mini-game.tga",
}
# x/y are rectangle centers; angles match UMG's clockwise screen rotation.
CHEVRON = (
    (14.5, 29.5, 5, 2.4, 45, "outline"),
    (17.5, 29.5, 5, 2.4, -45, "outline"),
    (14.5, 29.5, 4.6, .95, 45, "kind"),
    (17.5, 29.5, 4.6, .95, -45, "kind"),
)
CHEST = (
    (16, 17.4, 18, 11.6, 0, "outline"), (16, 17.4, 15.4, 9, 0, "kind"),
    (16, 11.5, 18, 6.4, 0, "outline"), (16, 11.5, 15.4, 3.8, 0, "kind"),
    (16, 14.1, 15.4, 1.3, 0, "outline"), (16, 15.4, 4.5, 5.8, 0, "outline"),
    (16, 15.4, 1.9, 3.2, 0, "white"), (16, 16, .95, 1.3, 0, "outline"),
)
AREA = (
    (11.4, 11.4, 12, 1.4, -45, "kind"), (20.6, 11.4, 12, 1.4, 45, "kind"),
    (11.4, 20.6, 12, 1.4, 45, "kind"), (20.6, 20.6, 12, 1.4, -45, "kind"),
    (16, 16, 8.1, 3.5, 0, "outline"), (13.4, 16, 1.3, 1.3, 0, "white"),
    (16, 16, 1.3, 1.3, 0, "white"), (18.6, 16, 1.3, 1.3, 0, "white"),
)
FLAGS = (
    (16, 17.86, 2.79, 14.88, -35, "outline"),
    (16, 17.86, 2.79, 14.88, 35, "outline"),
    (16, 17.86, 1.24, 14.26, -35, "kind"),
    (16, 17.86, 1.24, 14.26, 35, "kind"),
    (10.42, 11.66, 5.58, 4.96, -35, "outline"),
    (21.58, 11.66, 5.58, 4.96, 35, "outline"),
    (10.42, 11.66, 4.34, 3.72, -35, "kind"),
    (21.58, 11.66, 4.34, 3.72, 35, "kind"),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def canonical(value: object) -> bytes:
    return json.dumps(value, ensure_ascii=True, sort_keys=True,
                      separators=(",", ":")).encode("ascii")


def styles(kind: str, *, baseline: bool = False) -> list[dict]:
    glyph = AREA if kind == "AreaQuest" else FLAGS if kind == "MiniGame" else CHEST
    pieces = [{"x": float(x), "y": float(y), "w": float(w), "h": float(h),
               "angle": float(angle), "rgba": list(PALETTE[kind if color == "kind" else color])}
              for x, y, w, h, angle, color in glyph + CHEVRON]
    if kind == "AreaQuest" and not baseline:
        # Only the first eight pieces are the body. Never scale the shared v,
        # texture canvas or projection anchor along with the enlarged body.
        for index, piece in enumerate(pieces[:8]):
            piece["x"] = ANCHOR[0] + (piece["x"] - ANCHOR[0]) * AREA_GLYPH_SCALE
            piece["y"] = ANCHOR[1] + (piece["y"] - ANCHOR[1]) * AREA_GLYPH_SCALE
            piece["w"] *= AREA_GLYPH_SCALE
            piece["h"] = AREA_GREY_STROKE_WIDTH if index < 4 else piece["h"] * AREA_GLYPH_SCALE
    return pieces


def corners(piece: dict) -> list[tuple[float, float]]:
    angle = math.radians(piece["angle"])
    c, s = math.cos(angle), math.sin(angle)
    return [(piece["x"] + dx * c - dy * s, piece["y"] + dx * s + dy * c)
            for dx, dy in ((-piece["w"] / 2, -piece["h"] / 2),
                           (piece["w"] / 2, -piece["h"] / 2),
                           (piece["w"] / 2, piece["h"] / 2),
                           (-piece["w"] / 2, piece["h"] / 2))]


def bounds(pieces: list[dict]) -> list[float]:
    points = [point for piece in pieces for point in corners(piece)]
    return [min(p[0] for p in points), min(p[1] for p in points),
            max(p[0] for p in points), max(p[1] for p in points)]


def raster_pieces(pieces: list[dict], backing: dict | None = None) -> Image.Image:
    factor = SCALE * SUPERSAMPLE
    image = Image.new("RGBA", (REFERENCE[0] * factor, REFERENCE[1] * factor))
    if backing is not None:
        ImageDraw.Draw(image).polygon([(x * factor, y * factor) for x, y in backing["points"]],
                                      fill=tuple(backing["rgba"]))
    for piece in pieces:
        layer = Image.new("RGBA", image.size)
        ImageDraw.Draw(layer).polygon([(x * factor, y * factor) for x, y in corners(piece)],
                                      fill=tuple(piece["rgba"]))
        image = Image.alpha_composite(image, layer)
    image = image.resize((REFERENCE[0] * SCALE, REFERENCE[1] * SCALE), Image.Resampling.LANCZOS)
    # Canonical transparent pixels avoid irrelevant RGB garbage and texture halos.
    pixels = bytearray(image.tobytes())
    for offset in range(0, len(pixels), 4):
        if pixels[offset + 3] == 0:
            pixels[offset:offset + 3] = b"\0\0\0"
    return Image.frombytes("RGBA", image.size, bytes(pixels))


def raster(kind: str) -> Image.Image:
    return raster_pieces(styles(kind), AREA_BACKING if kind == "AreaQuest" else None)


def glyph_metrics(pieces: list[dict]) -> dict:
    # Measure the body independently; cropping the full texture can include
    # the unchanged v once the Area Quest body becomes taller.
    alpha = raster_pieces(pieces[:8]).getchannel("A")
    opaque = alpha.point(lambda value: 255 if value > 127 else 0)
    values = alpha.tobytes()
    return {"alpha_128_bounds": opaque.getbbox(),
            "alpha_128_area_reference": sum(value > 127 for value in values) / (SCALE * SCALE),
            "weighted_area_reference": sum(values) / (255 * SCALE * SCALE)}


def tga_bytes(image: Image.Image) -> bytes:
    return struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0,
                       image.width, image.height, 32, 0x28) + image.tobytes("raw", "BGRA")


def build(output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)
    entries = []
    geometry = {kind: styles(kind) for kind in FILES}
    baseline_geometry = {kind: styles(kind, baseline=True) for kind in FILES}
    for kind, name in FILES.items():
        image = raster(kind)
        payload = tga_bytes(image)
        (output / name).write_bytes(payload)
        entries.append({"path": name, "kind": kind, "width": image.width,
                        "height": image.height, "bytes": len(payload),
                        "sha256": sha256(payload), "alpha_bounds": image.getchannel("A").getbbox(),
                        "glyph_bounds": bounds(geometry[kind][:8]),
                        "glyph_metrics": glyph_metrics(geometry[kind]),
                        "full_bounds": bounds(geometry[kind])})
    manifest = {"schema_version": 1, "reference_size": REFERENCE, "anchor": ANCHOR,
                "raster_scale": SCALE, "supersample": SUPERSAMPLE,
                "source_sg04_sha256": SG04_SOURCE_SHA256,
                "source_sg04_geometry_sha256": sha256(canonical(baseline_geometry)),
                "design_revision": "SG-09",
                "area_quest_adjustment": {"glyph_scale": AREA_GLYPH_SCALE,
                                          "grey_stroke_width": AREA_GREY_STROKE_WIDTH},
                "area_quest_backing": AREA_BACKING,
                "generator_sha256": sha256(Path(__file__).read_bytes()),
                "geometry_sha256": sha256(canonical(geometry)), "geometry": geometry,
                "palette": PALETTE, "files": entries}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def preview(output: Path, path: Path) -> None:
    image = Image.new("RGB", (1320, 1040), (232, 237, 243))
    draw = ImageDraw.Draw(image)
    font_path = Path(r"C:\Windows\Fonts\segoeui.ttf")
    font = ImageFont.truetype(str(font_path), 20) if font_path.is_file() else ImageFont.load_default()
    small = ImageFont.truetype(str(font_path), 14) if font_path.is_file() else font
    draw.text((24, 14), "SCENE SG-09 | source-derived texture preview | no game screenshot", font=font, fill=(28, 45, 62))
    for row, background in enumerate(((29, 40, 49), (215, 207, 186))):
        y = 64 + row * 340
        draw.rectangle((16, y, 1304, y + 322), fill=background)
        ink = (235, 240, 245) if row == 0 else (31, 46, 61)
        for col, (kind, name) in enumerate(FILES.items()):
            x = 28 + col * 214
            draw.text((x, y + 12), kind, font=small, fill=ink)
            texture = Image.open(output / name).convert("RGBA")
            normal = texture.resize(REFERENCE, Image.Resampling.LANCZOS)
            image.paste(normal, (x + 18, y + 57), normal)
            draw.text((x + 62, y + 60), "1x", font=small, fill=ink)
            image.paste(texture, (x + 20, y + 116), texture)
            draw.text((x + 154, y + 175), "4x", font=small, fill=ink)
    draw.rectangle((16, 750, 1304, 970), fill=(29, 40, 49))
    for col, (label, texture) in enumerate((
            ("Area Quest | SG-06 outline baseline", raster_pieces(styles("AreaQuest"))),
            ("Area Quest | SG-09 translucent backing", raster("AreaQuest")),
            ("Chest | unchanged", raster("TreasureOther")))):
        x = 28 + col * 428
        draw.text((x, 765), label, font=small, fill=(235, 240, 245))
        normal = texture.resize(REFERENCE, Image.Resampling.LANCZOS)
        image.paste(normal, (x + 18, 819), normal)
        draw.text((x + 62, 822), "1x", font=small, fill=(235, 240, 245))
        image.paste(texture, (x + 130, 808), texture)
        draw.text((x + 276, 855), "4x", font=small, fill=(235, 240, 245))
    draw.text((24, 988), "Area Quest: gray-blue translucent backing inside the same outline/dots. Same center and v; chest and flag pixels unchanged.", font=small, fill=(31, 46, 61))
    draw.text((24, 1011), "1x is one reference unit per image pixel. Runtime DPI, filtering and perceived frame-time changes remain unverified.", font=small, fill=(31, 46, 61))
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "assets/ui/scene")
    parser.add_argument("--preview", type=Path)
    args = parser.parse_args()
    build(args.output)
    if args.preview:
        preview(args.output, args.preview)
    print("Built six Scene marker textures and deterministic manifest.")


if __name__ == "__main__":
    main()

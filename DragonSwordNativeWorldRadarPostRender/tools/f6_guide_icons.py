"""Source-bound Radar guide icons; never installs or modifies game assets.

rect is (left, top, right, bottom), in the caller's pixel coordinates. Icons
are normalized to one legend cell; this does not depict their in-game sizes.
Compact and map glyphs deliberately have separate IDs because they differ.
"""
from __future__ import annotations

import ast
import functools
import hashlib
import json
import math
import re
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
COMPACT = "src/native/compact_umg_renderer.cpp"
WORLD = "src/native/world_map_umg_renderer.cpp"
MODEL = "include/dswros/compact_render_model.hpp"
SCENE_FILES = {
    "scene_treasure_other": "treasure-other.tga",
    "scene_treasure_minigame": "treasure-mini-game.tga",
    "scene_treasure_map": "treasure-map.tga",
    "scene_treasure_puzzle": "treasure-puzzle.tga",
    "scene_area_quest": "area-quest.tga",
    "scene_minigame": "mini-game.tga",
}
KINDS = ("treasure_other", "treasure_minigame", "treasure_map", "treasure_puzzle",
         "boss", "assault", "fly", "mole", "wave", "area_quest")
ICON_IDS = tuple("compact_" + kind for kind in KINDS) + ("compact_bird_egg",) \
    + tuple("map_" + kind for kind in KINDS) \
    + ("height_above", "height_below", "scene_pointer") + tuple(SCENE_FILES)
ICON_IDS += tuple(f"height_{kind}_{state}" for kind in ("treasure", "mini_game", "area", "boss", "assault")
                  for state in ("above", "level", "below", "unknown")) + ("clock_1638",)
ALIASES = {
    "treasure_common": "compact_treasure_other", "treasure_green": "compact_treasure_minigame",
    "treasure_gold": "compact_treasure_map", "treasure_blue": "compact_treasure_puzzle",
    "boss": "compact_boss", "assault": "compact_assault", "fly": "compact_fly",
    "mole": "compact_mole", "wave": "compact_wave", "area": "compact_area_quest",
    "egg": "compact_bird_egg", "height_up": "height_above", "height_down": "height_below",
    "map_treasure_common": "map_treasure_other", "map_treasure_green": "map_treasure_minigame",
    "map_treasure_gold": "map_treasure_map", "map_treasure_blue": "map_treasure_puzzle",
    "map_area": "map_area_quest", "scene_treasure": "scene_treasure_other",
    "scene_area": "scene_area_quest", "scene_mini_game": "scene_minigame",
}

# Exact, normalized function fingerprints bind the few translated drawing
# operations below, not merely the presence of a token somewhere in a source.
# Color/numeric constants and compact piece geometry are also parsed directly.
SOURCE_FINGERPRINTS = {
    COMPACT + '|band_height_selection': '2E96B66EC824FA27145F6C15482E09CB2F20E29B1C457B1DC49AFACD79555FF9',
    "src/native/compact_umg_renderer.cpp|sharp_head_segment": "F9A57399E9DAAC672A4235DD96A2DBCCD4803461BC5EA860E2B36AFCEB7B14AE",
    "src/native/compact_umg_renderer.cpp|height_outline_segments": "20B3A5CBC491B8B80B88704802DCFE12056388C7DBAF80A95D3AE9186D26E1FE",
    "src/native/compact_umg_renderer.cpp|height_arrow_right_extent": "EDE7FE2FC725C1E1730D6A68E5A0672C004ED1D8044349BDDADB49A6B3E3A749",
    "src/native/compact_umg_renderer.cpp|configure_encounter_triangle_brush": "4969325D7ADA0DB2B7CFA8FEE671F5D611F9D08E3868068F5A3EC012733CB916",
    "src/native/compact_umg_renderer.cpp|height_pointer_fill_color": "FE2796F8AA82F356648ACA2839B740B394CA435D0FC407A1A0879F2F01E01306",
    "src/native/compact_umg_renderer.cpp|configure_clock_phase_brush": "673016A4057ED0149BB7833BCF18F6F53DCFAA5E95475ACB31027C6A86B7F4F7",
    "src/native/compact_umg_renderer.cpp|CompactUmgRenderer::configure_mini_game_height_indicator_unsafe": "54F5BA9A5802C7B2B8F47A6313319390997085CA406ED87FC3A781560CA42365",
    "src/native/compact_umg_renderer.cpp|CompactUmgRenderer::apply_height_pointer_transform_unsafe": "D980629075BE89817ADB0866EACAF25E643AB8A4C63CF5169F53B194D1333C11",
    "src/native/compact_umg_renderer.cpp|CompactUmgRenderer::update_world_clock_unsafe": "5DB758ADD9A77C99D88272FE8ED53D6CDB38CAE3830F75DB7A3095E53B7EEF47",
    "src/native/compact_umg_renderer.cpp|height_attachment": "2C21F87FEBE3237ACEF8FDACA1D0DE97CBB6F3EA09E0DF7438138C1A1CF0D047",
    "src/native/compact_umg_renderer.cpp|height_constants": "0F3C5E26EC9B7ECB987C5702A8FC5FE219FD413DD892F174148CA6A013633390",
    "src/native/compact_umg_renderer.cpp|clock_geometry": "1566E3659C86A00DD42CA2EC499B69C1C87934BDFAB031918238D23F8435CBD8",
    "include/dswros/compact_render_model.hpp|compact_time_phase": "32124FC1E20AA6F380452AF67A76CE6CF78FF61F34DA0EF400F5A03D0A135D98",
    "include/dswros/compact_render_model.hpp|area_quest_height_indicator_shape": "328C693AEFFBE5F999535BFC4017FDA68217E03BCC475620B4B535B235834C2D",
    "include/dswros/compact_render_model.hpp|mini_game_height_indicator_shape": "C19E0C5F5B9B1C22C363EF06B8CD26438E761E2B11A5FAD6BD7A55AD67D29BCB",
    COMPACT + '|palette': 'B0322C6D8C5458952317314E6976EF7F9E0A972E339CFA2A36E7B3DBB6767F13',
    COMPACT + '|marker_piece_style': '668FF8D7F6871EF754064B733E295C651968BCE77396EE08A1D1AEB2976D4BEE',
    COMPACT + '|configure_area_quest_outline_brush': '9A882F56E49D214D58AEEB7C6A057890890BD5A86199D66BE901902D08956349',
    COMPACT + '|configure_fly_outline_brush': '19B27E388D5994424CBB2663209BC384A1114096B3F9606270C7317A4DE78731',
    COMPACT + '|configure_bird_egg_oval_brush': 'E37014798964FFC9B42F75DACCEFA91520B4FCE00E45A5516834CF148A73D76B',
    COMPACT + '|CompactUmgRenderer::configure_area_quest_marker_shape_unsafe': '6772A9206279587DBE6086F3EEBA591D82BBE15CAFD16E3F9740027E0CD47CAF',
    WORLD + '|palette': '04EC4053CC56B132C10BBA5FA6FE41CD5EA2620522CD91897CE24F57C4EFCF19',
    WORLD + '|draw_chest_glyph': 'FDFD5EF16F52C43A5AB385FC0B6E66D4172AF9CE3C4A88B7583411CA1ACC0E1E',
    WORLD + '|draw_boss_glyph': '96EEA4B98A0F27798130374ABFDF8121F926BC16044360650886AFDE9BD2F816',
    WORLD + '|draw_boss_silhouette': 'A5E747E0A692AD5B9F8937B547E0F4985E9B93065A40396431F8C2421E667EA6',
    WORLD + '|boss_silhouette_point': '8A5516D7DB4B327F0911303F6E4309E6868D4B26AD59C02B11FAFAF3AA6871EC',
    WORLD + '|draw_assault_glyph': 'C2F77228A60312E679F3A5B850B87C1DF4D1F37BADCD11D9295A3F3F908CA8B1',
    WORLD + '|draw_fly_glyph': 'EEFA7BC53268E4B5D81104A9141CDCB402E17B0BED0E19D24C216AAF87012DB4',
    WORLD + '|draw_mole_glyph': '8EFED7D8EFB7676E104F57376D3D40665063409B22CF0C6A42F6E364DB50EFEC',
    WORLD + '|draw_wave_glyph': '4ED5062F3B07DC812B2A8A174E179BD10FF38BAFB8335E0D0C33A3A34E6872A6',
    WORLD + '|draw_area_quest_glyph': 'CA20BCB9E46716669687B92B65B1D9534A31BF605E632DC27C581802AAF51337',
    WORLD + '|transform_polygon': '77C92EA472C72B77BE6F3DBBC6ECE6855FA538DBA497DF777B1E2D59C2CBDD01',
    WORLD + '|cubic_bezier': 'E09F641828149141FA8B0686167923DBCB33250BF3DDCDEC0B96E3F127E3A160',
    WORLD + '|draw_atlas_diamond': 'ECE79F2C175616EFD769BBA30DE7A74F07CFC67DFDF20D0C3795D754CF3D73AC',
}


def _function(source: str, name: str) -> str:
    match = re.search(r"(?:^|\n)(?:\[\[nodiscard\]\]\s*)?(?:[\w:<>,*&]+\s+)+"
                      + re.escape(name) + r"\s*\([^;]*?\)\s*(?:noexcept\s*)?\{", source)
    if match is None:
        raise ValueError(f"guide icon source function missing: {name}")
    start = source.index("{", match.start())
    depth = 1
    end = start + 1
    while depth:
        if end >= len(source):
            raise ValueError(f"unclosed icon function: {name}")
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end].strip()


def _digest(value: str) -> str:
    return hashlib.sha256(re.sub(r"\s+", " ", value).strip().encode()).hexdigest().upper()


def _source_block(source: str, name: str) -> str:
    if name == "palette":
        return "\n".join(re.findall(r"constexpr (?:LinearColor|std::uint32_t) k\w+\s*\{[^}]+\};|"
                                    r"constexpr std::uint32_t k\w+\s*=\s*0x[\dA-F]+U;", source))
    if name == "height_constants":
        return "\n".join(re.findall(r"constexpr double k(?:Reference(?:Height|MiniGameTriangle|TreasureHalfWidth)|AreaQuestMarkerTriangle|BandMarkerReferenceStroke|EncounterTriangleReferenceOutline)\w*\s*=[^;]+;", source))
    if name == "height_attachment":
        return source[source.index("    const double height_group_size ="):source.index("    std::array<UObject*, kCompactUmgHeightChannelCount> height_groups{};")]
    if name == "clock_geometry":
        arrays = "\n".join(re.findall(r"kClock(?:DigitLeft|SegmentGeometry|ColonY)\{\{[\s\S]*?\}\};", source))
        colon = source[source.index("        kClockColonY{{"):source.index("    std::array<UObject*, kCompactClockPhasePieceCount> clock_phase_pieces{};")]
        return arrays + "\n" + colon
    if name == "band_height_selection":
        match = re.search(r"if \(band_height_marker\) \{\s*(const auto shape = encounter_marker[\s\S]*?const auto shape_code = static_cast<std::uint8_t>\(shape\);)", source)
        if match is None:
            raise ValueError("guide height availability/off-state mapping is missing")
        return match[1]
    return _function(source, name)


def _number(expression: str, names=None) -> float:
    expression = re.sub(r"(?<=\d)[FU]\b", "", expression.strip())
    expression = expression.replace("center.x", "cx").replace("center.y", "cy")
    tree = ast.parse(expression, mode="eval")
    names = {"cx": 0.0, "cy": 0.0, **(names or {})}

    def visit(node):
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return node.value
        if isinstance(node, ast.Name) and node.id in names:
            return names[node.id]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            return visit(node.operand) * (-1 if isinstance(node.op, ast.USub) else 1)
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add): return a + b
            if isinstance(node.op, ast.Sub): return a - b
            if isinstance(node.op, ast.Mult): return a * b
            if isinstance(node.op, ast.Div): return a / b
        raise ValueError(f"unsupported guide geometry expression: {expression}")
    return float(visit(tree.body))


def _constant(source, name):
    match = re.search(r"constexpr\s+(?:double|std::size_t)\s+" + re.escape(name) + r"\s*=([^;]+);", source)
    if match is None: raise ValueError(f"missing numeric icon constant: {name}")
    return _number(match[1])


@functools.lru_cache(maxsize=1)
def _compact_reference_sizes():
    source = (ROOT / "src/native/main.cpp").read_text(encoding="utf-8")
    encounter = re.search(r"spec.kind == EncounterKind::Boss \? ([\d.]+) : ([\d.]+),\s*spec.kind",source)
    area = re.search(r"([\d.]+),\s*dsnwr::CompactUmgMarkerKind::AreaQuest,\s*show_height",source)
    mini = re.search(r"([\d.]+),\s*umg_mini_game_kind\(spec.kind\),\s*show_height",source)
    egg = re.search(r"candidate.position, ([\d.]+),\s*dsnwr::CompactUmgMarkerKind::BirdEgg",source)
    if not all((encounter,area,mini,egg)):
        raise ValueError("compact icon reference-size mapping changed")
    return {"boss":float(encounter[1]),"assault":float(encounter[2]),"area_quest":float(area[1]),
            "fly":float(mini[1]),"mole":float(mini[1]),"wave":float(mini[1]),"bird_egg":float(egg[1])}


@functools.lru_cache(maxsize=1)
def verify_sources() -> dict:
    sources = {name: (ROOT / name).read_text(encoding="utf-8") for name in (COMPACT, WORLD, MODEL)}
    if not SOURCE_FINGERPRINTS:
        raise ValueError("guide icon source fingerprints have not been established")
    observed = {}
    for key, expected in SOURCE_FINGERPRINTS.items():
        path, function = key.split("|")
        block = _source_block(sources[path], function)
        actual = _digest(block)
        if actual != expected:
            raise ValueError(f"guide icon geometry changed; review mapping: {key}")
        observed[key] = actual
    observed["src/native/main.cpp|compact_reference_sizes"] = _digest(
        json.dumps(_compact_reference_sizes(),sort_keys=True))
    scene = json.loads((ROOT / "assets/ui/scene/manifest.json").read_text(encoding="utf-8"))
    records = {entry["path"]: entry for entry in scene["files"]}
    for filename in SCENE_FILES.values():
        actual = hashlib.sha256((ROOT / "assets/ui/scene" / filename).read_bytes()).hexdigest().upper()
        if filename not in records or actual != records[filename]["sha256"]:
            raise ValueError(f"guide scene icon differs from verified manifest: {filename}")
        observed[filename] = actual
    return observed


@functools.lru_cache(maxsize=1)
def _sources():
    verify_sources()
    compact = (ROOT / COMPACT).read_text(encoding="utf-8")
    world = (ROOT / WORLD).read_text(encoding="utf-8")
    compact_colors = {name: tuple(round(_number(item) * 255) for item in body.split(","))
                      for name, body in re.findall(r"constexpr LinearColor (k\w+)\{([^}]+)\};", compact)}
    world_colors = {}
    for name, value in re.findall(r"constexpr std::uint32_t (k\w+)\s*=\s*0x([\dA-F]+)U;", world):
        argb = int(value, 16)
        world_colors[name] = ((argb >> 16) & 255, (argb >> 8) & 255, argb & 255, (argb >> 24) & 255)
    return compact, world, compact_colors, world_colors


class _Painter:
    def __init__(self, width=512, height=512):
        self.image = Image.new("RGBA", (width, height))

    def point(self, point):
        return (self.image.width / 2 + point[0] * 8, self.image.height / 2 + point[1] * 8)

    def polygon(self, points, color):
        layer = Image.new("RGBA", self.image.size)
        ImageDraw.Draw(layer).polygon([self.point(p) for p in points], fill=color)
        self.image.alpha_composite(layer)

    def rect(self, x, y, width, height, color, angle=0, oval=False):
        if oval:
            layer = Image.new("RGBA", self.image.size)
            ImageDraw.Draw(layer).ellipse((*self.point((x - width / 2, y - height / 2)),
                                          *self.point((x + width / 2, y + height / 2))), fill=color)
            self.image.alpha_composite(layer)
            return
        radians = math.radians(angle)
        co, si = math.cos(radians), math.sin(radians)
        self.polygon([(x + a * co - b * si, y + a * si + b * co)
                      for a, b in ((-width/2, -height/2), (width/2, -height/2),
                                   (width/2, height/2), (-width/2, height/2))], color)

    def diamond(self, size, color, x=0, y=0):
        self.polygon([(x, y-size/2), (x+size/2, y), (x, y+size/2), (x-size/2, y)], color)

    def framed_rect(self, x, y, width, height, fill, outline, stroke, angle=0):
        # Slate draws its translucent center and opaque inside outline as one
        # brush. Painting a translucent center over a solid square would make
        # it opaque, so form the complete RGBA piece before compositing.
        layer = Image.new("RGBA", self.image.size)
        draw = ImageDraw.Draw(layer)
        radians = math.radians(angle)
        co,si = math.cos(radians),math.sin(radians)
        for w,h,color in ((width,height,outline),(max(0,width-2*stroke),max(0,height-2*stroke),fill)):
            points=[self.point((x+a*co-b*si,y+a*si+b*co)) for a,b in
                    ((-w/2,-h/2),(w/2,-h/2),(w/2,h/2),(-w/2,h/2))]
            draw.polygon(points,fill=color)
        self.image.alpha_composite(layer)

    def line(self, start, end, width, color):
        dx, dy = end[0]-start[0], end[1]-start[1]
        self.rect((start[0]+end[0])/2, (start[1]+end[1])/2, math.hypot(dx,dy), width,
                  color, math.degrees(math.atan2(dy,dx)))

    def capsule(self, x, y, width, height, angle, color):
        # Slate RoundedBox with HalfHeightRadius, used by the live clock.
        if width < height:
            width, height, angle = height, width, angle + 90
        radians = math.radians(angle)
        self.rect(x, y, max(0.001, width-height), height, color, angle)
        offset = (width-height)/2
        for sign in (-1, 1):
            self.rect(x+sign*offset*math.cos(radians), y+sign*offset*math.sin(radians),
                      height, height, color, oval=True)


def _compact(kind, reference_override=None, area_dots=True):
    source, _, colors, _ = _sources()
    painter = _Painter()
    style = _function(source, "marker_piece_style")
    enum = {"boss": "Boss", "assault": "Assault", "fly": "Fly", "mole": "Mole", "wave": "Wave",
            "area_quest": "AreaQuest", "bird_egg": "BirdEgg"}
    reference = reference_override or _compact_reference_sizes().get(kind,25.0)
    fill_name = {"treasure_other": "kTreasureOther", "treasure_minigame": "kTreasureMiniGame",
                 "treasure_map": "kTreasureMap", "treasure_puzzle": "kTreasurePuzzle"}
    if kind in fill_name:
        block = style[:style.index("const double encounter_inner_extent")]
    else:
        begin = style.index("case CompactUmgMarkerKind::" + enum[kind] + ":")
        match = re.search(r"\n    case CompactUmgMarkerKind::", style[begin+5:])
        block = style[begin:begin+5+match.start()] if match else style[begin:]
    records = re.findall(r"(?:case \d+|default): return \{([^}]+)\};", block)[:4]
    if len(records) != 4: raise ValueError(f"expected four compact icon pieces: {kind}")
    stroke = _constant(source, "kBandMarkerReferenceStroke")
    names = {"encounter_inner_extent": max(0, .98 - 2 * stroke / max(1, reference))}
    for index, record in enumerate(records):
        if kind == "area_quest" and not area_dots and index > 0:
            continue
        entries = [entry.strip() for entry in record.split(",")]
        width, height, x, y, angle = [_number(entry, names) for entry in entries[:5]]
        color = colors[fill_name[kind] if entries[5] == "fill" else entries[5]]
        w, h, px, py = width*reference, height*reference, x*reference, y*reference
        if kind == "area_quest" and index == 0:
            painter.framed_rect(px,py,w,h,colors["kAreaQuestBackdrop"],colors["kOutline"],stroke,angle)
        elif kind == "fly":
            painter.rect(px, py, w, h, colors["kFlyOutline"], angle)
            painter.rect(px, py, max(0,w-2.5), max(0,h-2.5), color, angle)
        else:
            painter.rect(px, py, w, h, color, angle, oval=kind == "bird_egg")
    return painter.image


def _points(block, names):
    return [(_number(a,names), _number(b,names)) for a,b in re.findall(r"Vector2D\{([^,}]+),([^}]+)\}", block)]


def _named_polygon(block, name, names):
    match = re.search(r"const auto " + name + r" = std::array\{(.*?)\n    \};", block, re.S)
    if match is None: raise ValueError(f"guide map polygon missing: {name}")
    return _points(match[1], names)


def _transform(points, scale=1, dx=0, dy=0, origin=(0,0)):
    return [(origin[0]+(x-origin[0])*scale+dx, origin[1]+(y-origin[1])*scale+dy) for x,y in points]


def _map(kind):
    _, source, _, colors = _sources()
    painter = _Painter()
    name = "chest" if kind.startswith("treasure_") else "area_quest" if kind == "area_quest" else kind
    block = _function(source, "draw_" + name + "_glyph")
    boss = _constant(source, "kBossMarkerSize")
    assault = _constant(source, "kAssaultMarkerSize")
    area = _constant(source, "kAreaQuestMarkerSize")
    d = _constant(source, "kMiniGameMarkerSize")
    if name == "chest":
        tone = {"treasure_other":"kWhite", "treasure_minigame":"kGreen", "treasure_map":"kOrange", "treasure_puzzle":"kBlue"}[kind]
        painter.rect(.6,.8,10.8,8.8,colors["kShadow"])
        polys = re.findall(r"draw_atlas_polygon_aa\(pixels, bounds, std::array\{(.*?)\}, (\w+)\);", block,re.S)
        for points, color in polys:
            painter.polygon(_points(points,{}), colors[tone if color == "fill" else color])
        for x,y,w,h,color in ((0,2.2,11.2,5.8,"kOutline"),(0,2,8.8,3.6,tone),
                              (0,1.4,2.8,4.4,"kOutline"),(0,1.1,1.2,1.8,"kOfficialPale")):
            painter.rect(x,y,w,h,colors[color])
    elif name == "boss":
        painter.diamond(boss,colors["kShadow"],.9,.9)
        painter.diamond(boss,colors["kOfficialWhite"])
        painter.diamond(boss-5,colors["kOfficialGreenDark"])
        silhouette = _function(source,"draw_boss_silhouette")
        for poly in re.findall(r"std::array\{(.*?)\}, color\);",silhouette,re.S):
            points = [(float(x),float(y)) for x,y in re.findall(r"point\(([^,]+), ([^)]+)\)",poly)]
            painter.polygon([((x-.5)*.5*boss,(-.28+y*.56)*boss) for x,y in points],colors["kOfficialWhite"])
        for x in (-2.2,2.2): painter.rect(x,-1,2.2,2.2,colors["kOfficialGreenDark"])
    elif name == "assault":
        painter.diamond(assault,colors["kShadow"],.9,.9)
        for size,color in ((assault,"kOfficialWhite"),(assault-4,"kOfficialPale"),(assault-8,"kOfficialGreenDark")):
            painter.diamond(size,colors[color])
        for x,y,w,h,color in ((0,-3.2,5.2,13,"kOfficialWhite"),(0,-3.2,3,10.4,"kOfficialCyan"),
                             (0,7.2,5.4,5.4,"kOfficialWhite"),(0,7.2,3.2,3.2,"kOfficialCyan")):
            painter.rect(x,y,w,h,colors[color])
    elif name in ("fly","mole","wave"):
        outline_scale = _constant(block,"outline_scale")
        if name == "fly":
            wing = re.search(r"const auto wing = .*?return std::array\{(.*?)\n        \};",block,re.S)[1]
            shapes = [_points(wing,{"d":d,"side":side}) for side in (-1,1)]
            shapes.append(_named_polygon(block,"arrow",{"d":d}))
            fills = ["kFlyWing","kFlyWing","kFlyArrow"]
            origins = [(0,0)]*3
        elif name == "mole":
            shapes = [_named_polygon(block,k,{"d":d}) for k in ("handle","head")]
            fills,origins = ["kHammerHandle","kHammer"],[(0,0)]*2
        else:
            shapes,origins = [],[]
            steps = int(_constant(block,"curve_steps"))
            for strand in range(4):
                offset=(strand-1.5)*d*.11
                points = [(_number(a,{"d":d,"offset":offset}),_number(b,{"d":d,"offset":offset}))
                          for a,b in re.findall(r"const Vector2D p\d\{([^,]+),([^}]+)\};",block)]
                def curve(ids,t):
                    weights=((1-t)**3,3*(1-t)**2*t,3*(1-t)*t*t,t**3)
                    return tuple(sum(points[i][axis]*w for i,w in zip(ids,weights)) for axis in (0,1))
                shape=[points[0]]+[curve((0,1,2,3),s/steps) for s in range(1,steps+1)]
                shape += [curve((3,4,5,6),s/steps) for s in range(1,steps+1)] + [points[7]]
                shape += [curve((7,8,9,0),s/steps) for s in range(1,steps+1)]
                shapes.append(shape); origins.append((offset,0))
            fills=["kWave"]*4
        for shape in shapes: painter.polygon(_transform(shape,dx=.9,dy=.9),colors["kShadow"])
        for shape,origin in zip(shapes,origins): painter.polygon(_transform(shape,outline_scale,origin=origin),colors["kOutline"])
        for shape,color in zip(shapes,fills): painter.polygon(shape,colors[color])
        if name == "mole": painter.polygon(_transform(shapes[1],.72,-d*.035,-d*.035),colors["kHammerHighlight"])
    elif name == "area_quest":
        painter.diamond(area,colors["kShadow"],1,1)
        painter.diamond(area,colors["kAreaQuestBubble"])
        painter.diamond(area-4,colors["kAreaQuestDark"])
        painter.polygon(_points(block,{}),colors["kOfficialWhite"])
        for x in (-3.8,0,3.8): painter.rect(x,-.8,2.2,2.2,colors["kAreaQuestDark"])
    return painter.image


def _height(above):
    source, _, colors, _ = _sources()
    painter = _Painter()
    # This is the actual Area Quest above/below replacement marker. Boss and
    # Assault use the same geometry with their category tint and dark outline.
    reference = _compact_reference_sizes()["area_quest"]
    width = _constant(source,"kAreaQuestMarkerTriangleHalfWidth") * reference
    height = _constant(source,"kAreaQuestMarkerTriangleHalfHeight") * reference
    apex,base=(-height,height) if above else (height,-height)
    points=[(-width,base),(0,apex),(width,base)]
    for start,end in zip(points,points[1:]+points[:1]):
        painter.line(start,end,_constant(source,"kBandMarkerReferenceStroke"),colors["kOutline"])
    return painter.image


def _sharp_head(tip_x, tip_y, base_x, base_y, thickness, upper):
    dx, dy = base_x-tip_x, (base_y-tip_y) * (1 if upper else -1)
    distance_squared, radius = dx*dx+dy*dy, thickness/2
    tangent = radius*math.sqrt(distance_squared-radius*radius)/distance_squared
    radial = radius*radius/distance_squared
    x, y = tip_x+radial*dx-tangent*dy, tip_y+radial*dy+tangent*dx
    return ((x, y if upper else 2*tip_y-y), (base_x, base_y))


@functools.lru_cache(maxsize=20)
def _height_demo(kind, state):
    source, _, colors, _ = _sources()
    painter = _Painter(768, 640)

    def marker(name, x=0, y=0, reference=None, dots=True):
        graphic = _compact(name, reference_override=reference, area_dots=dots)
        painter.image.alpha_composite(graphic, (round((painter.image.width-graphic.width)/2+x*8),
                                                round((painter.image.height-graphic.height)/2+y*8)))

    if kind == "treasure":
        reference, marker_x = 22.0, 12.0
        marker("treasure_other", marker_x, reference=reference)
        if state == "unknown":
            return painter.image
        length, head, half, outline, inner, inset, tail = (
            _constant(source, "kReferenceHeight"+name) for name in
            ("Length", "HeadLength", "HeadHalfWidth", "OutlineWidth", "InnerWidth", "EndCapInset", "TailInset"))
        upper = _sharp_head(0, 0, -head, half, outline, True)
        lower = _sharp_head(0, 0, -head, -half, outline, False)
        outer = [((-length, 0), (-head+1.0, 0)), upper, lower]
        distance = math.hypot(head-inset, half)
        base_x, base_y = -head+(head-inset)/distance*inset, half-half/distance*inset
        parts = [(outer[0], outline, colors["kOutline"]),
                 (((-length+tail, 0), (-inset, 0)), inner, colors["kTreasureOther"]),
                 (upper, outline, colors["kOutline"]), (lower, outline, colors["kOutline"]),
                 (_sharp_head(-inset, 0, base_x, base_y, inner, True), inner, colors["kTreasureOther"]),
                 (_sharp_head(-inset, 0, base_x, -base_y, inner, False), inner, colors["kTreasureOther"])]
        # Three examples of the live continuous angle, rather than discrete
        # up/down states. Its horizontal state is intentionally still visible.
        angle = -35 if state == "above" else 35 if state == "below" else 0
        co, si = math.cos(math.radians(angle)), math.sin(math.radians(angle))
        rotate = lambda p: (p[0]*co-p[1]*si, p[0]*si+p[1]*co)
        extent = 0.0
        for start, end in outer:
            dx, dy = end[0]-start[0], end[1]-start[1]
            distance = math.hypot(dx, dy)
            nx, ny = -dy/distance*outline/2, dx/distance*outline/2
            for point in (start, end):
                for sign in (-1, 1):
                    extent = max(extent, rotate((point[0]+sign*nx, point[1]+sign*ny))[0])
        offset = marker_x-reference*_constant(source,"kReferenceTreasureHalfWidth") \
            -_constant(source,"kReferenceHeightClearance")-extent
        for (start, end), width, color in parts:
            start, end = rotate(start), rotate(end)
            painter.line((start[0]+offset,start[1]),(end[0]+offset,end[1]),width,color)
    elif kind == "mini_game":
        reference = _compact_reference_sizes()["mole"]
        marker("mole", y=-7)
        if state in ("above", "below"):
            width = _constant(source,"kReferenceMiniGameTriangleHalfWidth")
            height = _constant(source,"kReferenceMiniGameTriangleHalfHeight")
            outline = _constant(source,"kReferenceMiniGameTriangleOutlineWidth")
            fill_height = _constant(source,"kReferenceMiniGameTriangleFillBarHeight")
            cy = -7+reference*_constant(source,"kReferenceTreasureHalfWidth") \
                +_constant(source,"kReferenceMiniGameTriangleClearance")+height
            apex, base = (cy-height,cy+height) if state == "above" else (cy+height,cy-height)
            triangle = [(-width,base),(0,apex),(width,base)]
            for start,end in zip(triangle,triangle[1:]+triangle[:1]):
                painter.line(start,end,outline,colors["kOutline"])
            for index in range(3):
                fraction = (index+1)/4
                bar_width = max(fill_height*.75,width*2*fraction-outline*1.15)
                painter.rect(0,apex+(base-apex)*fraction,bar_width,fill_height,colors["kHammer"])
    else:
        name = "area_quest" if kind == "area" else kind
        if state in ("level", "unknown"):
            marker(name, dots=state == "level")
        else:
            reference = _compact_reference_sizes()[name]
            width = _constant(source,"kAreaQuestMarkerTriangleHalfWidth")*reference
            height = _constant(source,"kAreaQuestMarkerTriangleHalfHeight")*reference
            apex,base = (-height,height) if state == "above" else (height,-height)
            triangle = [(-width,base),(0,apex),(width,base)]
            core = _constant(source,"kBandMarkerReferenceStroke")
            border = 0 if kind == "area" else _constant(source,"kEncounterTriangleReferenceOutline")
            fill = colors["kOutline" if kind == "area" else "kOfficialWhite" if kind == "boss" else "kOfficialCyan"]
            for start,end in zip(triangle,triangle[1:]+triangle[:1]):
                if not border:
                    painter.line(start,end,core,fill)
                else:
                    dx,dy=end[0]-start[0],end[1]-start[1]
                    painter.framed_rect((start[0]+end[0])/2,(start[1]+end[1])/2,
                                        math.hypot(dx,dy),core+2*border,fill,
                                        colors["kOfficialGreenDark"],border,math.degrees(math.atan2(dy,dx)))
    return painter.image


def _clock_1638():
    source, _, colors, _ = _sources()
    block = _function(source, "CompactUmgRenderer::update_world_clock_unsafe")
    painter = _Painter(1024, 384)
    lefts = [float(value) for value in re.search(r"kClockDigitLeft\{\{([^}]+)\}\}",source)[1].split(",")]
    segment_block = re.search(r"kClockSegmentGeometry\{\{(.*?)\}\};",source,re.S)[1]
    segments = [[float(value) for value in row.split(",")] for row in re.findall(r"\{([^}]+)\}",segment_block)]
    masks = [int(value,16) for value in re.findall(r"0x([\dA-F]+)U",re.search(r"kDigitMasks\{\{(.*?)\}\};",block,re.S)[1])]
    for digit, left in zip((1,6,3,8),lefts):
        for index,(x,y,w,h) in enumerate(segments):
            if masks[digit] & (1 << index):
                painter.rect(left+x-58,y-15,w,h,colors["kClockDigit"])
    ys = [float(value) for value in re.search(r"kClockColonY\{\{([^}]+)\}\}",source)[1].split(",")]
    for y in ys:
        painter.rect(38.5+1.7-58,y-15,3.4,3.4,colors["kClockDigit"])
    # 16:38 is the live afternoon presentation band: rounded sun and 8 rays.
    painter.capsule(98-58,0,24,24,45,colors["kClockDial"])
    painter.capsule(98-58,0,9,9,45,colors["kClockAfternoon"])
    for index in range(8):
        angle=45*index
        radians=math.radians(angle)
        painter.capsule(98-58+math.sin(radians)*9,-math.cos(radians)*9,
                        1.8,4.6,angle,colors["kClockAfternoon"])
    return painter.image


@functools.lru_cache(maxsize=96)
def _icon(kind):
    verify_sources()
    if kind in SCENE_FILES or kind == "scene_pointer":
        filename=SCENE_FILES.get(kind,"treasure-other.tga")
        image=Image.open(ROOT/"assets/ui/scene"/filename).convert("RGBA")
        if kind == "scene_pointer":
            # Authored reference y=28..36 isolates the existing small down-v.
            # It points at the scene anchor; it is not a height comparison.
            image=image.crop((0,112,128,144))
        return image
    if kind.startswith("compact_"): return _compact(kind.removeprefix("compact_"))
    if kind.startswith("map_"): return _map(kind.removeprefix("map_"))
    if kind in ("height_above","height_below"): return _height(kind == "height_above")
    if kind.startswith("height_"):
        name,state=kind.removeprefix("height_").rsplit("_",1)
        return _height_demo(name,state)
    if kind == "clock_1638": return _clock_1638()
    raise ValueError(f"unknown guide icon: {kind}")


def draw_legend_icon(image: Image.Image, kind: str, rect) -> None:
    """Composite a source-derived icon inside a caller-owned LTRB rectangle."""
    kind=ALIASES.get(kind,kind)
    if kind not in ICON_IDS: raise ValueError(f"unknown guide icon: {kind}")
    left,top,right,bottom=map(int,rect)
    if image.mode != "RGBA" or right <= left or bottom <= top:
        raise ValueError("guide icons require an RGBA image and positive LTRB rectangle")
    source=_icon(kind)
    bounds=source.getchannel("A").getbbox()
    if not bounds: raise ValueError(f"empty guide icon: {kind}")
    if re.fullmatch(r"height_(treasure|mini_game|area|boss|assault)_(above|level|below|unknown)",kind):
        name=kind.removeprefix("height_").rsplit("_",1)[0]
        # Keep one scale and anchor for all four states of a category. Cropping
        # each independently would falsely enlarge the aligned/unknown glyph.
        boxes=[_icon(f"height_{name}_{state}").getchannel("A").getbbox()
               for state in ("above","level","below","unknown")]
        bounds=(min(box[0] for box in boxes),min(box[1] for box in boxes),
                max(box[2] for box in boxes),max(box[3] for box in boxes))
    source=source.crop(bounds)
    available=(max(1,round((right-left)*.82)),max(1,round((bottom-top)*.82)))
    if kind == "scene_pointer": available=(max(1,round((right-left)*.42)),max(1,round((bottom-top)*.42)))
    ratio=min(available[0]/source.width,available[1]/source.height)
    source=source.resize((max(1,round(source.width*ratio)),max(1,round(source.height*ratio))),Image.Resampling.LANCZOS)
    image.alpha_composite(source,(left+(right-left-source.width)//2,top+(bottom-top-source.height)//2))


draw_icon = draw_legend_icon

"""SG-12 asset and native-consumer contract checks; no game interaction."""
from __future__ import annotations

import hashlib
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

import f6_glass_assets as assets

EXPECTED_LANGUAGES = ("en", "ja", "ko", "zh-hans", "zh-hant", "fr", "de", "es-es", "ru", "th", "pt-br")
EXPECTED_TIPS = (
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
EXPECTED_CONFIRMATIONS = ("Endorse", "Title", "EndorseBody", "FeedbackBody", "RestoreBody", "Yes", "No")
EXPECTED_CONFIRMATION_CROPS = ((206, 24), (320, 32), (320, 72), (320, 72), (320, 72), (144, 24), (144, 24))
EXPECTED_NUMERIC_CHARACTERS = "0123456789 m-v"
EXPECTED_NUMERIC_ADVANCES = (9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 5, 15, 9, 9)
EXPECTED_SIZES = {"main-glass.tga": (1520, 1752), "popup-glass.tga": (1520, 1752),
    "chip-idle.tga": (444, 60), "chip-active.tga": (444, 60),
    "check-idle.tga": (88, 88), "check-active.tga": (88, 88),
    **{f"tooltip-{language}.tga": (640, 7920) for language in EXPECTED_LANGUAGES}}


def verify_column_all_controls(hub, header, fail, require, method) -> None:
    """Check All's real readback path, ownership and independent column scope."""
    for snippet in (
        "std::array<RC::Unreal::FWeakObjectPtr, 2> column_all_controls_{};",
        "std::array<RC::Unreal::FWeakObjectPtr, 2> column_all_visuals_{};",
        "std::array<bool, 2> column_all_selected_{};",
        "kRadarVisibilityAllCategories = 0x7FU;",
        "kRadarVisibilityWorldCategories = 0x3EU;",
    ):
        require(header, snippet, "All owns only Radar's seven and Map's five supported members")
    opened = method(hub, "open_unsafe")
    for snippet in (
        "LocalizedTextSlot::MarkerAll, localized.all_markers, kContentX, kMarkerAllY, 420.0, 24.0, 5, 0.4375",
        "const bool enabled = (mask & kColumnCategories[column]) == kColumnCategories[column];",
        "column_all_controls[column] = add_control(control_x[column] + 10.0, kMarkerAllY, 24.0, 24.0, enabled);",
        "column_all_visuals[column] = add_border(control_x[column] + 11.0, kMarkerAllY + 1.0, 22.0, 22.0, 11, column == 0 ? kCompactEnabled : kWorldEnabled);",
        "column_all_selected_[column] = enabled;",
        "column_all_controls_[column] = column_all_controls[column];",
        "column_all_visuals_[column] = column_all_visuals[column];",
        "bind_tip(column_all_controls[0], dswros::RadarTooltipId::AllRadar);",
        "bind_tip(column_all_controls[1], dswros::RadarTooltipId::AllMap);",
    ):
        require(opened, snippet, "All starts unchecked for partial columns and uses matching square hit targets")
    service = method(hub, "service_unsafe")
    all_start = service.find("for (std::size_t column = 0; column < column_all_controls_.size(); ++column)")
    members_start = service.find("for (std::size_t column = 0; column < kColumnCount; ++column)", all_start)
    packed = service.find("pending_masks_ = pack_radar_visibility_masks(compact, world, scene);", members_start)
    if not 0 <= all_start < members_start < packed:
        fail("All requests must precede member readback and the single packed-mask publication")
    request = service[all_start:members_start]
    for snippet in (
        "const bool selected = is_checked(control, is_checked_);",
        "if (selected == column_all_selected_[column]) continue;",
        "if ((kColumnCategories[column] & (1U << category)) == 0U) continue;",
        "UObject* member = controls_[column][category].Get();",
        "set_checked(member, set_is_checked_, selected);",
    ):
        require(request, snippet, "Only an explicit All click changes that column's supported controls")
    if any(token in request for token in ("height_controls_", "pending_scene_settings_", "pending_language_", "mode_control")):
        fail("All must not alter Scene, height, language or filters")
    display = service[packed:service.find("for (std::size_t index", packed)]
    for snippet in (
        "const auto mask = column == 0 ? compact : world;",
        "const bool selected = (mask & kColumnCategories[column]) == kColumnCategories[column];",
        "if (selected != column_all_selected_[column]) { set_checked(column_all_controls_[column].Get(), set_is_checked_, selected);",
        "column_all_selected_[column] = selected;",
    ):
        require(display, snippet, "Member changes refresh All without rewriting peers or every-tick paints")
    reset = method(hub, "reset_runtime_handles")
    for snippet in ("column_all_controls_.fill(FWeakObjectPtr{});",
                    "column_all_visuals_.fill(FWeakObjectPtr{});", "column_all_selected_.fill(false);"):
        require(reset, snippet, "All retains no stale widget or displayed state across Travel")


def verify_native(hub_path: Path, fail, require, method) -> None:
    hub = hub_path.read_text(encoding="utf-8")
    header = hub_path.with_suffix(".hpp").read_text(encoding="utf-8")
    verify_native_status_readability(hub, fail)
    verify_column_all_controls(hub, header, fail, require, method)
    for snippet in (
        "if (main_glass) return main_glass;",
        "if (main_glass && (&color == &kPanelBackground || &color == &kPanelAccent",
        "|| &color == &kContentBackground || &color == &kBugReportButton",
        "|| &color == &kCloseButton || &color == &kLanguageSelector)) return main_glass;",
        "if (popup_glass && (&color == &kPanelFrame || &color == &kPopupBackground",
        "|| &color == &kLanguageOption)) return popup_glass;",
    ):
        require(hub, snippet, "accepted translucent sheets must replace native opaque backdrop layers")
    for snippet in (
        "constexpr double kTooltipReferenceWidth = 320.0;",
        "constexpr double kTooltipReferenceHeight = 72.0;",
        "constexpr double kChipSkinReferenceWidth = 222.0;",
        "constexpr double kChipSkinReferenceHeight = 30.0;",
        "constexpr double kChipCornerRadius = 10.0;",
        "BrushColorParameters parameters{decode_ui_srgb(color)};",
        "add_border(control_x[column] + 11.0, y + 1.0, 22.0, 22.0, 11, column == 0 ? kCompactEnabled : kWorldEnabled)",
        "add_control(control_x[column] + 10.0, y, 24.0, 24.0, enabled)",
        "add_border(control_x[column] + 11.0, y + 1.0, 22.0, 22.0, 10, kToggleFrame)",
        "if ((index == 2U || index == 3U) && !configure_chip_nine_slice_unsafe(image, unit_scale)) return nullptr;",
        "exact_parameter(set_tool_tip_, L\"Widget\", 0, 8, 8)",
        "exact_parameter(set_content_, L\"Content\", 0, 8, 16)",
        "exact_parameter(set_content_, L\"ReturnValue\", 8, 8, 16)",
        "exact_parameter(set_width_override_, L\"InWidthOverride\", 0, 4, 4)",
        "exact_parameter(set_height_override_, L\"InHeightOverride\", 0, 4, 4)",
        "exact_parameter(set_clipping_, L\"InClipping\", 0, 1, 1)",
    ):
        require(hub, snippet, "SG-06 square check / skin / tooltip ABI")
    rows = re.search(r"kRows\{\{(.*?)\}\};", hub, re.S)
    expected_rows = ["Treasure", "Boss", "Assault", "MiniGames", "AreaQuests", "BirdEggs", "Clock"]
    if rows is None or re.findall(r"RadarVisibilityCategory::(\w+)", rows[1]) != expected_rows:
        fail("display rows must put Clock after Bird Eggs without changing enum bits")
    require(header, "static constexpr std::size_t kMaximumTooltipCount = 64;", "bounded tooltip owners")
    require(header, "std::array<TooltipRecord, kMaximumTooltipCount> tooltips_{};", "fixed tooltip records")
    create = method(hub, "create_tooltip_content_unsafe")
    for snippet in (
        "ByteParameters clipping{1};", "canvas->ProcessEvent(set_clipping_, &clipping);",
        "-kTooltipReferenceHeight * static_cast<double>(tooltip_id) * unit_scale",
        "kTooltipReferenceHeight * static_cast<double>(kConfirmationAtlasTiles) * unit_scale",
        "ScalarParameters width{static_cast<float>(kTooltipReferenceWidth * unit_scale)};",
        "ScalarParameters height{static_cast<float>(kTooltipReferenceHeight * unit_scale)};",
        "control->ProcessEvent(set_tool_tip_, &tooltip);", "record.content = size_box;", "record.image = image;",
    ):
        require(create, snippet, "34 tooltip clips use the full 55-tile atlas height")
    refresh = method(hub, "refresh_tooltips_unsafe")
    atlas_codes = re.search(r"atlas_codes\{\{(.*?)\}\};", refresh, re.S)
    if atlas_codes is None or tuple(re.findall(r'L"([^"]+)"', atlas_codes[1])) != EXPECTED_LANGUAGES:
        fail("native atlas filenames must follow all 11 resolved languages including AUTO")
    for snippet in (
        "if (tooltip_atlas_language_ != resolved_ui_language_)",
        "if (tooltip_widget_abi_available_ && !tooltip_atlas_attempted_)",
        "tooltip_atlas_attempted_ = true;", "tooltip_atlas_ = FWeakObjectPtr{};",
        "if (texture && content && image && apply_text_overlay_unsafe(image, texture))",
        "control->ProcessEvent(set_tool_tip_, &clear);",
        "localized.tooltips[record.id]",
    ):
        require(refresh, snippet, "language-edge import and failed-asset native fallback")
    reset = method(hub, "reset_runtime_handles")
    for snippet in ("for (auto& tooltip : tooltips_) tooltip = TooltipRecord{};", "tooltip_count_ = 0;",
                    "tooltip_atlas_ = FWeakObjectPtr{};", "tooltip_atlas_attempted_ = false;"):
        require(reset, snippet, "tooltip lifecycle reset")
    tick = method(hub, "service_unsafe")
    if "refresh_tooltips_unsafe(" in tick or "import_text_overlay_unsafe(" in tick:
        fail("tooltip resource work must not be added to every tick")
    for tooltip_id in EXPECTED_TIPS:
        require(hub, f"dswros::RadarTooltipId::{tooltip_id}", "each setting has localized hover help")
    # The old implementation mentioned every generic enum yet bound an entire
    # column to one vague sentence. Prove actual category/option-to-topic paths.
    require(hub, "static_assert(dswros::kRadarTooltipCount == 34U);", "34 exact tooltip topics")
    require(header, "static_assert(58U <= kMaximumTooltipCount);", "58 owners fit the retained pool")
    verify_confirmation_native(hub, header, fail, require, method)
    mapping = re.search(r"marker_tooltip_for\(.*?switch \(category\) \{(.*?)\n    \}", hub, re.S)
    if mapping is None:
        fail("category tooltip lookup is missing")
    pairs = re.findall(r"case RadarVisibilityCategory::(\w+): return dswros::RadarTooltipId::(\w+);", mapping[1])
    expected_categories = ["Treasure", "Boss", "Assault", "MiniGames", "AreaQuests", "BirdEggs", "Clock"]
    if pairs != [(category, category) for category in expected_categories]:
        fail("a category checkbox points to another category's explanation")
    require(mapping[1], "default: return dswros::RadarTooltipId::Count;", "invalid category has no invented tooltip")
    for snippet in (
        "const auto category = kRows[row].category;",
        "const auto topic = marker_tooltip_for(category);",
        "for (std::size_t column = 0; column < 2U; ++column) if (UObject* control = controls[column][category_index]) bind_tip(control, topic);",
        "UObject* label_tooltip_target = add_border(kContentX, kMarkerRowsY + static_cast<double>(row) * kMarkerRowStep, 420.0, kMarkerRowStep, 11, transparent_hover_color);",
        "set_visibility(label_tooltip_target, set_visibility_, kVisible);",
        "bind_tip(label_tooltip_target, topic);",
        "bind_tip(controls[2][static_cast<std::size_t>(scene_categories[index])], scene_tips[index]);",
        "bind_tip(height_controls[index], height_tips[index]);",
        "bind_tip(area_mode_controls[0], dswros::RadarTooltipId::AreaQuestAvailable);",
        "bind_tip(area_mode_controls[1], dswros::RadarTooltipId::AreaQuestAll);",
        "bind_tip(assault_mode_controls[0], dswros::RadarTooltipId::AssaultAvailable);",
        "bind_tip(assault_mode_controls[1], dswros::RadarTooltipId::AssaultAll);",
    ):
        require(hub, snippet, "every specific category, Scene, height and filter control binds its own topic")
    for name, expected in (
        ("scene_tips", ("SceneTreasure", "SceneAreaQuests", "SceneMiniGames")),
        ("height_tips", ("HeightTreasure", "HeightAreaQuests", "HeightMole", "HeightBoss", "HeightAssault")),
        ("distance_tips", ("DistanceOff", "DistanceAim", "DistanceAuto", "DistanceAll")),
    ):
        array = re.search(r"\b" + name + r"\{\{(.*?)\}\};", hub, re.S)
        actual = () if array is None else tuple(re.findall(r"dswros::RadarTooltipId::(\w+)", array[1]))
        if actual != expected:
            fail(f"{name} no longer matches the actual control order")
    nine_slice = method(hub, "configure_chip_nine_slice_unsafe")
    for snippet in (
        "if (!chip_nine_slice_abi_available_ || !image || !image->IsA(image_class_)",
        "|| !std::isfinite(unit_scale) || !(unit_scale > 0.0)) return false;",
        "set_image_brush_value_property_->CopyCompleteValue(target, current);",
        "constexpr std::array<double, 4> margins{{kChipCornerRadius / kChipSkinReferenceWidth, kChipCornerRadius / kChipSkinReferenceHeight, kChipCornerRadius / kChipSkinReferenceWidth, kChipCornerRadius / kChipSkinReferenceHeight}};",
        "kChipSkinReferenceWidth * unit_scale, kChipSkinReferenceHeight * unit_scale",
        "if (!write_font_metric(draw_as, draw_value, 1.0)) return false;",
        "image->ProcessEvent(set_image_brush_, parameters.data());",
        "if (!read_font_metric(metric, address, actual) || !font_metric_matches(actual, expected[index])) return false;",
        "actual_draw_as == 1.0 && texture",
        "read_struct_object_property(image, L\"Brush\", L\"ResourceObject\") == texture",
    ):
        require(nine_slice, snippet, "reflected Box brush keeps scaled ten-unit circular caps and texture")
    # Actual grid hits touch but do not overlap at a 24px row step, including
    # the smallest/largest supported layout scale. Visuals stay square.
    for scale in (0.5, 0.75, 1.0, 1.5, 2.5):
        for row in range(8):
            hit = (144 + row * 24) * scale
            if row < 7 and hit + 24 * scale > (144 + (row + 1) * 24) * scale + 1e-9:
                fail("square check geometry crosses its next row")


def verify_native_status_readability(hub: str, fail) -> float:
    """Protect status text when both texture sheets fail to load.

    The source-verified status row has no extra native reading plate. Its
    background is exactly the frame followed by the inset panel. Check their
    actual constants, rather than mandating a particular opacity or RGB value.
    """
    flat = re.sub(r"\s+", " ", hub)
    for call in (
        "add_border(0.0, 0.0, kReferencePanelWidth, kReferencePanelHeight, 0, kPanelFrame)",
        "add_border(1.0, 1.0, kReferencePanelWidth - 2.0, kReferencePanelHeight - 2.0, 1, kPanelBackground)",
        "add_localized_text(LocalizedTextSlot::StatusLabel, localized.status, 388.0, 59.0, 90.0, 24.0, 7, 0.375)",
        "add_localized_text(LocalizedTextSlot::StatusValue, status_value, 493.0, 59.0, 102.0, 24.0, 7, 0.40625, kTextCenter)",
    ):
        if call not in flat:
            fail("native status reading-surface geometry changed; re-audit its layers")

    def linear(value):
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4

    background = (1.0, 1.0, 1.0)
    for name in ("kPanelFrame", "kPanelBackground"):
        match = re.search(r"constexpr LinearColor " + name + r"\s*\{([^}]+)\}", hub)
        if match is None:
            fail("native status reading-surface palette is missing")
        values = []
        for token in match[1].split(","):
            pieces = token.strip().replace("F", "").split("/")
            if not 1 <= len(pieces) <= 2:
                fail("native status reading-surface palette cannot be parsed")
            value = float(pieces[0]) / (float(pieces[1]) if len(pieces) == 2 else 1.0)
            if not 0.0 <= value <= 1.0:
                fail("native status reading-surface palette is out of range")
            values.append(value)
        if len(values) != 4:
            fail("native status reading-surface palette needs RGBA")
        background = tuple(linear(value) * values[3] + old * (1.0 - values[3])
                           for value, old in zip(values[:3], background))
    weights = (0.2126, 0.7152, 0.0722)
    foreground = sum(w * linear(v / 255.0) for w, v in zip(weights, (247, 253, 255)))
    contrast = (foreground + 0.05) / (sum(w * v for w, v in zip(weights, background)) + 0.05)
    if contrast < 4.5:
        fail("native fallback status text falls below 4.5:1 on linear-light white")
    return contrast


def restrained_blue_gray(pixel) -> bool:
    """Bound intentional tint without accepting either charcoal or saturated blue."""
    red, green, blue = pixel[:3]
    return (14 <= blue - red <= 40 and 5 <= green - red <= 20
            and 5 <= blue - green <= 22 and 20 <= sum(pixel[:3]) / 3 <= 95)


def verify_material_pixels(images: dict[str, Image.Image], fail, main_layout) -> dict:
    """Check final raster behavior independently of generator colors/formulas.

    White and black backdrops expose the *combined* opacity already baked into
    the sheet. Linear-light white is the worst case for the light small text;
    ordinary byte-space Pillow blending would overstate its contrast.
    """
    def linear(value: int) -> float:
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4

    weights = (0.2126, 0.7152, 0.0722)
    text_light = sum(w * linear(v) for w, v in zip(weights, (237, 244, 249)))
    main = images["main-glass.tga"]
    black = Image.alpha_composite(Image.new("RGBA", main.size, (0, 0, 0, 255)), main)
    white = Image.alpha_composite(Image.new("RGBA", main.size, (255, 255, 255, 255)), main)
    transmissions, contrasts = [], []
    # Dense inset samples cover every ordinary card body, including the first
    # and last rows. The sheet should quiet scenery rather than reveal a
    # distracting second layer of game labels under its own text.
    for top, bottom in ((98, 344), (354, 562), (572, 688), (698, 808)):
        for y in range(top + 12, bottom - 12, 3):
            for x in range(52, 710, 37):
                point = (x * 2, y * 2)
                r, g, b, alpha = main.getpixel(point)
                transmission = 1.0 - alpha / 255.0
                if not 0.07 <= transmission <= 0.11:
                    fail("ordinary card loses its controlled 7-11 percent scene transmission")
                delta = tuple(a - b for a, b in zip(white.getpixel(point)[:3], black.getpixel(point)[:3]))
                if any(abs(difference / 255.0 - transmission) > 1.0 / 255 for difference in delta):
                    fail("opaque backdrop compositions disagree with the final sheet alpha")
                if not restrained_blue_gray((r, g, b)) or not 25 <= (r + g + b) / 3 <= 65:
                    fail("ordinary card lost its controlled soft blue-gray tint")
                background_light = sum(w * (linear(v) * (1.0 - transmission) + transmission)
                                       for w, v in zip(weights, (r, g, b)))
                contrast = (text_light + 0.05) / (background_light + 0.05)
                if contrast < 4.5:
                    fail("ordinary card small text falls below 4.5:1 on linear-light white")
                transmissions.append(transmission)
                contrasts.append(contrast)
    gaps = [1.0 - main.getpixel((800, y * 2))[3] / 255.0 for y in (94, 349, 567, 693, 813)]
    if not all(0.08 <= value <= 0.11 for value in gaps):
        fail("gaps must retain 8-11 percent scene transmission")
    # Sample actual straight card edges separately from their interiors. A
    # high-contrast outline would recreate the nested-box appearance even if
    # average card opacity and text contrast still passed.
    for top, bottom in ((98, 344), (354, 562), (572, 688), (698, 808)):
        y = top + (bottom - top) // 2
        edge = main.getpixel((40, y * 2))
        inset = main.getpixel((44, y * 2))
        outside = main.getpixel((32, y * 2))
        if any(abs(a - b) > 3 for a, b in zip(edge, inset)):
            fail("card edge regained a bright rim")
        if any(abs(a - b) > 8 for a, b in zip(inset[:3], outside[:3])):
            fail("card fill is too distinct from its surrounding sheet")
    for language in EXPECTED_LANGUAGES:
        atlas = images[f"tooltip-{language}.tga"]
        for index in range(34):
            for y in (24, 72, 120):
                r, g, b, alpha = atlas.getpixel((16, index * 144 + y))
                if not 0.96 <= alpha / 255.0 <= 0.99 or not restrained_blue_gray((r, g, b)):
                    fail("tooltip lost its more opaque blue-gray reading surface")
    for name in ("popup-glass.tga", "chip-idle.tga", "check-idle.tga"):
        image = images[name]
        points = [(image.width // 2, image.height // 2)] if name != "popup-glass.tga" else [(400, 190), (400, 420)]
        if any(not restrained_blue_gray(image.getpixel(p)) for p in points):
            fail("inactive/popup skins lost their controlled blue-gray tint")
    idle = images["chip-idle.tga"]
    # The same nine-slice also backs confirmation text, so its center and
    # straight edges need a stable reading surface without a translucent rim.
    for point in ((222, 0), (222, 4), (222, 30), (222, 59)):
        if not 0.96 <= idle.getpixel(point)[3] / 255.0 <= 0.99:
            fail("idle/confirmation surface must remain 96-99 percent opaque")
    for name in ("chip-idle.tga", "chip-active.tga", "check-idle.tga", "check-active.tga"):
        image = images[name]
        if image.getpixel((0, 0))[3] != 0:
            fail("control skin lost its transparent rounded corner")
        if image.getpixel((image.width // 2, 0))[3] < 240:
            fail("control skin regained a translucent outline")
    selected = images["chip-active.tga"].getpixel((222, 30))
    neutral = idle.getpixel((222, 30))
    if selected[1] - neutral[1] < 20 or not 35 <= selected[2] - selected[0] <= 70:
        fail("selected controls lost their restrained blue accent")
    text_metrics = verify_text_materials(images, main_layout, fail)
    return {"card_samples": len(transmissions),
            "scene_transmission_min": min(transmissions), "scene_transmission_max": max(transmissions),
            "linear_white_minimum_text_contrast": min(contrasts),
            "gap_scene_transmission_min": min(gaps), "gap_scene_transmission_max": max(gaps),
            **text_metrics}


def verify_text_materials(images, main_layout, fail) -> dict:
    """Conservative white-background probes at the verified native text slots.

    This assembles actual main/idle/active/popup pixel layers in linear light;
    it never calls the skin generator or trusts its intended layer opacity.
    The caller supplies the independently source-verified native 45-slot layout.
    These static bounds do not claim the game's exact font/tonemapping pipeline.
    """
    if len(main_layout) != 45:
        fail("text material checks require the complete native 45-slot inventory")

    def linear(value):
        value /= 255.0
        return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4

    weights = (0.2126, 0.7152, 0.0722)
    primary_light = sum(w * linear(v) for w, v in zip(weights, (237, 244, 249)))
    secondary_light = sum(w * linear(v) for w, v in zip(weights, (206, 220, 232)))
    secondary_slots = {"language", "status", "radar", "map", "radar_only", "scene_distance"}
    main = images["main-glass.tga"]

    def over(background, pixel):
        alpha = pixel[3] / 255.0
        return tuple(linear(v) * alpha + old * (1.0 - alpha)
                     for v, old in zip(pixel[:3], background))

    def skin_pixel(name, rectangle, point):
        x, y, w, h = rectangle
        px, py = point
        if not (x <= px < x + w and y <= py < y + h):
            return None
        # Native Slate Box uses 10-unit corner caps with a 222-by-30 source.
        def axis(value, target, source):
            if value < 10.0:
                return value
            if value >= target - 10.0:
                return source - (target - value)
            return 10.0 + (value - 10.0) * (source - 20.0) / (target - 20.0)
        image = images[name]
        sx = min(image.width - 1, max(0, round(axis(px - x, w, 222.0) * 2)))
        sy = min(image.height - 1, max(0, round(axis(py - y, h, 30.0) * 2)))
        return image.getpixel((sx, sy))

    # The same idle and selected images remain layered at their actual native
    # rectangles. Check every label with selected visuals both off and on.
    controls = [(606, 56, 130, 30, 0)]
    controls += [(36 + 234 * i, 402, 222, 30, 1) for i in range(3)]
    controls += [(36 + 174 * i, 524, 166, 30, 1) for i in range(4)]
    controls += [(36 + 234 * (i % 3), 616 + 34 * (i // 3), 222, 28, 1) for i in range(5)]
    controls += [(36 + 354 * g + 172 * i, 770, 160, 30, 1) for g in range(2) for i in range(2)]
    value_controls = [(632, y, 92, 26) for y in (438, 470)]
    records = []
    for selected in (False, True):
        layers = [("chip-idle.tga", box) for box in value_controls]
        for x, y, w, h, inset in controls:
            if inset:
                layers.append(("chip-idle.tga", (x, y, w, h)))
            if selected or not inset:
                layers.append(("chip-active.tga", (x + inset, y + inset, w - 2 * inset, h - 2 * inset)))
        slots = [(row[0], row[1:5], row[0] == "title") for row in main_layout]
        slots += [("endorse", (278, 831, 206, 24), False)]
        # The centered dynamic value is bounded to six characters ("1000 m")
        # at role .4375. Use its conservative 72-by-18 reading interior inside
        # the 88-by-26 text slot, excluding the decorative corner-edge pixel.
        slots += [(f"slider_value_{i}", (642, y + 4, 72, 18), False) for i, y in enumerate((438, 470))]
        for name, rectangle, large_title in slots:
            text_light = secondary_light if name in secondary_slots else primary_light
            x, y, w, h = rectangle
            contrasts = []
            for px in (x + 2, x + w * 0.25, x + w * 0.5, x + w * 0.75, x + w - 2):
                for py in (y + 3, y + h * 0.5, y + h - 3):
                    rgb = over((1.0, 1.0, 1.0), main.getpixel((round(px * 2), round(py * 2))))
                    for filename, box in layers:
                        pixel = skin_pixel(filename, box, (px, py))
                        if pixel is not None:
                            rgb = over(rgb, pixel)
                    light = sum(wt * v for wt, v in zip(weights, rgb))
                    contrast = (text_light + 0.05) / (light + 0.05)
                    threshold = 3.0 if large_title else 4.5
                    if contrast < threshold:
                        fail(f"actual text slot {name} selected={selected} contrast {contrast:.3f}:1 below {threshold}:1")
                    contrasts.append(contrast)
            records.append({"slot": name, "selected": selected, "large_title": large_title,
                            "minimum_contrast": min(contrasts)})
        popup = images["popup-glass.tga"]
        for index in range(12):
            x, y = 78 + 202 * (index % 3), 101 + 40 * (index // 3)
            contrasts = []
            for px in (x + 7, x + 95, x + 183):
                for py in (y + 7, y + 16, y + 25):
                    point = (px * 2, py * 2)
                    rgb = over(over((1.0, 1.0, 1.0), main.getpixel(point)), popup.getpixel(point))
                    if selected:
                        rgb = over(rgb, skin_pixel("chip-active.tga", (x + 2, y + 2, 186, 28), (px, py)))
                    light = sum(wt * v for wt, v in zip(weights, rgb))
                    contrast = (primary_light + 0.05) / (light + 0.05)
                    if contrast < 4.5:
                        fail(f"actual popup text slot {index} selected={selected} falls below 4.5:1")
                    contrasts.append(contrast)
            records.append({"slot": f"popup_{index}", "selected": selected,
                            "large_title": False, "minimum_contrast": min(contrasts)})
    small = [r["minimum_contrast"] for r in records if not r["large_title"]]
    return {"text_slot_states": len(records), "small_text_minimum_linear_white_contrast": min(small),
            "text_material_checks": records}


def verify_confirmation_native(hub: str, header: str, fail, require, method) -> None:
    """Bind seven text crops to the real modal geometry and guarded fallback."""
    geometry = re.search(r"confirmation_text_geometry\{\{(.*?)\}\};", hub, re.S)
    expected = [(278,831,206,24,.40625,1), (220,350,320,32,.50,1),
                (220,382,320,72,.40625,1), (396,466,144,24,.40625,1), (220,466,144,24,.40625,1)]
    actual = [] if geometry is None else [tuple(float(n) for n in row.split(","))
        for row in re.findall(r"\{\{([^{}]+)\}\}", geometry[1])]
    if actual != expected:
        fail("confirmation atlas crops no longer match the actual five label rectangles/scales")
    for name, expected_value in (("kConfirmationX",190), ("kConfirmationY",334),
                                 ("kConfirmationWidth",380), ("kConfirmationHeight",184)):
        match = re.search(r"constexpr double " + name + r"\s*=\s*([\d.]+)", hub)
        if match is None or float(match[1]) != expected_value:
            fail(f"{name} breaks the validated modal text bounds")
    for snippet in ("constexpr std::size_t kConfirmationAtlasTiles = dswros::kRadarTooltipCount + dswros::kRadarConfirmationTextCount + kNumericGlyphCount;",
                    "static_assert(kConfirmationAtlasTiles == 55U);",
                    "bind_tip(endorsement_control_.Get(), dswros::RadarTooltipId::Endorse);",
                    "kTooltipReferenceHeight * static_cast<double>(kConfirmationAtlasTiles) * record.scale",
                    "canvas->ProcessEvent(set_clipping_, &clipping);"):
        require(hub, snippet, "shared confirmation atlas geometry and Endorse hover binding")
    atlas_validation = re.search(r"bool valid_confirmation_atlas_file\([^)]*\)\s*\{(.*?)\n\}", hub, re.S)
    if atlas_validation is None:
        fail("native confirmation atlas header validation is missing")
    for snippet in ("std::array<std::uint8_t, 18> header{};",
                    "if (!input.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()))) return false;",
                    "return header[0] == 0 && header[1] == 0 && (header[2] == 2 || header[2] == 10)",
                    "&& header[16] == 32 && (header[17] & 0x0FU) == 8",
                    "&& width == 640U && height == kConfirmationAtlasTiles * 144U;"):
        require(atlas_validation[1], snippet, "reject old, truncated or non-RGBA confirmation atlases")
    import_edge = method(hub, "refresh_tooltips_unsafe")
    require(import_edge, "tooltip_atlas_ = valid_confirmation_atlas_file(atlas_path) ? import_text_overlay_unsafe(host_.Get(), atlas_path) : nullptr;",
            "atlas dimensions must pass before binding could enable Yes")
    refresh = method(hub, "refresh_confirmation_text_unsafe")
    for snippet in ("const std::array<Text, 5> ids{{Text::Endorse, Text::Title, body, Text::Yes, Text::No}};",
                    "Action::RestoreDefaults ? Text::RestoreBody",
                    "Action::BugReport ? Text::FeedbackBody : Text::EndorseBody",
                    "tooltip_atlas_language_ == resolved_ui_language_",
                    "dswros::kRadarTooltipCount + static_cast<std::size_t>(ids[index])",
                    "ready[index] = apply_text_overlay_unsafe(image, texture);",
                    "confirmation_text_ready_ = ready[1] && ready[2] && ready[3] && ready[4];",
                    "visible && !packaged ? kHitTestInvisible : kCollapsed"):
        require(refresh, snippet, "all modal strings share verified atlas binding with readable fallback")
    visible = method(hub, "set_confirmation_visibility_unsafe")
    for snippet in ("enable(yes, visible && confirmation_text_ready_);", "enable(no, visible);"):
        require(visible, snippet, "missing confirmation text permits cancellation only")
    reset = method(hub, "reset_runtime_handles")
    require(reset, "for (auto& record : confirmation_texts_) record = ConfirmationTextRecord{};", "modal weak handles cleared")
    require(header, "std::array<ConfirmationTextRecord, 5> confirmation_texts_{};", "bounded shared-atlas consumers")
    # A bright-scene upper bound with the modal drawn over white, deliberately
    # omitting the darker existing panel and dim layers, is conservative.
    surface = re.search(r"constexpr LinearColor confirmation_surface\{(.*?)\};", hub, re.S)
    if surface is None:
        fail("native confirmation reading surface is missing")
    values = surface[1].split(",")
    rgb = [float(re.fullmatch(r"\s*([\d.]+)F / 255\.0F\s*", v)[1]) / 255 for v in values[:3]]
    alpha = float(values[3].strip().removesuffix("F"))
    linear = lambda v: v / 12.92 if v <= .04045 else ((v + .055) / 1.055) ** 2.4
    weights = (.2126,.7152,.0722)
    foreground = sum(w * linear(v / 255) for w,v in zip(weights,(237,244,249)))
    background = sum(w * (linear(v) * alpha + 1 - alpha) for w,v in zip(weights,rgb))
    if (foreground + .05) / (background + .05) < 4.5:
        fail("native modal surface falls below 4.5:1 over a worst-white scene")


def verify_numeric_tiles(root: Path, language: str, manifest: dict, font_path: Path, fail) -> None:
    if (assets.NUMERIC_CHARACTERS != EXPECTED_NUMERIC_CHARACTERS
        or assets.NUMERIC_CROP != (16, 26)
        or assets.NUMERIC_ADVANCES != EXPECTED_NUMERIC_ADVANCES
        or manifest.get("numeric_characters") != EXPECTED_NUMERIC_CHARACTERS
        or manifest.get("numeric_crop") != [16, 26]
        or manifest.get("numeric_advances") != list(EXPECTED_NUMERIC_ADVANCES)):
        fail("numeric atlas character order, crop or fixed advances differ")
    entries = manifest.get("numeric_metrics", {}).get(language)
    if not isinstance(entries, list) or len(entries) != 14:
        fail(f"{language} must supply every digit, space, unit and placeholder")
    actual = Image.open(root / "assets/ui/f6" / f"tooltip-{language}.tga").convert("RGBA")
    for index, character in enumerate(EXPECTED_NUMERIC_CHARACTERS):
        size = 26 if character == "v" else 28
        font = ImageFont.truetype(str(font_path), size)
        expected = Image.new("RGBA", (640, 144))
        bounds = None
        if character != " ":
            left, top, right, bottom = font.getbbox(character, anchor="ls")
            width, height = right - left, bottom - top
            if width > 2 * EXPECTED_NUMERIC_ADVANCES[index] or width > 32 or height > 52:
                fail(f"{language}/{character} cannot fit without shrinking")
            ascent, descent = font.getmetrics()
            baseline = (52 - ascent - descent) // 2 + ascent
            ImageDraw.Draw(expected).text(((32-width)//2-left, baseline),
                character, font=font, anchor="ls", fill=(237, 244, 249, 255))
            bounds = list(expected.getchannel("A").getbbox())
        entry = {"character": character, "crop": [16, 26],
                 "advance": EXPECTED_NUMERIC_ADVANCES[index], "font_pixels": size,
                 "bounds": bounds}
        if entries[index] != entry:
            fail(f"{language}/{character} numeric metrics differ from the actual regular font")
        crop = actual.crop((0, (41+index)*144, 640, (42+index)*144))
        if crop.tobytes() != expected.tobytes():
            fail(f"{language}/{character} numeric pixels lose a glyph, add weight or escape their crop")


def verify_assets(root: Path, manifest: dict, cjk: Path, latin: Path, fail, canonical, main_layout) -> int:
    if assets.LANGUAGES != EXPECTED_LANGUAGES or assets.TOOLTIP_IDS != EXPECTED_TIPS:
        fail("glass generator language/tooltip order changed")
    if assets.TOOLTIP_SIZE != (640, 7920) or assets.SKIN_SIZES != {k:v for k,v in EXPECTED_SIZES.items() if not k.startswith("tooltip-")}:
        fail("glass generator geometry differs from fixed native consumer")
    thai = root / "tools/f6-fonts/NotoSansThai-Variable.ttf"
    expected_thai = "5A1C559BB539583C8A1FD99D1C5B9491E5E14478C9CD2BD0970D5C3096CC9EF8"
    if (assets.THAI_FONT_SHA256 != expected_thai or hashlib.sha256(thai.read_bytes()).hexdigest().upper() != expected_thai
        or manifest.get("thai_font_source") != "tools/f6-fonts/NotoSansThai-Variable.ttf"
        or manifest.get("thai_font_sha256") != expected_thai):
        fail("Thai source-font pin differs")
    license_hash = hashlib.sha256((root / "tools/f6-fonts/LICENSE_NOTO_THAI").read_bytes()).hexdigest().upper()
    if license_hash != "2E98FD23A52D253DB8612CD5942C8F2FF4111B21D2367050FDCA91D8CCC374A0":
        fail("unmodified Thai font license differs")
    values = assets.read_tooltips(root)
    confirmations = assets.read_confirmation_texts(root)
    if assets.CONFIRMATION_IDS != EXPECTED_CONFIRMATIONS or assets.CONFIRMATION_CROPS != EXPECTED_CONFIRMATION_CROPS:
        fail("confirmation text enum or clipped crop differs from native contract")
    confirmation_count = assets.verify_tooltip_glyphs(confirmations, cjk, latin, thai)
    if (manifest.get("confirmation_ids") != list(EXPECTED_CONFIRMATIONS)
        or manifest.get("confirmation_crops") != [list(c) for c in EXPECTED_CONFIRMATION_CROPS]
        or manifest.get("confirmation_localizations_sha256") != canonical(confirmations)
        or manifest.get("confirmation_verified_codepoint_count") != confirmation_count
        or manifest.get("atlas_tile_count") != 55):
        fail("confirmation manifest loses text, glyph coverage, crop or total atlas height")
    count = assets.verify_tooltip_glyphs(values, cjk, latin, thai)
    if (manifest.get("tooltip_ids") != list(EXPECTED_TIPS)
        or manifest.get("tooltip_reference_size") != [320, 72]
        or manifest.get("tooltip_localizations_sha256") != canonical(values)
        or manifest.get("tooltip_verified_codepoint_count") != count
        or manifest.get("glass_generator_sha256") != hashlib.sha256(Path(assets.__file__).read_bytes()).hexdigest().upper()):
        fail("tooltip manifest source, text, glyph or geometry identity differs")
    expected_images = assets.build_skins()
    for language, tips in values.items():
        font_path = assets.tooltip_font(language, cjk, latin, thai)
        atlas, metrics, confirm_metrics = assets.build_tooltip_atlas(tips, font_path, confirmations[language])
        expected_images[f"tooltip-{language}.tga"] = atlas
        if manifest.get("confirmation_metrics", {}).get(language) != confirm_metrics:
            fail(f"{language} confirmation measured layout differs")
        for index, entry in enumerate(confirm_metrics):
            width, height = EXPECTED_CONFIRMATION_CROPS[index]
            if (entry["id"] != EXPECTED_CONFIRMATIONS[index] or entry["font_pixels"] != (32 if index == 1 else 26)
                or not 1 <= len(entry["lines"]) <= (4 if index in (2, 3, 4) else 1)
                or re.sub(r"\s", "", "".join(entry["lines"])) != re.sub(r"\s", "", confirmations[language][index])
                or any(not (8 <= b[0] < b[2] <= width * 2 - 8 and 6 <= b[1] < b[3] <= height * 2 - 6) for b in entry["bounds"])):
                fail(f"{language}/{entry['id']} loses confirmation text or escapes its exact native crop")
            tile = atlas.crop((0, (34 + index) * 144, 640, (35 + index) * 144))
            outside = tile.copy()
            ImageDraw.Draw(outside).rectangle((0, 0, width * 2 - 1, height * 2 - 1), fill=(0, 0, 0, 0))
            if outside.getchannel("A").getbbox() is not None or tile.getpixel((0, 0))[3] != 0:
                fail(f"{language}/{entry['id']} is not a transparent text-only tile inside its crop")
        if manifest.get("tooltip_metrics", {}).get(language) != metrics:
            fail(f"{language} tooltip measured layout differs")
        for index, entry in enumerate(metrics):
            if (entry["id"] != EXPECTED_TIPS[index] or entry["font_pixels"] != 26
                or not 1 <= len(entry["lines"]) <= 4
                or re.sub(r"\s", "", "".join(entry["lines"])) != re.sub(r"\s", "", tips[index])
                or any(not (24 <= b[0] < b[2] <= 616 and 12 <= b[1] < b[3] <= 132) for b in entry["bounds"])):
                fail(f"{language}/{entry['id']} loses source text or escapes its clipping-safe tile")
            original_runs = re.findall(r"[A-Za-z0-9]+", tips[index])
            wrapped_runs = [run for line in entry["lines"] for run in re.findall(r"[A-Za-z0-9]+", line)]
            if original_runs != wrapped_runs:
                fail(f"{language}/{entry['id']} splits an embedded Latin name or number across lines")
            if any(line[0] in "，。；：！？、）》」』”’" for line in entry["lines"]):
                fail(f"{language}/{entry['id']} puts closing punctuation at the start of a line")
        verify_numeric_tiles(root, language, manifest, font_path, fail)
    actual_images = {}
    for filename, expected in expected_images.items():
        actual = Image.open(root / "assets/ui/f6" / filename).convert("RGBA")
        actual_images[filename] = actual
        if actual.size != EXPECTED_SIZES[filename] or actual.tobytes() != expected.tobytes():
            fail(f"{filename} pixels differ from source-derived complete text / deterministic skin")
    verify_material_pixels(actual_images, fail, main_layout)
    return count

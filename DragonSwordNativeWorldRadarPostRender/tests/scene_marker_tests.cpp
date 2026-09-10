#include <dswros/scene_marker_model.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace {
int failures{};
int checks{};
void check(bool value, const char* name) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}
using namespace dswros;
SceneMarker point(std::int64_t id, double meters,
                  SceneMarkerKind kind = SceneMarkerKind::TreasureOther,
                  SceneMiniGameKind minigame_kind = SceneMiniGameKind::None) {
    return {id, kind, {meters * 100.0, 0, 0}, minigame_kind};
}
}

int main() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<SceneMarker> points{point(1, 1), point(2, 600), point(3, 600.01),
        point(4, inf), point(5, nan), point(0, 1), point(-1, 1)};
    auto selected = select_scene_markers({}, points);
    check(selected.count == 2, "range, identity and finite-data filtering");
    check(selected.values[1].marker.id == 2, "600m inclusive edge");
    check(select_scene_markers({nan, 0, 0}, points).count == 0,
          "invalid player fails closed");

    points.clear();
    for (int i = 80; i > 0; --i) points.push_back(point(i, i));
    selected = select_scene_markers({}, points);
    check(selected.count == 24, "default projection capacity");
    check(selected.values.front().marker.id == 1
          && selected.values[selected.count - 1].marker.id == 24, "nearest points survive input order");
    std::reverse(points.begin(), points.end());
    auto reordered = select_scene_markers({}, points);
    for (std::size_t i = 0; i < selected.count; ++i)
        check(reordered.values[i].marker.id == selected.values[i].marker.id,
              "selection order is deterministic");

    points = {point(1, 10), point(1, 9, SceneMarkerKind::TreasurePuzzle),
        point(1, 8, SceneMarkerKind::AreaQuest), point(2, 20)};
    selected = select_scene_markers({}, points);
    check(selected.count == 3, "treasure duplicate does not consume a slot");
    check(selected.values[0].marker.kind == SceneMarkerKind::AreaQuest,
          "quest and treasure ids have distinct namespaces");
    check(selected.values[1].distance_meters == 9, "closer duplicate replaces earlier one");
    points = {point(7, 12, SceneMarkerKind::TreasurePuzzle),
        point(7, 12, SceneMarkerKind::TreasureMiniGame)};
    selected = select_scene_markers({}, points);
    std::reverse(points.begin(), points.end());
    reordered = select_scene_markers({}, points);
    check(selected.count == 1 && reordered.count == 1
          && selected.values[0].marker.kind == SceneMarkerKind::TreasureMiniGame
          && reordered.values[0].marker.kind == selected.values[0].marker.kind,
          "equal-distance conflicting treasure categories resolve deterministically");
    points = {point(1, 10, static_cast<SceneMarkerKind>(255)), point(2, 11)};
    check(select_scene_markers({}, points).count == 1,
          "invalid category does not consume a projection slot");
    points = {point(1, 10), point(2, 9.5)};
    SceneSelection previous{};
    previous.values[0] = {point(1, 10), 10, 10};
    previous.count = 1;
    selected = select_scene_markers({}, points, previous);
    check(selected.values[0].marker.id == 1, "small distance jitter retains incumbent");
    points[1] = point(2, 8);
    selected = select_scene_markers({}, points, previous);
    check(selected.values[0].marker.id == 2, "materially closer point can replace incumbent");
    points = {{7, SceneMarkerKind::AreaQuest, {100, 200, -12345}}};
    selected = select_scene_markers({}, points);
    check(selected.values[0].marker.position.z == -12345,
          "quest navigation anchor height is never invented or rewritten");

    points.clear();
    for (int i = 2647; i > 0; --i) points.push_back(point(i, i / 10.0));
    selected = select_scene_markers({}, points);
    check(selected.count == 24 && selected.values.front().marker.id == 1
          && selected.values[selected.count - 1].marker.id == 24,
          "full high-density catalog remains capped at 24 nearest candidates");
    check(select_scene_markers({}, std::span<const SceneMarker>{points}.first(7)).count == 7,
          "truncated candidate span is respected");
    SceneSelection oversized_previous = selected;
    oversized_previous.count = std::numeric_limits<std::size_t>::max();
    check(select_scene_markers({}, points, oversized_previous).count == 24,
          "malformed previous count is bounded by fixed storage");
    std::array<SceneProjectedPoint, 24> dense_projection{};
    dense_projection.fill({500, 300, true});
    check(layout_scene_markers(selected, dense_projection, 1000, 600).count == 1,
          "dense overlapping projected candidates collapse to one readable glyph");
    selected.count = std::numeric_limits<std::size_t>::max();
    check(layout_scene_markers(selected,
              std::span<const SceneProjectedPoint>{dense_projection}.first(1),
              1000, 600).count == 1,
          "truncated projection span and malformed selection count stay bounded");

    points = {point(1, 10), point(2, 20), point(3, 30), point(4, 40), point(5, 50)};
    selected = select_scene_markers({}, points);
    std::array<SceneProjectedPoint, 5> projected{{
        {500, 300, true}, {510, 310, true}, {700, 350, true},
        {500, 300, false}, {nan, 300, true}}};
    auto frame = layout_scene_markers(selected, projected, 1000, 600);
    check(frame.count == 2, "crowding, rear camera and invalid projection filtered");
    check(frame.values[0].candidate_index == 0, "closer clustered point retained");
    check(SceneDisplaySettings{}.distance_mode == SceneDistanceMode::NearestCenter
          && frame.focus == 0 && frame.values[0].show_distance,
          "default Auto focus immediately shows one visible center-nearest distance");
    frame = layout_scene_markers(selected, projected, 1000, 600, {},
        frame.focus_state, 120);
    check(frame.focus == 0 && frame.values[0].show_distance,
          "default Auto focus retains one center-focused distance label");
    projected[0] = {10, 300, true};
    projected[1] = {1100, 300, true};
    projected[2] = {700, 599, true};
    check(layout_scene_markers(selected, projected, 1000, 600).count == 0,
          "viewport margins prevent clipped glyphs and labels");
    check(layout_scene_markers(selected, projected, inf, 600).count == 0,
          "invalid viewport fails closed");
    check(layout_scene_markers(selected, projected, 1000, nan).count == 0
          && layout_scene_markers(selected, projected, 159, 119).count == 0,
          "NaN and undersized viewports fail closed");
    check(layout_scene_markers(selected, {}, 1000, 600).count == 0,
          "missing projections are safe");
    projected[0] = {100, 100, true};
    frame = layout_scene_markers(selected, projected, 1000, 600);
    check(frame.count == 1 && frame.focus == 0 && frame.values[0].show_distance,
          "default Auto focus can select a peripheral visible marker");
    projected.fill({500, 300, false});
    check(layout_scene_markers(selected, projected, 1000, 600).count == 0,
          "all rear-camera projections remain hidden");
    projected[0] = {inf, 300, true};
    projected[1] = {500, nan, true};
    projected[2] = {500, -inf, true};
    check(layout_scene_markers(selected, projected, 1000, 600).count == 0,
          "nonfinite screen coordinates on either axis remain hidden");
    projected[0] = {52, 52, true};
    projected[1] = {948, 548, true};
    check(layout_scene_markers(selected, projected, 1000, 600).count == 2,
          "exact safe viewport-margin boundaries are included");
    const auto zero = scene_distance_digits(0);
    check(zero[0] == 0 && zero[1] == 0 && zero[2] == 0
          && zero[3] == kSceneDigitSegments[0],
          "zero distance has no leading zeroes");
    const auto far = scene_distance_digits(599.9);
    check(far[0] == 0 && far[1] == kSceneDigitSegments[6]
          && far[2] == kSceneDigitSegments[0]
          && far[3] == kSceneDigitSegments[0], "distance rounds to 600 meters");
    check(scene_distance_digits(nan) == std::array<std::uint8_t, 4>{},
          "nonfinite distance cannot become a glyph index");
    check(scene_distance_digits(inf) == std::array<std::uint8_t, 4>{}
          && scene_distance_digits(-inf) == std::array<std::uint8_t, 4>{}
          && scene_distance_digits(-0.01) == std::array<std::uint8_t, 4>{},
          "infinite and negative distances are rejected");
    check(scene_distance_digits(0.49) == zero
          && scene_distance_digits(0.5)[3] == kSceneDigitSegments[1],
          "submeter rounding boundary");
    const auto ten = scene_distance_digits(9.5);
    check(scene_distance_digits(9.49)[2] == 0
          && ten[0] == 0 && ten[1] == 0 && ten[2] == kSceneDigitSegments[1]
          && ten[3] == kSceneDigitSegments[0], "rounding carries into tens");
    const auto hundred = scene_distance_digits(99.5);
    check(scene_distance_digits(99.49)[1] == 0
          && hundred[0] == 0 && hundred[1] == kSceneDigitSegments[1]
          && hundred[2] == kSceneDigitSegments[0]
          && hundred[3] == kSceneDigitSegments[0], "rounding carries into hundreds");
    const auto thousand = scene_distance_digits(999.5);
    check(thousand[0] == kSceneDigitSegments[1]
          && thousand[1] == kSceneDigitSegments[0]
          && thousand[2] == kSceneDigitSegments[0]
          && thousand[3] == kSceneDigitSegments[0], "1000 meters has four digits");
    check(scene_distance_digits(1000.5) == thousand
          && scene_distance_digits(std::numeric_limits<double>::max()) == thousand,
          "huge finite distances saturate before integer conversion");
    check(std::wstring_view(scene_distance_text(0).data()) == L"0 m"
          && std::wstring_view(scene_distance_text(9).data()) == L"9 m"
          && std::wstring_view(scene_distance_text(10).data()) == L"10 m"
          && std::wstring_view(scene_distance_text(999).data()) == L"999 m"
          && std::wstring_view(scene_distance_text(1000).data()) == L"1000 m"
          && scene_distance_text(1001)[0] == L'\0',
          "cached distance strings have correct digits and unit at every width");

    const SceneDisplaySettings maximum{1000, 50, SceneDistanceMode::All};
    points = {point(1, 0), point(2, 600), point(3, 1000), point(4, 1000.01)};
    selected = select_scene_markers({}, points, {}, maximum);
    check(selected.count == 3 && selected.values[2].marker.id == 3,
          "configurable range includes exactly 1000 meters and coincident targets");
    check(select_scene_markers({}, points, {}, {0, 50, SceneDistanceMode::All}).count == 0,
          "zero range hides even a coincident marker");
    check(select_scene_markers({}, points, {}, {1000, 0, SceneDistanceMode::All}).count == 0,
          "zero limit hides all markers");
    check(select_scene_markers({}, points, {}, {1, 50, SceneDistanceMode::All}).count == 1,
          "small configured range is honored");
    const auto clamped = normalize_scene_display_settings({65535, 255,
        static_cast<SceneDistanceMode>(255)});
    check(clamped.range_meters == 1000 && clamped.marker_limit == 50
          && clamped.distance_mode == SceneDistanceMode::NearestCenter,
          "invalid settings normalize to bounded supported values");
    points.clear();
    for (int i = 2647; i > 0; --i) points.push_back(point(i, i / 10.0));
    selected = select_scene_markers({}, points, {}, maximum);
    check(selected.count == kSceneMarkerCapacity
          && selected.values.front().marker.id == 1
          && selected.values.back().marker.id == 50,
          "high-density selection honors configurable maximum of 50");
    const auto selected_seven = select_scene_markers({}, points, selected,
        {1000, 7, SceneDistanceMode::Off});
    check(selected_seven.count == 7 && selected_seven.values[6].marker.id == 7,
          "lowering capacity immediately removes excess incumbents");
    points.clear();
    for (int i = 40; i > 0; --i) {
        points.push_back(point(i, i, SceneMarkerKind::TreasureMap));
        points.push_back(point(i, i, SceneMarkerKind::AreaQuest));
        points.push_back(point(i, i, SceneMarkerKind::MiniGame));
    }
    selected = select_scene_markers({}, points, {}, maximum);
    std::array<std::size_t, 3> namespace_counts{};
    for (std::size_t i = 0; i < selected.count; ++i)
        ++namespace_counts[scene_identity_namespace(selected.values[i].marker.kind)];
    check(selected.count == 50 && namespace_counts[0] > 0
          && namespace_counts[1] > 0 && namespace_counts[2] > 0,
          "all enabled categories share one 50-marker limit without namespace collisions");
    std::array<SceneProjectedPoint, kSceneMarkerCapacity> grid{};
    for (std::size_t i = 0; i < grid.size(); ++i)
        grid[i] = {100.0 + static_cast<double>(i % 10) * 80.0,
                   100.0 + static_cast<double>(i / 10) * 80.0, true};
    auto many_frame = layout_scene_markers(selected, grid, 1000, 600, maximum);
    const auto distance_count = [](const SceneFrame& candidate_frame) {
        std::size_t count{};
        for (std::size_t i = 0; i < candidate_frame.count; ++i)
            count += candidate_frame.values[i].show_distance ? 1U : 0U;
        return count;
    };
    check(many_frame.count == 50 && distance_count(many_frame) == 50,
          "All mode labels every actually visible marker up to 50");
    grid[0].in_front = false;
    grid[1] = {nan, 200, true};
    many_frame = layout_scene_markers(selected, grid, 1000, 600, maximum);
    check(many_frame.count == 48 && distance_count(many_frame) == 48,
          "All mode does not retain labels for rejected projections");
    many_frame = layout_scene_markers(selected, grid, 1000, 600,
        {1000, 50, SceneDistanceMode::Off});
    check(many_frame.count == 48 && distance_count(many_frame) == 0
          && many_frame.focus_identity.id == 0,
          "distance Off preserves markers while clearing labels and focus");
    check(layout_scene_markers(selected, grid, 1000, 600,
        {0, 50, SceneDistanceMode::All}).count == 0,
          "layout independently respects zero range");

    // Display lifting and label corrections never feed back into source/range.
    const std::array<SceneMarker, 8> display_markers{{
        point(1, 1000, SceneMarkerKind::TreasureOther),
        point(2, 1000, SceneMarkerKind::TreasureMiniGame),
        point(3, 1000, SceneMarkerKind::TreasureMap),
        point(4, 1000, SceneMarkerKind::TreasurePuzzle),
        point(5, 1000, SceneMarkerKind::AreaQuest),
        point(6, 1000, SceneMarkerKind::MiniGame, SceneMiniGameKind::Fly),
        point(7, 1000, SceneMarkerKind::MiniGame, SceneMiniGameKind::Mole),
        point(8, 1000, SceneMarkerKind::MiniGame, SceneMiniGameKind::Wave),
    }};
    const std::array<double, 8> lifts{160, 160, 160, 160, 180, 150, 150, 150};
    const std::array<double, 8> offsets{1, 1, 1, 1, 1, 0, 2, 0};
    selected = select_scene_markers({}, display_markers, {}, maximum);
    check(selected.count == 8, "display lift does not exclude raw 1000m range edge");
    for (std::size_t i = 0; i < display_markers.size(); ++i) {
        const auto& marker = display_markers[i];
        const auto lifted = scene_display_position(marker);
        check(lifted.x == marker.position.x && lifted.y == marker.position.y
              && lifted.z == marker.position.z + lifts[i]
              && marker.position.z == 0,
              "display-only category lift preserves original XYZ");
        check(scene_distance_offset_meters(marker) == offsets[i],
              "each category has its single total display-distance correction");
        for (const double raw : {-2.0, 0.0, 0.49, 0.5, 0.99, 1.0, 1.49,
                                 1.5, 1.99, 2.0, 2.49, 2.5, 999.5, 1000.0}) {
            const SceneCandidate candidate{marker, raw, raw};
            const auto adjusted = scene_display_distance(candidate);
            check(adjusted == static_cast<std::uint16_t>(
                      std::round(std::max(0.0, raw - offsets[i])))
                  && candidate.distance_meters == raw && candidate.rank == raw
                  && candidate.marker.position.z == 0,
                  "distance subtracts once then clamps then rounds without changing data");
            if (raw <= offsets[i])
                check(std::wstring_view(scene_distance_text(adjusted).data()) == L"0 m",
                      "zero or negative adjusted distance always renders 0m");
        }
    }
    for (std::size_t i = 0; i < selected.count; ++i)
        check(selected.values[i].distance_meters == 1000.0
              && selected.values[i].marker.position.z == 0,
              "selection retains original distance and catalog height after lifting");
    check(scene_display_distance({display_markers[0], nan, 0}) == 1001
          && scene_display_distance({display_markers[0], inf, 0}) == 1001,
          "nonfinite display distances remain hidden");
    check(scene_display_distance({display_markers[0],
              std::numeric_limits<double>::max(), 0}) == 1000,
          "huge finite adjusted distance saturates before integer conversion");
    check(scene_display_distance({display_markers[6], 2.5, 0}) == 1
          && scene_display_distance({display_markers[1], 2.5, 0}) == 2,
          "Mole uses total 2m correction while its reward chest uses 1m");
    const std::array<SceneMarker, 2> outside_raw_range{{
        point(9, 600.5, SceneMarkerKind::AreaQuest),
        point(10, 601.5, SceneMarkerKind::MiniGame, SceneMiniGameKind::Mole),
    }};
    check(select_scene_markers({}, outside_raw_range).count == 0,
          "display-distance correction never pulls out-of-range raw positions into selection");

    points = {point(1, 20), point(2, 30)};
    selected = select_scene_markers({}, points);
    std::array<SceneProjectedPoint, 2> center_points{{
        {540, 300, true}, {450, 300, true}}};
    const SceneDisplaySettings central{600, 24, SceneDistanceMode::CentralRadius};
    const SceneDisplaySettings nearest{600, 24, SceneDistanceMode::NearestCenter};
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, {}, 1000);
    check(frame.focus_identity.id == 0 && frame.focus_state.identity.id == 1
          && distance_count(frame) == 0, "Aim begins same-identity dwell without text");
    auto pending = frame.focus_state;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, pending, 1099);
    check(distance_count(frame) == 0, "Aim stays hidden at 99ms");
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, pending, 1100);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "Aim reveals exactly one label at 100ms");
    const auto mature = frame.focus_state;
    auto swapped = selected;
    std::swap(swapped.values[0], swapped.values[1]);
    std::swap(center_points[0], center_points[1]);
    frame = layout_scene_markers(swapped, center_points, 1000, 600, central, mature, 1121);
    check(frame.focus_identity.id == 1 && frame.focus == 1,
          "Aim dwell follows identity across projection slot reordering");
    std::swap(center_points[0], center_points[1]);
    center_points[0].x = 550;
    center_points[1].x = 458;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, mature, 1200);
    check(frame.focus_identity.id == 1 && frame.focus_state.challenger.id == 0,
          "Aim keeps its label when the challenger has insufficient proportional advantage");
    center_points[1].x = 475;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, mature, 1200);
    check(frame.focus_identity.id == 1 && frame.focus_state.challenger.id == 2
          && distance_count(frame) == 1,
          "Aim keeps the current label while a clearly nearer challenger settles");
    pending = frame.focus_state;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, pending, 1549);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "Aim retains exactly the old label at 349ms of replacement dwell");
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, pending, 1550);
    check(frame.focus_identity.id == 2 && distance_count(frame) == 1
          && frame.focus_state.challenger.id == 0,
          "Aim replaces the label at 350ms without an intervening blank frame");
    center_points[0].in_front = false;
    center_points[1].x = 404;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
        frame.focus_state, 1551);
    check(frame.focus_identity.id == 2 && distance_count(frame) == 1,
          "mature Aim focus survives movement through its original ellipse edge");
    center_points[1].x = 384.8;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
        frame.focus_state, 1552);
    check(frame.count == 1 && frame.focus_state.identity.id == 0
          && distance_count(frame) == 0,
          "Aim exact expanded horizontal ellipse edge clears focus and text");
    center_points[1].x = 404.01;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
        frame.focus_state, 1553);
    check(frame.focus_state.identity.id == 2 && distance_count(frame) == 0,
          "Aim reentry just inside radius requires a new dwell");
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
        frame.focus_state, 1673);
    check(frame.focus_identity.id == 2, "Aim reentry can finish the new dwell");
    center_points[1].in_front = false;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
        frame.focus_state, 1674);
    check(frame.count == 0 && frame.focus_state.identity.id == 0,
          "Aim view loss clears pending and mature focus without grace");

    center_points = {{{585, 300, true}, {400, 300, true}}};
    const SceneFocusState auto_incumbent{{2, SceneMarkerKind::TreasureOther}, 0,
                                        SceneDistanceMode::NearestCenter, true};
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest,
        auto_incumbent, 2000);
    check(frame.focus_identity.id == 2 && distance_count(frame) == 1,
          "Auto retains visible incumbent with only fifteen percent improvement");
    center_points[0].x = 560;
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest,
        auto_incumbent, 2001);
    check(frame.focus_identity.id == 2 && frame.focus_state.challenger.id == 1
          && distance_count(frame) == 1,
          "Auto begins a challenger timer while retaining its current label");
    const auto auto_pending = frame.focus_state;
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest,
        auto_pending, 2500);
    check(frame.focus_identity.id == 2 && distance_count(frame) == 1,
          "Auto retains exactly the old label at 499ms of replacement dwell");
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest,
        auto_pending, 2501);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1
          && frame.focus_state.challenger.id == 0,
          "Auto replaces the label at 500ms and clears its challenger");
    center_points[1].in_front = false;
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest,
        auto_incumbent, 2502);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "Auto never retains an invisible incumbent");
    center_points[0] = {100, 100, true};
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "Auto can label a peripheral visible marker without Aim dwell");

    center_points[0] = {500, 300, true};
    const SceneFocusState old_auto{{1, SceneMarkerKind::TreasureOther}, 0,
                                  SceneDistanceMode::NearestCenter, true};
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, old_auto, 3000);
    check(distance_count(frame) == 0 && frame.focus_state.since_ms == 3000,
          "switching Auto to Aim starts a fresh dwell");
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest,
        frame.focus_state, 3001);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "switching pending Aim to Auto reveals the visible focus immediately");
    frame = layout_scene_markers(selected, center_points, 1000, 600,
        {600, 24, SceneDistanceMode::Off}, frame.focus_state, 3002);
    check(frame.count == 1 && distance_count(frame) == 0
          && frame.focus_state.identity.id == 0, "Off clears both focus states and keeps icons");
    frame = layout_scene_markers(selected, center_points, 1000, 600, maximum,
        mature, 3003);
    check(frame.count == 1 && distance_count(frame) == 1
          && frame.focus_state.identity.id == 0, "All labels visible icons without retaining dwell");
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, mature, 999);
    check(distance_count(frame) == 0 && frame.focus_state.since_ms == 999,
          "backward monotonic input restarts dwell without unsigned underflow");
    auto different_namespace = selected;
    different_namespace.values[0].marker.kind = SceneMarkerKind::AreaQuest;
    frame = layout_scene_markers(different_namespace, center_points, 1000, 600,
        central, mature, 4000);
    check(distance_count(frame) == 0 && frame.focus_state.since_ms == 4000,
          "equal numeric id in a different category namespace starts a new Aim dwell");
    auto same_treasure = selected;
    same_treasure.values[0].marker.kind = SceneMarkerKind::TreasurePuzzle;
    frame = layout_scene_markers(same_treasure, center_points, 1000, 600,
        central, mature, 4001);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "same chest identity keeps dwell across a treasure subtype change");
    center_points[1] = {510, 300, true};
    frame = layout_scene_markers(selected, center_points, 1000, 600,
        nearest, auto_incumbent, 4002);
    check(frame.count == 1 && frame.focus_identity.id == 1,
          "Auto immediately retires an incumbent hidden by overlap filtering");
    center_points = {{{395.99, 500, true}, {100, 500, false}}};
    frame = layout_scene_markers(selected, center_points, 600, 1000, central, {}, 5000);
    check(frame.focus_state.identity.id == 1 && distance_count(frame) == 0,
          "portrait Aim uses 16 percent of the shorter viewport side horizontally");
    frame = layout_scene_markers(selected, center_points, 600, 1000, central,
        frame.focus_state, 5120);
    check(frame.focus_identity.id == 1, "portrait Aim dwell can complete inside its radius");
    center_points[0].x = 396;
    frame = layout_scene_markers(selected, center_points, 600, 1000, central,
        frame.focus_state, 5121);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "portrait mature Aim retains its label at the original radius edge");
    center_points[0] = {2092.79, 540, true};
    frame = layout_scene_markers(selected, center_points, 3840, 1080, central, {}, 6000);
    check(frame.focus_state.identity.id == 1,
          "ultrawide Aim radius depends on height rather than wide screen width");
    center_points[0].x = 2092.8;
    frame = layout_scene_markers(selected, center_points, 3840, 1080, central,
        frame.focus_state, 6120);
    check(frame.focus_state.identity.id == 0,
          "ultrawide radius boundary cannot complete an obsolete Aim dwell");
    // The new aiming region deliberately accepts larger vertical displacement
    // while still excluding the ellipse's corners and keeping Auto unchanged.
    center_points = {{{575, 300, true}, {500, 300, false}}};
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, {}, 7000);
    check(frame.focus_state.identity.id == 1,
          "Aim admits 75px horizontal offset formerly outside 60px circle");
    center_points[0] = {500, 490, true};
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, {}, 7001);
    check(frame.focus_state.identity.id == 1 && distance_count(frame) == 0,
          "Aim admits 190px vertical offset and still begins its dwell");
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
                                 frame.focus_state, 7121);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "vertically displaced Aim target becomes visible after dwell");
    center_points[0].y = 504;
    frame = layout_scene_markers(selected, center_points, 1000, 600, central,
                                 frame.focus_state, 7122);
    check(frame.focus_identity.id == 1 && distance_count(frame) == 1,
          "mature Aim retains its label at the original vertical ellipse boundary");
    center_points[0] = {585, 480, true};
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, {}, 7200);
    check(frame.count == 1 && frame.focus_state.identity.id == 0,
          "Aim excludes ellipse corners despite each axis individually fitting");
    center_points = {{{500, 460, true}, {580, 300, true}}};
    frame = layout_scene_markers(selected, center_points, 1000, 600, central, {}, 7300);
    check(frame.focus_state.identity.id == 1,
          "Aim normalized distance is more tolerant of height than horizontal offset");
    frame = layout_scene_markers(selected, center_points, 1000, 600, nearest, {}, 7300);
    check(frame.focus_identity.id == 2,
          "Auto retains ordinary circular center ranking independent from Aim ellipse");
    for (const auto viewport : std::array<std::pair<double, double>, 3>{{
             {1920, 1080}, {3840, 1080}, {1080, 1920}}}) {
        const double side = std::min(viewport.first, viewport.second);
        const double cx = viewport.first * 0.5;
        const double cy = viewport.second * 0.5;
        for (const double sign : {-1.0, 1.0}) {
            center_points = {{{cx, cy + sign * side * 0.33, true}, {0, 0, false}}};
            frame = layout_scene_markers(selected, center_points, viewport.first,
                viewport.second, central, {}, 7400);
            check(frame.focus_state.identity.id == 1,
                  "portrait and ultrawide retain symmetric tall Aim acceptance");
            center_points[0] = {cx + sign * side * 0.17, cy, true};
            frame = layout_scene_markers(selected, center_points, viewport.first,
                viewport.second, central, {}, 7400);
            check(frame.focus_state.identity.id == 0,
                  "horizontal Aim remains bounded on both sides and aspect ratios");
        }
    }
    // Replacement timers follow a continuously better identity, not a slot,
    // and never hide the still-visible distance while another target settles.
    const std::array<SceneMarker, 3> switch_markers{{
        point(1, 20), point(2, 30), point(3, 40)}};
    const auto switch_selection = select_scene_markers({}, switch_markers);
    for (const auto settings : {central, nearest}) {
        const bool aim = settings.distance_mode == SceneDistanceMode::CentralRadius;
        const std::uint64_t delay = aim ? 350 : 500;
        const SceneFocusState held{{1, SceneMarkerKind::TreasureOther}, 9000,
            settings.distance_mode, true, {}, 0, 9900};
        std::array<SceneProjectedPoint, 3> targets{{
            {580, 300, true}, {480, 300, true}, {500, 500, true}}};
        auto small_advantage = targets;
        small_advantage[0] = {aim ? 528.8 : 534.0, 300, true};
        small_advantage[1] = aim ? SceneProjectedPoint{500, 252.06, true}
            : SceneProjectedPoint{473, 300, true};
        small_advantage[2].in_front = false;
        const auto protected_label = layout_scene_markers(switch_selection,
            small_advantage, 1000, 600, settings, held, 10000);
        check(protected_label.count == 2 && protected_label.focus_identity.id == 1
              && protected_label.focus_state.challenger.id == 0,
              "proportional advantage alone cannot defeat the minimum spatial buffer");
        auto stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, held, 10000);
        check(stable.focus_identity.id == 1 && stable.focus_state.challenger.id == 2
              && distance_count(stable) == 1,
              "replacement starts with exactly one incumbent distance in either mode");
        targets[1].x = 430;
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 10100);
        check(stable.focus_identity.id == 1 && stable.focus_state.challenger.id == 0,
              "losing the required advantage immediately cancels replacement dwell");
        targets[1].x = 480;
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 10200);
        check(stable.focus_state.challenger_since_ms == 10200,
              "regained advantage starts a fresh timer rather than accumulating time");
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 10200 + delay - 1);
        check(stable.focus_identity.id == 1 && distance_count(stable) == 1,
              "interrupted challenger cannot replace the label before its new full dwell");
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 10200 + delay);
        check(stable.focus_identity.id == 2 && distance_count(stable) == 1,
              "recovered challenger can replace the label after continuous full dwell");

        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, held, 11000);
        targets[1].x = 420;
        targets[2].y = 260;
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 11100);
        check(stable.focus_identity.id == 1 && stable.focus_state.challenger.id == 3
              && stable.focus_state.challenger_since_ms == 11100,
              "a different winning challenger starts its own identity timer");
        targets[1].x = 480;
        targets[2].y = 500;
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 11200);
        check(stable.focus_identity.id == 1 && stable.focus_state.challenger.id == 2
              && stable.focus_state.challenger_since_ms == 11200,
              "B to C to B camera sweeps cannot reuse B's earlier elapsed dwell");
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 11200 + delay - 1);
        check(stable.focus_identity.id == 1 && distance_count(stable) == 1,
              "alternating challengers preserve the old distance until the final dwell finishes");

        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, held, 12000);
        const auto replacement = stable.focus_state;
        auto reordered_switches = switch_selection;
        auto reordered_targets = targets;
        std::swap(reordered_switches.values[0], reordered_switches.values[1]);
        std::swap(reordered_targets[0], reordered_targets[1]);
        stable = layout_scene_markers(reordered_switches, reordered_targets, 1000, 600,
            settings, replacement, 12000 + delay);
        check(stable.focus_identity.id == 2 && stable.focus == 0
              && distance_count(stable) == 1,
              "replacement timer follows identity through candidate and projection reordering");
        auto recategorized = switch_selection;
        recategorized.values[1].marker.kind = SceneMarkerKind::AreaQuest;
        stable = layout_scene_markers(recategorized, targets, 1000, 600,
            settings, replacement, 12000 + delay);
        check(stable.focus_identity.id == 1
              && stable.focus_state.challenger.kind == SceneMarkerKind::AreaQuest
              && stable.focus_state.challenger_since_ms == 12000 + delay,
              "same numeric challenger id in a different namespace cannot inherit its timer");
        recategorized.values[1].marker.kind = SceneMarkerKind::TreasurePuzzle;
        stable = layout_scene_markers(recategorized, targets, 1000, 600,
            settings, replacement, 12000 + delay);
        check(stable.focus_identity.id == 2
              && stable.focus_identity.kind == SceneMarkerKind::TreasurePuzzle,
              "treasure subtype changes retain the same challenger identity timer");
        targets[1].in_front = false;
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, replacement, 12100);
        check(stable.focus_identity.id == 1 && stable.focus_state.challenger.id == 0,
              "a challenger leaving the displayed set cancels its pending timer");
        targets[1].in_front = true;

        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, replacement, 12300);
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, stable.focus_state, 12200);
        check(stable.focus_state.challenger.id == 0
              && stable.focus_state.since_ms == 12200
              && (aim ? distance_count(stable) == 0 : stable.focus_identity.id == 2),
              "clock rollback after acquisition clears obsolete held and challenger timers");
        for (const auto mode : {SceneDistanceMode::Off, SceneDistanceMode::All}) {
            stable = layout_scene_markers(switch_selection, targets, 1000, 600,
                {600, 24, mode}, replacement, 12500);
            check(stable.focus_state.identity.id == 0
                  && stable.focus_state.challenger.id == 0
                  && distance_count(stable) == (mode == SceneDistanceMode::All ? 3 : 0),
                  "Off and All clear an active replacement timer and apply their label policy");
        }
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            aim ? nearest : central, replacement, 12500);
        check(stable.focus_state.challenger.id == 0
              && (aim ? stable.focus_identity.id == 2 : distance_count(stable) == 0),
              "changing between Aim and Auto discards an active replacement timer");

        // A retained label always belongs to the actual visible set. Its
        // disappearance must not freeze text over a stale or crowded anchor.
        targets[0].in_front = false;
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, replacement, 12100);
        check(stable.focus_state.identity.id == 2
              && stable.focus_state.challenger.id == 0
              && (aim ? distance_count(stable) == 0 : stable.focus_identity.id == 2),
              "lost incumbent reacquires visible targets using the mode's initial policy");
        targets[0] = {20, 300, true};
        stable = layout_scene_markers(switch_selection, targets, 1000, 600,
            settings, replacement, 12100);
        check(stable.focus_state.identity.id == 2
              && (aim ? distance_count(stable) == 0 : stable.focus_identity.id == 2),
              "offscreen incumbents cannot survive a pending replacement");
        targets[0] = {580, 300, true};
        auto clipped = switch_selection;
        clipped.values[0].distance_meters = 601;
        stable = layout_scene_markers(clipped, targets, 1000, 600,
            settings, replacement, 12100);
        check(stable.focus_state.identity.id == 2,
              "range filtering cannot retain a stale incumbent distance");
        stable = layout_scene_markers(reordered_switches, reordered_targets, 1000, 600,
            {600, 1, settings.distance_mode}, replacement, 12100);
        check(stable.count == 1 && stable.focus_state.identity.id == 2
              && (aim ? distance_count(stable) == 0 : stable.focus_identity.id == 2),
              "reducing the shared limit retires an incumbent removed from the displayed set");
        reordered_targets[1] = {490, 300, true};
        stable = layout_scene_markers(reordered_switches, reordered_targets, 1000, 600,
            settings, replacement, 12100);
        check(stable.count == 2 && stable.focus_state.identity.id == 2
              && (aim ? distance_count(stable) == 0 : stable.focus_identity.id == 2),
              "overlap filtering retires an incumbent even during replacement dwell");
        for (const std::uint64_t fps : {30, 60, 144}) {
            auto timed = held;
            bool sweeps_stable = true;
            targets[2].in_front = false;
            for (std::uint64_t sample = 0; sample < fps; ++sample) {
                const auto elapsed = sample * 1000 / fps;
                targets[1].x = (elapsed / 100) % 2 == 0 ? 480.0 : 420.0;
                stable = layout_scene_markers(switch_selection, targets, 1000, 600,
                    settings, timed, 16000 + elapsed);
                timed = stable.focus_state;
                sweeps_stable = sweeps_stable && stable.focus_identity.id == 1
                    && distance_count(stable) == 1;
            }
            check(sweeps_stable,
                  "100ms camera sweeps preserve one label at 30, 60 and 144 FPS");
            targets[1].x = 480;
            stable = layout_scene_markers(switch_selection, targets, 1000, 600,
                settings, timed, 17000);
            timed = stable.focus_state;
            bool timing_correct = stable.focus_identity.id == 1;
            unsigned replacements = 0;
            std::int64_t last_identity = stable.focus_identity.id;
            for (std::uint64_t sample = 1; sample <= fps; ++sample) {
                const auto elapsed = sample * 1000 / fps;
                stable = layout_scene_markers(switch_selection, targets, 1000, 600,
                    settings, timed, 17000 + elapsed);
                timed = stable.focus_state;
                timing_correct = timing_correct && distance_count(stable) == 1
                    && stable.focus_identity.id == (elapsed < delay ? 1 : 2);
                replacements += stable.focus_identity.id != last_identity;
                last_identity = stable.focus_identity.id;
            }
            check(timing_correct && replacements == 1,
                  "sustained focus switches once at elapsed-time threshold independent of FPS");
        }
    }
    std::array<SceneProjectedPoint, 2> outer_targets{{
        {595, 300, true}, {0, 0, false}}};
    auto outer = layout_scene_markers(selected, outer_targets, 1000, 600,
        central, {}, 14000);
    outer_targets[0].x = 605;
    outer = layout_scene_markers(selected, outer_targets, 1000, 600,
        central, outer.focus_state, 14120);
    check(outer.focus_state.identity.id == 0 && distance_count(outer) == 0,
          "initial Aim dwell cannot mature after moving into the incumbent-only outer region");
    outer_targets[0].x = 500;
    outer = layout_scene_markers(selected, outer_targets, 1000, 600,
        central, {}, 15000);
    outer = layout_scene_markers(selected, outer_targets, 1000, 600,
        central, outer.focus_state, 15120);
    outer_targets[0].y = 544.79;
    outer = layout_scene_markers(selected, outer_targets, 1000, 600,
        central, outer.focus_state, 15121);
    check(outer.focus_identity.id == 1 && distance_count(outer) == 1,
          "mature Aim retains its label just inside the expanded vertical boundary");
    outer_targets[0].y = 544.8;
    outer = layout_scene_markers(selected, outer_targets, 1000, 600,
        central, outer.focus_state, 15122);
    check(outer.count == 1 && outer.focus_state.identity.id == 0 && distance_count(outer) == 0,
          "expanded vertical ellipse boundary removes mature text without hiding its icon");

    // A visual frame consumes the already selected list without reordering it
    // when movement changes which target is nearest. Range and displayed
    // distance still use the latest raw player coordinates, never the UI lift.
    std::array<SceneMarker, 4> frame_markers{{point(1, 20), point(2, 10),
        point(3, 900), point(4, 0)}};
    auto live = refresh_scene_frame_selection({500, 0, 0}, frame_markers);
    check(live.count == 3 && live.values[0].marker.id == 1
        && live.values[1].marker.id == 2 && live.values[2].marker.id == 4,
        "visual frames preserve control-list priority while immediately excluding range overflow");
    check(live.values[0].distance_meters == 15 && live.values[1].distance_meters == 5,
        "visual distances follow current player movement without changing catalog anchors");
    live = refresh_scene_frame_selection({}, frame_markers,
        {600, 1, SceneDistanceMode::All});
    check(live.count == 1 && live.values[0].marker.id == 1,
        "visual frame obeys a reduced shared limit immediately");
    check(refresh_scene_frame_selection({}, frame_markers,
        {0, 50, SceneDistanceMode::All}).count == 0,
        "zero range hides cached candidates immediately");
    frame_markers[0].position.x = nan;
    frame_markers[1].id = 0;
    check(refresh_scene_frame_selection({}, frame_markers).count == 1,
        "invalid frame candidates never reach projection");
    // A previously visible marker can move a few pixels through a cutoff
    // without blinking. New arrivals still require the full clearance.
    std::array<SceneMarker, 2> edge_markers{{point(701, 10), point(702, 20)}};
    const auto edge_selection = refresh_scene_frame_selection({}, edge_markers);
    std::array<SceneProjectedPoint, 2> edge_points{{{48, 200, true}, {300, 200, true}}};
    const std::array<SceneFocusIdentity, 1> retained{{{701, SceneMarkerKind::TreasureOther}}};
    check(layout_scene_markers(edge_selection, edge_points, 1280, 720).count == 1,
        "new edge marker requires normal clearance");
    check(layout_scene_markers(edge_selection, edge_points, 1280, 720, {}, {}, 0, retained).count == 2,
        "visible edge marker retains eight-pixel hysteresis");
    edge_points[0].x = 43;
    check(layout_scene_markers(edge_selection, edge_points, 1280, 720, {}, {}, 0, retained).count == 1,
        "edge hysteresis cannot retain offscreen marker");
    edge_points[0] = {300, 200, true}; edge_points[1] = {335, 200, true};
    const std::array<SceneFocusIdentity, 1> cluster_retained{{{702, SceneMarkerKind::TreasureOther}}};
    check(layout_scene_markers(edge_selection, edge_points, 1280, 720).count == 1,
        "new crowded marker needs full separation");
    check(layout_scene_markers(edge_selection, edge_points, 1280, 720, {}, {}, 0, cluster_retained).count == 2,
        "existing crowded marker does not flicker around forty-pixel boundary");
    edge_points[1].in_front = false;
    check(layout_scene_markers(edge_selection, edge_points, 1280, 720, {}, {}, 0, cluster_retained).count == 1,
        "hysteresis never retains markers behind camera");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

#include <dswros/encounter_height.hpp>

#include <array>
#include <iostream>
#include <limits>

namespace {

using Shape = dswros::AreaQuestHeightIndicatorShape;

bool expect(bool passed, const char* message) {
    if (!passed) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return passed;
}

} // namespace

int main() {
    bool passed = true;
    // Raw player Z includes the existing +150 origin offset. Expected states
    // cover both inclusive 5 m edges, just outside them, and repeated ascent
    // and descent across the same spawn point.
    struct HeightSample {
        double player_z;
        Shape expected;
    };
    constexpr std::array<HeightSample, 9> samples{{
        {349.0, Shape::Above},
        {350.0, Shape::Aligned},
        {850.0, Shape::Aligned},
        {1350.0, Shape::Aligned},
        {1351.0, Shape::Below},
        {1400.0, Shape::Below},
        {1350.0, Shape::Aligned},
        {350.0, Shape::Aligned},
        {349.0, Shape::Above},
    }};
    const auto spawn = dswros::encounter_spawn_height_profile(700.0);
    for (const auto& sample : samples) {
        passed &= expect(
            dswros::encounter_height_indicator_shape(
                spawn, dswros::CompactRenderModel::comparable_player_z(
                    sample.player_z)) == sample.expected,
            "encounter spawn guidance must include both dead-zone edges and recover on crossing");
    }

    const auto below_sea_level = dswros::encounter_spawn_height_profile(-2000.0);
    passed &= expect(
        dswros::encounter_height_indicator_shape(below_sea_level, -2500.0)
            == Shape::Aligned
        && dswros::encounter_height_indicator_shape(below_sea_level, -2500.01)
            == Shape::Above
        && dswros::encounter_height_indicator_shape(below_sea_level, -1499.99)
            == Shape::Below,
        "negative spawn heights must use the same world-space boundaries");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (const double invalid_z : {nan, infinity, -infinity}) {
        const auto invalid_spawn = dswros::encounter_spawn_height_profile(invalid_z);
        passed &= expect(
            !dswros::encounter_height_profile_valid(invalid_spawn)
                && dswros::encounter_height_indicator_shape(invalid_spawn, 0.0)
                    == Shape::Unavailable
                && dswros::encounter_height_indicator_shape(spawn, invalid_z)
                    == Shape::Unavailable,
            "invalid source or player height must preserve unknown instead of fabricating alignment");
    }

    dswros::AreaQuestHeightProfile multi_band{};
    multi_band.band_count = 2;
    multi_band.bands[0] = {700.0, 700.0};
    multi_band.bands[1] = {4000.0, 4000.0};
    dswros::AreaQuestHeightProfile broad_band{};
    broad_band.band_count = 1;
    broad_band.bands[0] = {700.0, 2000.0};
    passed &= expect(
        dswros::encounter_height_indicator_shape({}, 0.0) == Shape::Unavailable
            && dswros::encounter_height_indicator_shape(multi_band, 700.0)
                == Shape::Unavailable
            && dswros::encounter_height_indicator_shape(broad_band, 700.0)
                == Shape::Unavailable,
        "encounters must require exactly one static spawn point and reject task-style bands");

    // Z=0 is a legitimate authored spawn height and must stay distinct from a
    // missing source, including when it is directly level with the player.
    passed &= expect(
        dswros::encounter_height_indicator_shape(
            dswros::encounter_spawn_height_profile(0.0), 0.0) == Shape::Aligned,
        "a trusted zero-height spawn must not be mistaken for missing data");

    if (!passed) {
        return 1;
    }
    std::cout << "Encounter height tests passed\n";
    return 0;
}

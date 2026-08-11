using System;
using System.Drawing;

namespace DragonSwordWorldRadar
{
    // Shared visual language for every radar layer. Treasure types keep their
    // semantic fill colors, while treasures and bosses use the same neutral
    // dark outline so markers remain readable without a purple halo.
    internal static class RadarMarkerStyle
    {
        public static readonly Color Outline =
            Color.FromArgb(245, 14, 20, 30);
        public static readonly Color Shadow =
            Color.FromArgb(125, 0, 0, 0);
        public static readonly Color SoftHighlight =
            Color.FromArgb(185, 255, 255, 255);

        public static readonly Color TreasureOther =
            Color.FromArgb(255, 244, 246, 248);
        public static readonly Color TreasureMiniGame =
            Color.FromArgb(255, 75, 222, 115);
        public static readonly Color TreasureMap =
            Color.FromArgb(255, 255, 166, 54);
        public static readonly Color TreasurePuzzle =
            Color.FromArgb(255, 74, 159, 239);

        public static readonly Color BossBacking =
            Color.FromArgb(255, 219, 70, 70);
        public static readonly Color BossInner =
            Color.FromArgb(225, 255, 153, 112);
        public static readonly Color BossIcon =
            Color.FromArgb(255, 255, 248, 235);

        public static readonly Color FlyWing =
            Color.FromArgb(255, 154, 219, 255);
        public static readonly Color FlyArrow =
            Color.FromArgb(255, 105, 199, 255);

        public static Color GetTreasureFill(TreasureKind kind)
        {
            if (kind == TreasureKind.MiniGame)
            {
                return TreasureMiniGame;
            }
            if (kind == TreasureKind.Map)
            {
                return TreasureMap;
            }
            if (kind == TreasureKind.PressurePuzzle
                || kind == TreasureKind.StatuePuzzle)
            {
                return TreasurePuzzle;
            }
            return TreasureOther;
        }

        public static float GetTreasureOutlineWidth(
            float displayScale)
        {
            return Math.Max(1f, 1.45f * displayScale);
        }

        public static float GetBossOutlineWidth(
            float displayScale)
        {
            return Math.Max(2.2f, 3.0f * displayScale);
        }

        public static float GetBossInnerWidth(
            float displayScale)
        {
            return Math.Max(1f, 1.15f * displayScale);
        }

        public static float GetFlyOutlineWidth(float displayScale)
        {
            return Math.Max(1.2f, 1.8f * displayScale);
        }
    }
}

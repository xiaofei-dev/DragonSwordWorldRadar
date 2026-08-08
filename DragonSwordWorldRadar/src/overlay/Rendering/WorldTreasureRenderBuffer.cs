using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;

namespace DragonSwordWorldRadar
{
    internal struct ProjectedWorldTreasure
    {
        public WorldTreasure Treasure;
        public float X;
        public float Y;
    }

    internal sealed class WorldTreasureRenderBuffer : IDisposable
    {
        private readonly List<ProjectedWorldTreasure> _markers =
            new List<ProjectedWorldTreasure>(256);
        private readonly HashSet<long> _projectedPixels =
            new HashSet<long>();

        public readonly GraphicsPath OtherPath =
            new GraphicsPath(FillMode.Winding);
        public readonly GraphicsPath MiniGamePath =
            new GraphicsPath(FillMode.Winding);
        public readonly GraphicsPath MapPath =
            new GraphicsPath(FillMode.Winding);
        public readonly GraphicsPath PuzzlePath =
            new GraphicsPath(FillMode.Winding);

        public int Count
        {
            get { return _markers.Count; }
        }

        public ProjectedWorldTreasure this[int index]
        {
            get { return _markers[index]; }
        }

        public void Reset()
        {
            _markers.Clear();
            _projectedPixels.Clear();
            ResetPath(OtherPath);
            ResetPath(MiniGamePath);
            ResetPath(MapPath);
            ResetPath(PuzzlePath);
        }

        public int Add(
            WorldTreasure treasure,
            float x,
            float y)
        {
            _markers.Add(new ProjectedWorldTreasure
            {
                Treasure = treasure,
                X = x,
                Y = y
            });
            return _markers.Count - 1;
        }

        public bool ReservePixel(long key)
        {
            return _projectedPixels.Add(key);
        }

        public void AddMarker(
            TreasureKind kind,
            RectangleF marker)
        {
            if (kind == TreasureKind.MiniGame)
            {
                MiniGamePath.AddEllipse(marker);
            }
            else if (kind == TreasureKind.Map)
            {
                MapPath.AddEllipse(marker);
            }
            else if (kind == TreasureKind.PressurePuzzle
                || kind == TreasureKind.StatuePuzzle)
            {
                PuzzlePath.AddEllipse(marker);
            }
            else
            {
                OtherPath.AddEllipse(marker);
            }
        }

        public void Dispose()
        {
            OtherPath.Dispose();
            MiniGamePath.Dispose();
            MapPath.Dispose();
            PuzzlePath.Dispose();
        }

        private static void ResetPath(GraphicsPath path)
        {
            path.Reset();
            path.FillMode = FillMode.Winding;
        }
    }
}

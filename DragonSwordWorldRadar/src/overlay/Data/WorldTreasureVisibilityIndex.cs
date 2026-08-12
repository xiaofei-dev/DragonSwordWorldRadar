using System;
using System.Collections.Generic;

namespace DragonSwordWorldRadar
{
    internal sealed class WorldTreasureVisibilityIndex
    {
        private static readonly IList<WorldTreasure> Empty =
            Array.Empty<WorldTreasure>();

        private Dictionary<int, List<WorldTreasure>> _byMap =
            new Dictionary<int, List<WorldTreasure>>();
        private Dictionary<int, List<WorldTreasure>> _stagingByMap =
            new Dictionary<int, List<WorldTreasure>>();
        private readonly Stack<List<WorldTreasure>> _listPool =
            new Stack<List<WorldTreasure>>();
        private int _catalogVersion = -1;
        private int _saveVersion = -1;
        private int _count;
        private int _version;

        public int Count
        {
            get { return _count; }
        }

        public int Version
        {
            get { return _version; }
        }

        public bool Refresh(
            WorldTreasureCatalog catalog,
            TreasureSaveState saveState)
        {
            int catalogVersion = catalog.Version;
            int saveVersion;
            bool hasSave;
            Dictionary<int, ulong> opened;
            saveState.CaptureOpenedState(
                out saveVersion,
                out hasSave,
                out opened);
            if (catalogVersion == _catalogVersion
                && saveVersion == _saveVersion)
            {
                return false;
            }

            int replacementCount = 0;
            RecycleMapLists(_stagingByMap);
            try
            {
                // An unopened treasure cannot be determined until the first
                // SQLCipher snapshot is available. Publish an empty index
                // during that short UNKNOWN phase instead of flashing every
                // catalog point and hiding opened records a few seconds later.
                if (hasSave)
                {
                    IList<WorldTreasure> points = catalog.Points;
                    for (int index = 0;
                        index < points.Count;
                        index++)
                    {
                        WorldTreasure treasure = points[index];
                        if (saveState.IsOpened(
                                treasure.SaveId,
                                opened))
                        {
                            continue;
                        }

                        replacementCount++;
                        List<WorldTreasure> mapPoints;
                        if (!_stagingByMap.TryGetValue(
                                treasure.MapId,
                                out mapPoints))
                        {
                            mapPoints = _listPool.Count == 0
                                ? new List<WorldTreasure>()
                                : _listPool.Pop();
                            _stagingByMap[treasure.MapId] = mapPoints;
                        }
                        mapPoints.Add(treasure);
                    }
                }
            }
            catch
            {
                // Keep the last complete active index if a replacement build
                // fails. The next maintenance pass retries the same versions.
                RecycleMapLists(_stagingByMap);
                throw;
            }

            Dictionary<int, List<WorldTreasure>> previous = _byMap;
            _byMap = _stagingByMap;
            _stagingByMap = previous;
            RecycleMapLists(_stagingByMap);

            // Publish the version pair only after the replacement index is
            // complete and active. Readers never observe a partial rebuild.
            _count = replacementCount;
            _catalogVersion = catalogVersion;
            _saveVersion = saveVersion;
            _version++;
            return true;
        }

        public IList<WorldTreasure> GetMap(int mapId)
        {
            List<WorldTreasure> points;
            return _byMap.TryGetValue(mapId, out points)
                ? points
                : Empty;
        }

        public void Reset()
        {
            _catalogVersion = -1;
            _saveVersion = -1;
            _count = 0;
            _version++;
            RecycleMapLists(_byMap);
            RecycleMapLists(_stagingByMap);
        }

        private void RecycleMapLists(
            IDictionary<int, List<WorldTreasure>> index)
        {
            foreach (List<WorldTreasure> list in index.Values)
            {
                list.Clear();
                _listPool.Push(list);
            }
            index.Clear();
        }
    }
}

using System.Collections.Generic;

namespace DragonSwordWorldRadar
{
    // A Fly activity is considered complete only after its same-ID
    // DT_MiniGame_G5 reward chest has been claimed. This reuses the stable
    // external SQLCipher snapshot and never calls a game UObject.
    internal sealed class MoleRewardVisibilityIndex
    {
        private const int FirstMiniGameId = 11001;
        private const int LastMiniGameId = 11034;
        private int _catalogVersion = -1;
        private int _saveVersion = -1;
        private long _visibleMask;

        public long VisibleMask
        {
            get { return _visibleMask; }
        }

        public bool Refresh(WorldMoleCatalog catalog, TreasureSaveState saveState)
        {
            int catalogVersion = catalog.Version;
            int saveVersion;
            bool hasSave;
            Dictionary<int, ulong> opened;
            saveState.CaptureOpenedState(out saveVersion, out hasSave, out opened);
            if (catalogVersion == _catalogVersion && saveVersion == _saveVersion)
            {
                return false;
            }

            long nextMask = 0;
            if (hasSave)
            {
                foreach (WorldMole mole in catalog.Points)
                {
                    if (mole == null
                        || mole.MiniGameId < FirstMiniGameId
                        || mole.MiniGameId > LastMiniGameId
                        || mole.MaskBit < 0
                        || mole.MaskBit >= 34)
                    {
                        continue;
                    }
                    // Treasure display overrides (notably ignore 11003) must
                    // not alter the raw same-ID Fly completion bit.
                    if (!TreasureSaveState.IsRawOpened(mole.RewardSaveId, opened))
                    {
                        nextMask |= 1L << mole.MaskBit;
                    }
                }
            }

            bool changed = nextMask != _visibleMask;
            _visibleMask = nextMask;
            _catalogVersion = catalogVersion;
            _saveVersion = saveVersion;
            return changed;
        }

        public void Reset()
        {
            _catalogVersion = -1;
            _saveVersion = -1;
            _visibleMask = 0;
        }
    }
}

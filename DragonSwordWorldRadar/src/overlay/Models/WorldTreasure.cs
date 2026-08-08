namespace DragonSwordWorldRadar
{
    internal sealed class WorldTreasure
    {
        private bool _kindResolved;
        private TreasureKind _kind;
        private bool _debugNameResolved;
        private string _debugName;

        public long SaveId { get; set; }
        public int MapId { get; set; }
        public double X { get; set; }
        public double Y { get; set; }
        public double Z { get; set; }
        public bool HasZ { get; set; }
        public string UidName { get; set; }
        public long GroupId { get; set; }

        public TreasureKind Kind
        {
            get
            {
                if (!_kindResolved)
                {
                    _kind = TreasureIdentity.GetKind(UidName);
                    _kindResolved = true;
                }
                return _kind;
            }
        }

        public string DebugName
        {
            get
            {
                if (!_debugNameResolved)
                {
                    _debugName = TreasureIdentity.GetDebugName(
                        UidName,
                        SaveId);
                    _debugNameResolved = true;
                }
                return _debugName;
            }
        }
    }
}

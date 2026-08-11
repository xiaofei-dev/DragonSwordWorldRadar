namespace DragonSwordWorldRadar
{
    internal sealed class WorldMole
    {
        public int MiniGameId { get; set; }
        public long RewardSaveId { get; set; }
        public int MaskBit { get; set; }
        public int MapId { get; set; }
        public double X { get; set; }
        public double Y { get; set; }
        public double Z { get; set; }
        public bool HasZ { get; set; }
        public string UidName { get; set; }
        public string SourceEntry { get; set; }
    }
}

namespace DragonSwordWorldRadar
{
    internal sealed class BossPoint
    {
        public int bossId { get; set; }
        public int mapId { get; set; }
        public double x { get; set; }
        public double y { get; set; }
        public double z { get; set; }
        public bool hasZ { get; set; }
        public double dx { get; set; }
        public double dy { get; set; }
        public bool visible { get; set; }
        public string status { get; set; }
    }

    internal sealed class WorldMapState
    {
        public int mapId { get; set; }
        public double dimensions { get; set; }
        public double uiSize { get; set; }
        public double left { get; set; }
        public double top { get; set; }
        public double zoom { get; set; }
        public double viewportWidth { get; set; }
        public double viewportHeight { get; set; }
        public double viewportScale { get; set; }
        public double playerWorldX { get; set; }
        public double playerWorldY { get; set; }
        public double playerMapX { get; set; }
        public double playerMapY { get; set; }
    }
}

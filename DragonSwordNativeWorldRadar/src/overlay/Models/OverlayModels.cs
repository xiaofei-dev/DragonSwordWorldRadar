namespace DragonSwordWorldRadar
{
    internal enum WorldEncounterKind
    {
        Boss,
        Assault
    }

    internal sealed class WorldEncounterCondition
    {
        public string Type;
        public int VisibleFromHour;
        public int HiddenFromHour;
        public string Provenance;
        public string MissingConfirmation;
    }

    internal sealed class WorldEncounter
    {
        public int Id;
        public int MapId;
        public double X;
        public double Y;
        public double Z;
        public bool HasZ;
        public WorldEncounterKind Kind;
        public BossPoint BossMetadata;
        public readonly System.Collections.Generic.List<WorldEncounterCondition>
            Conditions =
                new System.Collections.Generic.List<WorldEncounterCondition>();
    }

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

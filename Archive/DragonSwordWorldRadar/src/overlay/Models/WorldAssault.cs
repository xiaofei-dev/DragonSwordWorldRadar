namespace DragonSwordWorldRadar
{
    internal sealed class WorldAssault
    {
        public int PlaceId, KindId, Cid, MapId;
        public string Uid, UidName, GameFingerprint, ConditionType, ConditionProvenance, MissingConfirmation;
        public double X, Y, Z;
        public bool HasTimeCondition;
        public int RevealCycleId, VisibleFromHour, HiddenFromHour;
    }
}

namespace DragonSwordNativeAutoPickup.Installer
{
    // Minimal UI contracts for the 1.3.1 single-ABI installer. The historical
    // dual-ABI InstallerEngine.cs is intentionally excluded from release builds.
    internal static class InstallerEngine
    {
        internal enum RangeSelection
        {
            None = 0,
            X3 = 3,
            X5 = 5,
            X10 = 10,
            X15 = 15,
            X20 = 20
        }

        internal enum OptionalRangePakState
        {
            Absent,
            ApprovedX3,
            ApprovedX5,
            ApprovedX10,
            ApprovedX15,
            ApprovedX20,
            MultipleApproved
        }
    }
}

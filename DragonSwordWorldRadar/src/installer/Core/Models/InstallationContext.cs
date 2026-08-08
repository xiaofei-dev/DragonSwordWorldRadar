using System;
using System.IO;

namespace DragonSwordWorldRadar.Installer
{
    public sealed class InstallationContext
    {
        public string ModRoot { get; private set; }
        public string GameRoot { get; private set; }
        public string Win64Root { get; private set; }
        public string ExecutablePath { get; private set; }
        public string PakRoot { get; private set; }
        public string GeneratedDataRoot { get; private set; }
        public string TemporaryRoot { get; private set; }
        public string OodleLibraryPath { get; private set; }

        public InstallationContext(
            string modRoot,
            string gameRoot,
            string oodleLibraryPath)
        {
            ModRoot = Path.GetFullPath(modRoot);
            GameRoot = Path.GetFullPath(gameRoot);
            Win64Root = Path.Combine(GameRoot, "DS", "Binaries", "Win64");
            ExecutablePath = Path.Combine(
                Win64Root,
                "DSClient-Win64-Shipping.exe");
            PakRoot = Path.Combine(GameRoot, "DS", "Content", "Paks");
            GeneratedDataRoot = Path.Combine(ModRoot, "data", "generated");
            TemporaryRoot = Path.Combine(
                Path.GetTempPath(),
                "DragonSwordWorldRadar-Install-" + Guid.NewGuid().ToString("N"));
            OodleLibraryPath = Path.GetFullPath(oodleLibraryPath);
        }
    }
}

using System;
using System.IO;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;

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
        public string GameFingerprint { get; private set; }

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
            GameFingerprint = ComputeGameFingerprint(ExecutablePath, Path.Combine(PakRoot, "pakchunk109-WindowsClient.pak"));
        }

        private static string ComputeGameFingerprint(string executablePath, string pakPath)
        {
            FileInfo executable = new FileInfo(executablePath);
            FileInfo pak = new FileInfo(pakPath);
            FileVersionInfo version = FileVersionInfo.GetVersionInfo(executablePath);
            string canonical = String.Join("|", new[] { version.FileVersion ?? String.Empty,
                version.ProductVersion ?? String.Empty, executable.Length.ToString(),
                executable.LastWriteTimeUtc.Ticks.ToString(), pak.Length.ToString(), pak.LastWriteTimeUtc.Ticks.ToString() });
            using (SHA256 sha = SHA256.Create())
            {
                byte[] hash = sha.ComputeHash(Encoding.UTF8.GetBytes(canonical));
                StringBuilder text = new StringBuilder(hash.Length * 2);
                foreach (byte value in hash) text.Append(value.ToString("x2"));
                return text.ToString();
            }
        }
    }
}

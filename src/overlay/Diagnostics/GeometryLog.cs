using System;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal static class GeometryLog
    {
        private static readonly string Path = ModPath.RuntimePath("logs", "DragonSwordWorldRadar.Geometry.log");

        public static void Write(string message)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }

            try
            {
                Directory.CreateDirectory(System.IO.Path.GetDirectoryName(Path));
                File.AppendAllText(
                    Path,
                    DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") +
                    " " + message + Environment.NewLine);
            }
            catch
            {
                // Diagnostic logging must never terminate the overlay.
            }
        }
    }
}

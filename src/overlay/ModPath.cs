using System;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal static class ModPath
    {
        public static readonly string BaseDirectory = Resolve();

        private static string Resolve()
        {
            string value = Environment.GetEnvironmentVariable(
                "EVENTRADAR_MOD_DIR");
            if (!String.IsNullOrWhiteSpace(value))
            {
                return Path.GetFullPath(value);
            }
            return Path.GetFullPath(Directory.GetCurrentDirectory());
        }

        public static string RuntimePath(params string[] parts)
        {
            string path = Path.Combine(BaseDirectory, "runtime");
            foreach (string part in parts)
            {
                path = Path.Combine(path, part);
            }
            return path;
        }
    }
}

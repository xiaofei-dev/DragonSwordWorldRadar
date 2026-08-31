using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

namespace DragonSwordWorldRadar
{
    internal sealed class GeneratedOwnerPointerConfig
    {
        private const string RequiredProvenance =
            "install-time-exact-executable-pattern";

        public readonly ulong Rva;
        public readonly string GameFingerprint;

        private GeneratedOwnerPointerConfig(ulong rva, string fingerprint)
        {
            Rva = rva;
            GameFingerprint = fingerprint;
        }

        public static GeneratedOwnerPointerConfig LoadForGame(Process game)
        {
            string configPath = Path.Combine(
                ModPath.BaseDirectory,
                "data",
                "generated",
                "save_owner_pointer.cfg");
            return Load(
                configPath,
                game.MainModule.FileName,
                ResolvePakPath(game.MainModule.FileName));
        }

        internal static GeneratedOwnerPointerConfig Load(
            string configPath,
            string executablePath,
            string pakPath)
        {
            Dictionary<string, string> fields = Parse(configPath);
            Require(fields, "schema_version", "1");
            Require(fields, "provenance", RequiredProvenance);

            string fingerprint = Value(fields, "game_fingerprint");
            if (!Regex.IsMatch(fingerprint, "^[0-9a-f]{64}$"))
            {
                throw new InvalidDataException(
                    "Generated owner-pointer fingerprint is malformed.");
            }
            string currentFingerprint = ComputeGameFingerprint(
                executablePath,
                pakPath);
            if (!String.Equals(
                fingerprint,
                currentFingerprint,
                StringComparison.Ordinal))
            {
                throw new InvalidDataException(
                    "Generated owner-pointer fingerprint does not match the current game.");
            }

            long executableLength;
            if (!Int64.TryParse(
                Value(fields, "executable_length"),
                NumberStyles.None,
                CultureInfo.InvariantCulture,
                out executableLength)
                || executableLength <= 0
                || new FileInfo(executablePath).Length != executableLength)
            {
                throw new InvalidDataException(
                    "Generated owner-pointer executable length is invalid.");
            }

            ulong rva;
            string rvaText = Value(fields, "owner_pointer_rva");
            if (!rvaText.StartsWith("0x", StringComparison.Ordinal)
                || !UInt64.TryParse(
                    rvaText.Substring(2),
                    NumberStyles.AllowHexSpecifier,
                    CultureInfo.InvariantCulture,
                    out rva)
                || rva == 0
                || rva >= (ulong)executableLength)
            {
                throw new InvalidDataException(
                    "Generated owner-pointer RVA is malformed or out of range.");
            }
            return new GeneratedOwnerPointerConfig(rva, fingerprint);
        }

        internal static string ComputeGameFingerprint(
            string executablePath,
            string pakPath)
        {
            FileInfo executable = new FileInfo(executablePath);
            FileInfo pak = new FileInfo(pakPath);
            if (!executable.Exists || !pak.Exists)
            {
                throw new FileNotFoundException(
                    "Current executable or fingerprint PAK is unavailable.");
            }
            FileVersionInfo version = FileVersionInfo.GetVersionInfo(
                executablePath);
            string canonical = String.Join("|", new[]
            {
                version.FileVersion ?? String.Empty,
                version.ProductVersion ?? String.Empty,
                executable.Length.ToString(CultureInfo.InvariantCulture),
                executable.LastWriteTimeUtc.Ticks.ToString(
                    CultureInfo.InvariantCulture),
                pak.Length.ToString(CultureInfo.InvariantCulture),
                pak.LastWriteTimeUtc.Ticks.ToString(
                    CultureInfo.InvariantCulture),
            });
            using (SHA256 sha = SHA256.Create())
            {
                byte[] hash = sha.ComputeHash(
                    Encoding.UTF8.GetBytes(canonical));
                StringBuilder text = new StringBuilder(hash.Length * 2);
                foreach (byte value in hash)
                {
                    text.Append(value.ToString("x2"));
                }
                return text.ToString();
            }
        }

        private static string ResolvePakPath(string executablePath)
        {
            DirectoryInfo win64 = Directory.GetParent(executablePath);
            DirectoryInfo binaries = win64 == null ? null : win64.Parent;
            DirectoryInfo ds = binaries == null ? null : binaries.Parent;
            DirectoryInfo gameRoot = ds == null ? null : ds.Parent;
            if (gameRoot == null)
            {
                throw new InvalidDataException(
                    "Could not resolve the game root for owner-pointer validation.");
            }
            return Path.Combine(
                gameRoot.FullName,
                "DS",
                "Content",
                "Paks",
                "pakchunk109-WindowsClient.pak");
        }

        private static Dictionary<string, string> Parse(string path)
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException(
                    "Generated owner-pointer configuration is missing.",
                    path);
            }
            Dictionary<string, string> fields =
                new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (string raw in File.ReadAllLines(path))
            {
                string line = raw.Trim();
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }
                int separator = line.IndexOf('=');
                if (separator <= 0 || separator == line.Length - 1)
                {
                    throw new InvalidDataException(
                        "Generated owner-pointer configuration contains a malformed line.");
                }
                string key = line.Substring(0, separator).Trim();
                string value = line.Substring(separator + 1).Trim();
                if (fields.ContainsKey(key))
                {
                    throw new InvalidDataException(
                        "Generated owner-pointer configuration contains a duplicate field.");
                }
                fields.Add(key, value);
            }
            string[] allowed =
            {
                "schema_version", "game_fingerprint",
                "executable_length", "owner_pointer_rva", "provenance"
            };
            if (fields.Count != allowed.Length)
            {
                throw new InvalidDataException(
                    "Generated owner-pointer configuration field count is invalid.");
            }
            foreach (string key in allowed)
            {
                Value(fields, key);
            }
            return fields;
        }

        private static string Value(
            IDictionary<string, string> fields,
            string key)
        {
            string value;
            if (!fields.TryGetValue(key, out value)
                || String.IsNullOrWhiteSpace(value))
            {
                throw new InvalidDataException(
                    "Generated owner-pointer configuration is missing " + key + ".");
            }
            return value;
        }

        private static void Require(
            IDictionary<string, string> fields,
            string key,
            string expected)
        {
            if (!String.Equals(Value(fields, key), expected, StringComparison.Ordinal))
            {
                throw new InvalidDataException(
                    "Generated owner-pointer " + key + " is unsupported.");
            }
        }
    }
}

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace DragonSwordWorldRadar
{
    internal sealed class SaveDatabaseFingerprint
    {
        private static readonly string[] SidecarSuffixes =
        {
            String.Empty,
            "-wal",
            "-shm",
            "-journal"
        };

        public readonly string Path;
        public readonly string Signature;
        public readonly DateTime LatestWriteUtc;
        public readonly bool Exists;

        private readonly IList<FileStamp> _files;

        private SaveDatabaseFingerprint(
            string path,
            IList<FileStamp> files)
        {
            Path = path;
            _files = files;
            Exists = files.Any(item => item.Suffix.Length == 0 && item.Exists);
            LatestWriteUtc = files
                .Where(item => item.Exists)
                .Select(item => item.LastWriteUtc)
                .DefaultIfEmpty(DateTime.MinValue)
                .Max();
            Signature = String.Join(
                ";",
                files.Select(item => item.ToSignature()).ToArray());
        }

        public static SaveDatabaseFingerprint Capture(string path)
        {
            List<FileStamp> files = new List<FileStamp>();
            foreach (string suffix in SidecarSuffixes)
            {
                files.Add(FileStamp.Capture(path + suffix, suffix));
            }
            return new SaveDatabaseFingerprint(path, files);
        }

        public string CopyConsistentSnapshot(string directory)
        {
            if (!Exists)
            {
                throw new FileNotFoundException(
                    "Save database does not exist.",
                    Path);
            }

            Directory.CreateDirectory(directory);
            string destination = System.IO.Path.Combine(
                directory,
                System.IO.Path.GetFileName(Path));

            Exception lastError = null;
            for (int attempt = 0; attempt < 3; attempt++)
            {
                try
                {
                    SaveDatabaseFingerprint before = Capture(Path);
                    if (!before.Exists)
                    {
                        throw new FileNotFoundException(
                            "Save database disappeared during snapshot.",
                            Path);
                    }

                    foreach (FileStamp file in before._files)
                    {
                        string target = destination + file.Suffix;
                        if (!file.Exists)
                        {
                            TryDelete(target);
                            continue;
                        }
                        CopyShared(file.Path, target);
                    }

                    SaveDatabaseFingerprint after = Capture(Path);
                    if (before.Signature == after.Signature)
                    {
                        return destination;
                    }
                    lastError = new IOException(
                        "Save database changed while its snapshot was copied.");
                }
                catch (Exception exception)
                {
                    lastError = exception;
                }
            }

            throw new IOException(
                "Could not create a consistent save database snapshot.",
                lastError);
        }

        private static void CopyShared(string source, string destination)
        {
            using (FileStream input = new FileStream(
                source,
                FileMode.Open,
                FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete))
            using (FileStream output = new FileStream(
                destination,
                FileMode.Create,
                FileAccess.Write,
                FileShare.Read))
            {
                input.CopyTo(output);
                // Consistency is verified by comparing the source fingerprint
                // before and after the copy. A forced physical-device flush is
                // unnecessary for this temporary reader snapshot and can
                // contend with the game's own WAL/save writes.
                output.Flush();
            }
        }

        private static void TryDelete(string path)
        {
            try
            {
                if (File.Exists(path))
                {
                    File.Delete(path);
                }
            }
            catch
            {
            }
        }

        private sealed class FileStamp
        {
            public string Path;
            public string Suffix;
            public bool Exists;
            public long Length;
            public DateTime LastWriteUtc;

            public static FileStamp Capture(
                string path,
                string suffix)
            {
                FileStamp result = new FileStamp
                {
                    Path = path,
                    Suffix = suffix
                };
                try
                {
                    FileInfo info = new FileInfo(path);
                    info.Refresh();
                    result.Exists = info.Exists;
                    if (result.Exists)
                    {
                        result.Length = info.Length;
                        result.LastWriteUtc = info.LastWriteTimeUtc;
                    }
                }
                catch
                {
                    result.Exists = false;
                }
                return result;
            }

            public string ToSignature()
            {
                return String.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}:{2}:{3}",
                    Suffix.Length == 0 ? "base" : Suffix,
                    Exists ? 1 : 0,
                    Length,
                    LastWriteUtc.Ticks);
            }
        }
    }

    internal sealed class SaveSlotFingerprint
    {
        public readonly string Stem;
        public readonly IList<SaveDatabaseFingerprint> Databases;
        public readonly string Signature;
        public readonly DateTime LatestWriteUtc;

        private SaveSlotFingerprint(
            string stem,
            IList<SaveDatabaseFingerprint> databases)
        {
            Stem = stem;
            Databases = databases;
            LatestWriteUtc = databases
                .Where(item => item.Exists)
                .Select(item => item.LatestWriteUtc)
                .DefaultIfEmpty(DateTime.MinValue)
                .Max();
            Signature = String.Join(
                "||",
                databases.Select(item =>
                    System.IO.Path.GetExtension(item.Path) + "=" +
                    item.Signature).ToArray());
        }

        public static SaveSlotFingerprint Capture(
            string selectedDatabasePath)
        {
            string directory = System.IO.Path.GetDirectoryName(
                selectedDatabasePath);
            string stemName = System.IO.Path.GetFileNameWithoutExtension(
                selectedDatabasePath);
            if (String.IsNullOrEmpty(directory)
                || String.IsNullOrEmpty(stemName))
            {
                throw new InvalidOperationException(
                    "The active save slot path is invalid.");
            }

            string stem = System.IO.Path.Combine(directory, stemName);
            List<SaveDatabaseFingerprint> databases = new List<SaveDatabaseFingerprint>
            {
                SaveDatabaseFingerprint.Capture(stem + ".db"),
                SaveDatabaseFingerprint.Capture(stem + ".bak")
            };
            if (!databases.Any(item => item.Exists))
            {
                throw new FileNotFoundException(
                    "Neither the active .db nor .bak save database exists.",
                    stem);
            }
            databases = databases
                .OrderByDescending(item => item.LatestWriteUtc)
                .ToList();
            return new SaveSlotFingerprint(stem, databases);
        }

        public string Summary
        {
            get
            {
                return String.Join(
                    ",",
                    Databases.Select(item => String.Format(
                        CultureInfo.InvariantCulture,
                        "{0}@{1:O}",
                        System.IO.Path.GetFileName(item.Path),
                        item.LatestWriteUtc)).ToArray());
            }
        }
    }
}

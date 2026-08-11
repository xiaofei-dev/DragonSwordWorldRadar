using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Diagnostics;
using System.Xml;

namespace DragonSwordWorldRadar.Installer
{
    internal static class TreasurePakExtractor
    {
        private const uint PakMagic = 0x5A6F12E1;
        private const uint DragonSwordPakVersion = 101;
        private const int PakFooterSize = 221;
        private const string TargetMountPoint =
            "../../../DS/Content/__GeneratedGameData__/Server/XML/GameData/";
        private const string DefaultTargetFileName =
            "SectionTreasureBoxData.xml";

        public static void Extract(
            string executablePath,
            string pakPath,
            string oodleLibraryPath,
            string outputPath)
        {
            Extract(
                executablePath,
                pakPath,
                oodleLibraryPath,
                DefaultTargetFileName,
                outputPath);
        }

        public static void Extract(
            string executablePath,
            string pakPath,
            string oodleLibraryPath,
            string targetFileName,
            string outputPath)
        {
            if (!File.Exists(executablePath))
            {
                throw new FileNotFoundException(
                    "The game executable was not found.", executablePath);
            }
            if (!File.Exists(pakPath))
            {
                throw new FileNotFoundException(
                    "The generated game-data PAK was not found.", pakPath);
            }
            byte[] aesKey = FindWorkingAesKey(executablePath, pakPath);
            ExtractXml(
                pakPath,
                aesKey,
                oodleLibraryPath,
                targetFileName,
                outputPath);
        }

        public static string[] ExtractMatching(
            string executablePath,
            string pakPath,
            string oodleLibraryPath,
            string outputDirectory,
            string[] fileNameFragments,
            int maximumFiles)
        {
            if (!File.Exists(executablePath))
            {
                throw new FileNotFoundException(
                    "The game executable was not found.", executablePath);
            }
            if (!File.Exists(pakPath))
            {
                throw new FileNotFoundException(
                    "The generated game-data PAK was not found.", pakPath);
            }
            if (fileNameFragments == null
                || fileNameFragments.Length == 0)
            {
                throw new ArgumentException(
                    "At least one XML filename fragment is required.",
                    "fileNameFragments");
            }
            if (maximumFiles <= 0 || maximumFiles > 128)
            {
                throw new ArgumentOutOfRangeException(
                    "maximumFiles",
                    "The matching XML extraction limit must be 1-128.");
            }

            Directory.CreateDirectory(outputDirectory);
            byte[] aesKey = FindWorkingAesKey(executablePath, pakPath);
            return ExtractMatchingXml(
                pakPath,
                aesKey,
                oodleLibraryPath,
                outputDirectory,
                fileNameFragments,
                maximumFiles);
        }

        private static byte[] FindWorkingAesKey(
            string executablePath, string pakPath)
        {
            foreach (byte[] candidate in AesKeyFinder.Find(executablePath))
            {
                if (CanReadTargetPak(pakPath, candidate))
                {
                    return candidate;
                }
            }
            throw new InvalidDataException(
                "The PAK encryption key could not be detected. " +
                "The game version may not be supported.");
        }

        private static bool CanReadTargetPak(
            string pakPath, byte[] key)
        {
            try
            {
                using (FileStream stream = File.OpenRead(pakPath))
                using (BinaryReader reader = new BinaryReader(stream))
                {
                    PakFooter footer = ReadFooter(reader);
                    byte[] index = ReadAt(
                        reader, footer.IndexOffset, footer.IndexSize);
                    if (footer.Encrypted)
                    {
                        index = DecryptAes(index, key);
                    }
                    byte numberMask = InferNumberMask(index);
                    byte stringMask = (byte)(index[4] ^ (byte)'.');
                    CustomReader indexReader = new CustomReader(
                        index, stringMask, numberMask);
                    return indexReader.ReadString().Equals(
                        TargetMountPoint,
                        StringComparison.OrdinalIgnoreCase);
                }
            }
            catch
            {
                return false;
            }
        }

        private static void ExtractXml(
            string pakPath,
            byte[] key,
            string oodleLibraryPath,
            string targetFileName,
            string outputPath)
        {
            using (FileStream stream = File.OpenRead(pakPath))
            using (BinaryReader binary = new BinaryReader(stream))
            {
                PakFooter footer = ReadFooter(binary);
                byte[] index = ReadAt(
                    binary, footer.IndexOffset, footer.IndexSize);
                if (footer.Encrypted)
                {
                    index = DecryptAes(index, key);
                }

                byte numberMask = InferNumberMask(index);
                byte stringMask = (byte)(index[4] ^ (byte)'.');
                CustomReader main = new CustomReader(
                    index, stringMask, numberMask);
                string mountPoint = main.ReadString();
                if (!mountPoint.Equals(
                    TargetMountPoint,
                    StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException(
                        "The generated game-data PAK has an unexpected mount point.");
                }

                main.ReadUInt32();
                main.ReadUInt64();
                uint hasPathHashIndex = main.ReadUInt32();
                if (hasPathHashIndex != 0)
                {
                    main.ReadUInt64();
                    main.ReadUInt64();
                    main.ReadRaw(20);
                }

                uint hasDirectoryIndex = main.ReadUInt32();
                if (hasDirectoryIndex == 0)
                {
                    throw new InvalidDataException(
                        "The PAK directory index is missing.");
                }
                ulong directoryOffset = main.ReadUInt64();
                ulong directorySize = main.ReadUInt64();
                main.ReadRaw(20);
                uint encodedEntriesSize = main.ReadUInt32();
                byte[] encodedEntries = main.ReadRaw(
                    checked((int)encodedEntriesSize));

                byte[] directoryIndex = ReadAt(
                    binary, directoryOffset, directorySize);
                if (footer.Encrypted)
                {
                    directoryIndex = DecryptAes(directoryIndex, key);
                }
                byte directoryStringMask = directoryIndex.Length > 8
                    ? (byte)(directoryIndex[8] ^ (byte)'/')
                    : stringMask;
                CustomReader directory = new CustomReader(
                    directoryIndex,
                    directoryStringMask,
                    numberMask);

                int encodedOffset = FindTargetEncodedOffset(
                    directory, targetFileName);
                if (encodedOffset < 0)
                {
                    throw new InvalidDataException(
                        "The requested XML uses an unsupported PAK entry layout.");
                }

                EncodedEntry encoded = FindEncodedEntry(
                    encodedEntries, encodedOffset, binary);
                DataEntry data = ReadDataEntry(binary, encoded.Offset);
                ValidateEntry(encoded, data, stream.Length);
                WriteDecompressedEntry(
                    binary,
                    data,
                    key,
                    oodleLibraryPath,
                    outputPath);
            }
        }

        private static string[] ExtractMatchingXml(
            string pakPath,
            byte[] key,
            string oodleLibraryPath,
            string outputDirectory,
            string[] fileNameFragments,
            int maximumFiles)
        {
            using (FileStream stream = File.OpenRead(pakPath))
            using (BinaryReader binary = new BinaryReader(stream))
            {
                PakFooter footer = ReadFooter(binary);
                byte[] index = ReadAt(
                    binary, footer.IndexOffset, footer.IndexSize);
                if (footer.Encrypted)
                {
                    index = DecryptAes(index, key);
                }

                byte numberMask = InferNumberMask(index);
                byte stringMask = (byte)(index[4] ^ (byte)'.');
                CustomReader main = new CustomReader(
                    index, stringMask, numberMask);
                string mountPoint = main.ReadString();
                if (!mountPoint.Equals(
                    TargetMountPoint,
                    StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException(
                        "The generated game-data PAK has an unexpected mount point.");
                }

                main.ReadUInt32();
                main.ReadUInt64();
                uint hasPathHashIndex = main.ReadUInt32();
                if (hasPathHashIndex != 0)
                {
                    main.ReadUInt64();
                    main.ReadUInt64();
                    main.ReadRaw(20);
                }

                uint hasDirectoryIndex = main.ReadUInt32();
                if (hasDirectoryIndex == 0)
                {
                    throw new InvalidDataException(
                        "The PAK directory index is missing.");
                }
                ulong directoryOffset = main.ReadUInt64();
                ulong directorySize = main.ReadUInt64();
                main.ReadRaw(20);
                uint encodedEntriesSize = main.ReadUInt32();
                byte[] encodedEntries = main.ReadRaw(
                    checked((int)encodedEntriesSize));

                byte[] directoryIndex = ReadAt(
                    binary, directoryOffset, directorySize);
                if (footer.Encrypted)
                {
                    directoryIndex = DecryptAes(directoryIndex, key);
                }
                byte directoryStringMask = directoryIndex.Length > 8
                    ? (byte)(directoryIndex[8] ^ (byte)'/')
                    : stringMask;
                CustomReader directory = new CustomReader(
                    directoryIndex,
                    directoryStringMask,
                    numberMask);

                List<PakDirectoryEntry> allEntries =
                    ReadDirectoryEntries(directory);
                List<PakDirectoryEntry> matches = allEntries
                    .Where(entry => MatchesAnyFragment(
                        entry.FileName,
                        fileNameFragments))
                    .OrderBy(entry => GetMatchRank(
                        entry.FileName,
                        fileNameFragments))
                    .ThenBy(entry => entry.FileName,
                        StringComparer.OrdinalIgnoreCase)
                    .Take(maximumFiles)
                    .ToList();
                if (matches.Count == 0)
                {
                    throw new FileNotFoundException(
                        "No matching Mole/MiniGame XML files were found in the game PAK.");
                }

                List<string> outputs = new List<string>();
                int outputIndex = 0;
                foreach (PakDirectoryEntry entry in matches)
                {
                    string safeName = Path.GetFileName(entry.FileName);
                    string outputPath = Path.Combine(
                        outputDirectory,
                        outputIndex.ToString(
                            "D3",
                            CultureInfo.InvariantCulture)
                            + "_" + safeName);
                    outputIndex++;
                    int nextEncodedOffset = FindNextEncodedOffset(
                        allEntries,
                        entry.EncodedOffset,
                        encodedEntries.Length);
                    if (TryExtractMatchingEntry(
                        binary,
                        stream.Length,
                        encodedEntries,
                        entry,
                        nextEncodedOffset,
                        numberMask,
                        key,
                        oodleLibraryPath,
                        outputPath))
                    {
                        outputs.Add(outputPath);
                    }
                    else
                    {
                        DeleteIfPresent(outputPath);
                        // A bounded filename search can include unrelated or
                        // revision-specific entries. Invalid candidates are
                        // skipped; the provider still requires a complete,
                        // validated real-Fly output before installation.
                    }
                }

                if (outputs.Count == 0)
                {
                    throw new InvalidDataException(
                        "Matching Mole/MiniGame XML entries could not be decoded.");
                }
                return outputs.ToArray();
            }
        }

        private static bool TryExtractMatchingEntry(
            BinaryReader binary,
            long pakLength,
            byte[] encodedEntries,
            PakDirectoryEntry directoryEntry,
            int nextEncodedOffset,
            byte numberMask,
            byte[] key,
            string oodleLibraryPath,
            string outputPath)
        {
            DeleteIfPresent(outputPath);

            // RevealCycleData is intercepted before every generic decoder.
            // Its exact build-pinned path is terminal: failure is fail-closed
            // and must never fall through to compact or mask-search logic.
            if (String.Equals(directoryEntry.FileName, "RevealCycleData.xml",
                StringComparison.OrdinalIgnoreCase))
            {
                return TryExtractExactRevealCycleEntry(binary, pakLength,
                    encodedEntries, directoryEntry.EncodedOffset,
                    nextEncodedOffset, key, oodleLibraryPath, outputPath,
                    directoryEntry.FileName);
            }

            // Most v101 entries use the one number mask inferred from the real
            // PAK index. Try that deterministic path first so a Mole install
            // does not perform a 0..255 mask search for every support table.
            try
            {
                EncodedEntry exact = ReadEncodedEntry(
                    encodedEntries,
                    directoryEntry.EncodedOffset,
                    numberMask);
                if (IsSaneEncodedEntry(exact, pakLength, false)
                    && TryWriteValidatedEntry(
                        binary,
                        pakLength,
                        exact,
                        key,
                        oodleLibraryPath,
                        outputPath,
                        directoryEntry.FileName))
                {
                    return true;
                }
            }
            catch
            {
                DeleteIfPresent(outputPath);
            }

            // Some current game-data tables use a deterministic 16-byte
            // one-block compact record. Try this exact, constant-time decoder
            // before the legacy 0..255 compatibility search so install-time
            // support-table extraction does not multiply random PAK reads.
            if (TryExtractSingleBlockXorCompactEntry(
                binary,
                pakLength,
                encodedEntries,
                directoryEntry.EncodedOffset,
                nextEncodedOffset,
                key,
                oodleLibraryPath,
                outputPath,
                directoryEntry.FileName))
            {
                return true;
            }

            // Keep the proven legacy decoder last for non-compact revisions.
            try
            {
                EncodedEntry legacy = FindEncodedEntry(
                    encodedEntries,
                    directoryEntry.EncodedOffset,
                    binary);
                if (TryWriteValidatedEntry(
                    binary,
                    pakLength,
                    legacy,
                    key,
                    oodleLibraryPath,
                    outputPath,
                    directoryEntry.FileName))
                {
                    return true;
                }
            }
            catch
            {
                DeleteIfPresent(outputPath);
            }
            return false;
        }

        private static bool TryWriteValidatedEntry(
            BinaryReader binary,
            long pakLength,
            EncodedEntry encoded,
            byte[] key,
            string oodleLibraryPath,
            string outputPath,
            string expectedFileName)
        {
            try
            {
                if (!IsSaneEncodedEntry(encoded, pakLength, false))
                {
                    return false;
                }
                DataEntry data = ReadDataEntry(binary, encoded.Offset);
                ValidateEntry(encoded, data, pakLength);
                WriteDecompressedEntry(
                    binary,
                    data,
                    key,
                    oodleLibraryPath,
                    outputPath);
                if (!ValidateTargetXmlIdentity(
                    outputPath,
                    expectedFileName))
                {
                    DeleteIfPresent(outputPath);
                    return false;
                }
                return true;
            }
            catch
            {
                DeleteIfPresent(outputPath);
                return false;
            }
        }

        private static bool TryExtractSingleBlockXorCompactEntry(
            BinaryReader binary,
            long pakLength,
            byte[] encodedEntries,
            int encodedOffset,
            int nextEncodedOffset,
            byte[] key,
            string oodleLibraryPath,
            string outputPath,
            string expectedFileName)
        {
            try
            {
                if (encodedOffset < 0
                    || encodedOffset + 16 > encodedEntries.Length
                    || nextEncodedOffset <= encodedOffset
                    || nextEncodedOffset - encodedOffset != 16)
                {
                    return false;
                }

                byte mask = encodedEntries[encodedOffset + 1];
                byte[] decoded = new byte[16];
                for (int index = 0; index < decoded.Length; index++)
                {
                    decoded[index] = (byte)(
                        encodedEntries[encodedOffset + index] ^ mask);
                }

                uint bits = ReadUInt32LittleEndian(decoded, 0);
                int compressionCode =
                    (int)((bits >> 23) & 0x3F);
                bool encrypted = (bits & (1u << 22)) != 0;
                uint blockCount = (bits >> 6) & 0xFFFF;
                uint blockSizeCode = bits & 0x3F;
                bool offset32 = (bits & (1u << 31)) != 0;
                bool uncompressed32 = (bits & (1u << 30)) != 0;
                bool compressed32 = (bits & (1u << 29)) != 0;

                if (compressionCode != 1
                    || encrypted
                    || blockCount != 1
                    || !offset32
                    || !uncompressed32
                    || !compressed32
                    || blockSizeCode == 0x3F)
                {
                    return false;
                }

                ulong pakOffset =
                    ReadUInt32LittleEndian(decoded, 4);
                ulong uncompressedSize =
                    ReadUInt32LittleEndian(decoded, 8);
                ulong compressedSize =
                    ReadUInt32LittleEndian(decoded, 12);
                if (pakOffset == 0
                    || pakOffset >= (ulong)pakLength
                    || uncompressedSize == 0
                    || compressedSize == 0
                    || uncompressedSize > 268435456
                    || compressedSize > 268435456)
                {
                    return false;
                }

                DataEntry data = ReadDataEntry(binary, pakOffset);
                ValidateCompactSingleBlockEntry(
                    data,
                    pakOffset,
                    compressedSize,
                    uncompressedSize,
                    pakLength);
                WriteDecompressedEntry(
                    binary,
                    data,
                    key,
                    oodleLibraryPath,
                    outputPath);
                if (!ValidateTargetXmlIdentity(
                    outputPath,
                    expectedFileName))
                {
                    DeleteIfPresent(outputPath);
                    return false;
                }
                return true;
            }
            catch
            {
                DeleteIfPresent(outputPath);
                return false;
            }
        }

        private static void ValidateCompactSingleBlockEntry(
            DataEntry data,
            ulong expectedPakOffset,
            ulong expectedCompressedSize,
            ulong expectedUncompressedSize,
            long pakLength)
        {
            // Match the proven probe boundary: the full DataEntry header is
            // authoritative for block layout. Cross-check only the values that
            // are encoded in the 16-byte record, then let the existing checked
            // decompressor validate block ranges and final output length.
            if (data == null
                || data.PakOffset != expectedPakOffset
                || data.CompressedSize != expectedCompressedSize
                || data.UncompressedSize != expectedUncompressedSize
                || data.CompressionIndex != 1
                || (data.Flags & 1) != 0)
            {
                throw new InvalidDataException(
                    "The compact PAK entry metadata is inconsistent.");
            }

            ValidateDataEntryBounds(data, pakLength);
        }

        private static void ValidateDataEntryBounds(
            DataEntry data,
            long pakLength)
        {
            if (data.UncompressedSize > 256UL * 1024UL * 1024UL
                || data.CompressedSize > 256UL * 1024UL * 1024UL
                || data.PayloadOffset > (ulong)pakLength
                || data.CompressionIndex > 1)
            {
                throw new InvalidDataException(
                    "The compact PAK data entry is outside the supported bounds.");
            }

            if (data.CompressionIndex == 1)
            {
                if (data.Blocks.Count == 0
                    || data.Blocks.Count > 4096
                    || data.CompressionBlockSize == 0
                    || data.CompressionBlockSize > 64U * 1024U * 1024U)
                {
                    throw new InvalidDataException(
                        "The compact PAK compression layout is invalid.");
                }
                foreach (DataBlock block in data.Blocks)
                {
                    if (block.End <= block.Start
                        || data.PakOffset > (ulong)pakLength
                        || block.End > (ulong)pakLength - data.PakOffset)
                    {
                        throw new InvalidDataException(
                            "The compact PAK compression block is outside the file.");
                    }
                }
            }
        }

        private static bool IsSaneEncodedEntry(
            EncodedEntry candidate,
            long pakLength,
            bool allowCompactZeroBlockSize)
        {
            if (candidate == null
                || candidate.Offset >= (ulong)pakLength
                || candidate.UncompressedSize == 0
                || candidate.UncompressedSize > 256 * 1024 * 1024
                || candidate.CompressedSize == 0)
            {
                return false;
            }

            bool compressed = candidate.CompressionSlot >= 0;
            if (compressed)
            {
                return candidate.CompressionBlockCount > 0
                    && candidate.CompressionBlockCount <= 4096
                    && (allowCompactZeroBlockSize
                        || candidate.CompressionBlockSize > 0)
                    && candidate.CompressionBlockSize
                        <= 64 * 1024 * 1024;
            }

            return candidate.CompressedSize
                    == candidate.UncompressedSize
                && candidate.CompressionBlockCount == 0;
        }

        private static int FindNextEncodedOffset(
            IList<PakDirectoryEntry> entries,
            int encodedOffset,
            int encodedEntriesLength)
        {
            int next = encodedEntriesLength;
            foreach (PakDirectoryEntry entry in entries)
            {
                if (entry.EncodedOffset > encodedOffset
                    && entry.EncodedOffset < next)
                {
                    next = entry.EncodedOffset;
                }
            }
            return next;
        }

        private static uint ReadUInt32LittleEndian(
            byte[] buffer,
            int offset)
        {
            if (buffer == null
                || offset < 0
                || offset + 4 > buffer.Length)
            {
                throw new ArgumentOutOfRangeException("offset");
            }
            return (uint)(
                buffer[offset]
                | (buffer[offset + 1] << 8)
                | (buffer[offset + 2] << 16)
                | (buffer[offset + 3] << 24));
        }

        private static bool ValidateTargetXmlIdentity(
            string outputPath,
            string expectedFileName)
        {
            try
            {
                FileInfo info = new FileInfo(outputPath);
                if (!info.Exists
                    || info.Length <= 0
                    || info.Length > 256L * 1024L * 1024L)
                {
                    return false;
                }

                XmlDocument document = new XmlDocument();
                document.XmlResolver = null;
                document.Load(outputPath);
                if (document.DocumentElement == null)
                {
                    return false;
                }

                string expected = Path.GetFileNameWithoutExtension(
                    expectedFileName) ?? String.Empty;
                string root = document.DocumentElement.LocalName;
                return String.Equals(
                        root,
                        expected,
                        StringComparison.OrdinalIgnoreCase)
                    || String.Equals(
                        root,
                        expected + "Map",
                        StringComparison.OrdinalIgnoreCase);
            }
            catch
            {
                return false;
            }
        }

        private static void DeleteIfPresent(string path)
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

        private static List<PakDirectoryEntry> ReadDirectoryEntries(
            CustomReader directory)
        {
            uint directoryCount = directory.ReadUInt32();
            if (directoryCount > 100000)
            {
                throw new InvalidDataException(
                    "The PAK directory count is invalid.");
            }

            List<PakDirectoryEntry> entries =
                new List<PakDirectoryEntry>();
            for (uint directoryIndex = 0;
                directoryIndex < directoryCount;
                directoryIndex++)
            {
                string directoryName = directory.ReadString();
                uint fileCount = directory.ReadUInt32();
                if (fileCount > 1000000)
                {
                    throw new InvalidDataException(
                        "The PAK file count is invalid.");
                }
                for (uint fileIndex = 0;
                    fileIndex < fileCount;
                    fileIndex++)
                {
                    PakDirectoryEntry entry = new PakDirectoryEntry();
                    entry.DirectoryName = directoryName;
                    entry.FileName = directory.ReadStringEndingWith(".xml");
                    entry.EncodedOffset = directory.ReadInt32();
                    entries.Add(entry);
                }
            }
            return entries;
        }

        private static bool MatchesAnyFragment(
            string fileName,
            string[] fragments)
        {
            if (String.IsNullOrWhiteSpace(fileName))
            {
                return false;
            }
            foreach (string fragment in fragments)
            {
                if (!String.IsNullOrWhiteSpace(fragment)
                    && fileName.IndexOf(
                        fragment,
                        StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    return true;
                }
            }
            return false;
        }

        private static int GetMatchRank(
            string fileName,
            string[] fragments)
        {
            for (int index = 0; index < fragments.Length; index++)
            {
                string fragment = fragments[index];
                if (String.IsNullOrWhiteSpace(fragment))
                {
                    continue;
                }
                if (String.Equals(
                    fileName,
                    fragment,
                    StringComparison.OrdinalIgnoreCase))
                {
                    return index * 10;
                }
                if (fileName.IndexOf(
                    fragment,
                    StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    return index * 10 + 1;
                }
            }
            return Int32.MaxValue;
        }

        private static int FindTargetEncodedOffset(
            CustomReader directory,
            string targetFileName)
        {
            uint directoryCount = directory.ReadUInt32();
            if (directoryCount > 100000)
            {
                throw new InvalidDataException(
                    "The PAK directory count is invalid.");
            }

            for (uint directoryIndex = 0;
                directoryIndex < directoryCount;
                directoryIndex++)
            {
                directory.ReadString();
                uint fileCount = directory.ReadUInt32();
                if (fileCount > 1000000)
                {
                    throw new InvalidDataException(
                        "The PAK file count is invalid.");
                }

                for (uint fileIndex = 0;
                    fileIndex < fileCount;
                    fileIndex++)
                {
                    string fileName = directory.ReadStringEndingWith(".xml");
                    int encodedOffset = directory.ReadInt32();
                    if (fileName.Equals(
                        targetFileName,
                        StringComparison.OrdinalIgnoreCase))
                    {
                        return encodedOffset;
                    }
                }
            }
            throw new FileNotFoundException(
                targetFileName + " was not found in the game PAK.");
        }

        private static EncodedEntry FindEncodedEntry(
            byte[] encodedEntries,
            int encodedOffset,
            BinaryReader pakReader)
        {
            for (int mask = 0; mask <= Byte.MaxValue; mask++)
            {
                try
                {
                    EncodedEntry candidate = ReadEncodedEntry(
                        encodedEntries, encodedOffset, (byte)mask);
                    bool compressed = candidate.CompressionSlot >= 0;
                    if (candidate.Offset >= (ulong)pakReader.BaseStream.Length
                        || candidate.UncompressedSize == 0
                        || candidate.UncompressedSize > 64 * 1024 * 1024
                        || candidate.CompressedSize == 0
                        || (compressed
                            && (candidate.CompressionBlockCount == 0
                                || candidate.CompressionBlockCount > 4096
                                || candidate.CompressionBlockSize == 0
                                || candidate.CompressionBlockSize > 4 * 1024 * 1024))
                        || (!compressed
                            && (candidate.CompressedSize != candidate.UncompressedSize
                                || candidate.CompressionBlockCount != 0)))
                    {
                        continue;
                    }

                    DataEntry data = ReadDataEntry(
                        pakReader, candidate.Offset);
                    if (data.CompressedSize == candidate.CompressedSize
                        && data.UncompressedSize == candidate.UncompressedSize
                        && data.CompressionIndex ==
                            (uint)(candidate.CompressionSlot + 1)
                        && ((data.Flags & 1) != 0) == candidate.Encrypted)
                    {
                        return candidate;
                    }
                }
                catch
                {
                }
            }
            throw new InvalidDataException(
                "The requested XML entry could not be decoded.");
        }

        private static EncodedEntry TryReadExactXor12UncompressedEntry(
            byte[] encodedEntries,
            int encodedOffset,
            BinaryReader pakReader)
        {
            if (encodedOffset < 0 || encodedOffset + 12 > encodedEntries.Length)
            {
                return null;
            }
            byte mask = encodedEntries[encodedOffset + 1];
            if (encodedEntries[encodedOffset] != mask
                || encodedEntries[encodedOffset + 2] != mask)
            {
                return null;
            }
            byte[] decoded = new byte[12];
            for (int index = 0; index < decoded.Length; index++)
            {
                decoded[index] = (byte)(encodedEntries[encodedOffset + index] ^ mask);
            }
            uint bits = BitConverter.ToUInt32(decoded, 0);
            int compressionCode = (int)((bits >> 23) & 0x3F);
            bool encrypted = (bits & (1u << 22)) != 0;
            uint blockCount = (bits >> 6) & 0xFFFF;
            uint blockSizeCode = bits & 0x3F;
            bool offset32 = (bits & (1u << 31)) != 0;
            bool uncompressed32 = (bits & (1u << 30)) != 0;
            bool compressed32 = (bits & (1u << 29)) != 0;
            if (compressionCode != 0 || encrypted || blockCount != 0
                || blockSizeCode != 0 || !offset32 || !uncompressed32
                || !compressed32)
            {
                return null;
            }
            ulong pakOffset = BitConverter.ToUInt32(decoded, 4);
            ulong size = BitConverter.ToUInt32(decoded, 8);
            if (pakOffset == 0 || pakOffset >= (ulong)pakReader.BaseStream.Length
                || size == 0 || size > 256UL * 1024UL * 1024UL)
            {
                return null;
            }
            try
            {
                DataEntry data = ReadDataEntry(pakReader, pakOffset);
                if (data.Offset != 0 || data.CompressedSize != size
                    || data.UncompressedSize != size || data.CompressionIndex != 0
                    || (data.Flags & 1) != 0)
                {
                    return null;
                }
            }
            catch
            {
                return null;
            }
            return new EncodedEntry
            {
                CompressionSlot = -1,
                Encrypted = false,
                CompressionBlockCount = 0,
                CompressionBlockSize = 0,
                Offset = pakOffset,
                CompressedSize = size,
                UncompressedSize = size
            };
        }

        private static bool TryExtractExactRevealCycleEntry(
            BinaryReader binary,
            long pakLength,
            byte[] encodedEntries,
            int encodedOffset,
            int nextEncodedOffset,
            byte[] key,
            string oodleLibraryPath,
            string outputPath,
            string expectedFileName)
        {
            const string ExpectedSha256 =
                "f323f0a09da370cf9b22f0589376f18d969261b562938376085ec6ccd706cf87";
            if (nextEncodedOffset - encodedOffset != 12)
            {
                return false;
            }
            EncodedEntry encoded = TryReadExactXor12UncompressedEntry(
                encodedEntries,
                encodedOffset,
                binary);
            if (encoded == null || encoded.CompressedSize != encoded.UncompressedSize
                || encoded.UncompressedSize != 754)
            {
                return false;
            }
            if (!TryWriteValidatedEntry(binary, pakLength, encoded, key,
                    oodleLibraryPath, outputPath, expectedFileName))
            {
                return false;
            }
            try
            {
                string digest;
                using (SHA256 sha = SHA256.Create())
                using (FileStream input = File.OpenRead(outputPath))
                {
                    digest = String.Concat(sha.ComputeHash(input)
                        .Select(value => value.ToString("x2", CultureInfo.InvariantCulture)));
                }
                if (!String.Equals(digest, ExpectedSha256, StringComparison.Ordinal))
                {
                    DeleteIfPresent(outputPath);
                    return false;
                }
                XmlDocument document = new XmlDocument { XmlResolver = null };
                document.Load(outputPath);
                if (document.DocumentElement == null
                    || !String.Equals(document.DocumentElement.LocalName,
                        "RevealCycleDataMap", StringComparison.Ordinal))
                {
                    DeleteIfPresent(outputPath);
                    return false;
                }
                bool foundRequiredCycle = false;
                foreach (XmlNode node in document.SelectNodes("//*"))
                {
                    XmlElement element = node as XmlElement;
                    if (element == null) continue;
                    string id = GetXmlAttribute(element, "RevealCycleID")
                        ?? GetXmlAttribute(element, "ID");
                    if (id == "10001"
                        && GetXmlAttribute(element, "RevealIngametime") == "23"
                        && GetXmlAttribute(element, "HideIngametime") == "6")
                    {
                        foundRequiredCycle = true;
                        break;
                    }
                }
                if (!foundRequiredCycle)
                {
                    DeleteIfPresent(outputPath);
                    return false;
                }
                return true;
            }
            catch
            {
                DeleteIfPresent(outputPath);
                return false;
            }
        }

        private static string GetXmlAttribute(XmlElement element, string name)
        {
            foreach (XmlAttribute attribute in element.Attributes)
            {
                if (String.Equals(attribute.LocalName, name,
                    StringComparison.OrdinalIgnoreCase)) return attribute.Value.Trim();
            }
            return null;
        }

        private static EncodedEntry ReadEncodedEntry(
            byte[] encodedEntries, int offset, byte numberMask)
        {
            CustomReader reader = new CustomReader(
                encodedEntries, 0, numberMask);
            reader.Position = offset;
            uint bits = reader.ReadUInt32();
            int compression = (int)((bits >> 23) & 0x3F);
            compression = compression == 0 ? -1 : compression - 1;
            bool encrypted = (bits & (1u << 22)) != 0;
            uint blockCount = (bits >> 6) & 0xFFFF;
            uint blockSize = bits & 0x3F;
            if (blockSize == 0x3F)
            {
                blockSize = reader.ReadUInt32();
            }
            else
            {
                blockSize <<= 11;
            }

            Func<int, ulong> readVariable = bit =>
                (bits & (1u << bit)) != 0
                    ? reader.ReadUInt32()
                    : reader.ReadUInt64();
            ulong dataOffset = readVariable(31);
            ulong uncompressed = readVariable(30);
            ulong compressed = compression < 0
                ? uncompressed
                : readVariable(29);

            if (blockCount > 1 || (blockCount > 0 && encrypted))
            {
                for (uint index = 0; index < blockCount; index++)
                {
                    reader.ReadUInt32();
                }
            }

            EncodedEntry entry = new EncodedEntry();
            entry.CompressionSlot = compression;
            entry.Encrypted = encrypted;
            entry.CompressionBlockCount = blockCount;
            entry.CompressionBlockSize = blockSize;
            entry.Offset = dataOffset;
            entry.UncompressedSize = uncompressed;
            entry.CompressedSize = compressed;
            return entry;
        }

        private static DataEntry ReadDataEntry(
            BinaryReader reader, ulong entryOffset)
        {
            reader.BaseStream.Position = checked((long)entryOffset);
            DataEntry entry = new DataEntry();
            entry.PakOffset = entryOffset;
            entry.Offset = reader.ReadUInt64();
            entry.CompressedSize = reader.ReadUInt64();
            entry.UncompressedSize = reader.ReadUInt64();
            entry.CompressionIndex = reader.ReadUInt32();
            reader.ReadBytes(20);

            if (entry.CompressionIndex != 0)
            {
                uint blockCount = reader.ReadUInt32();
                if (blockCount == 0 || blockCount > 4096)
                {
                    throw new InvalidDataException(
                        "The PAK compression block count is invalid.");
                }
                for (uint index = 0; index < blockCount; index++)
                {
                    DataBlock block = new DataBlock();
                    block.Start = reader.ReadUInt64();
                    block.End = reader.ReadUInt64();
                    entry.Blocks.Add(block);
                }
            }

            entry.Flags = reader.ReadByte();
            entry.CompressionBlockSize = reader.ReadUInt32();
            entry.PayloadOffset = checked((ulong)reader.BaseStream.Position);
            return entry;
        }

        private static void ValidateEntry(
            EncodedEntry encoded, DataEntry data, long pakLength)
        {
            bool compressed = encoded.CompressionSlot >= 0;
            if (data.Offset != 0
                || data.CompressedSize != encoded.CompressedSize
                || data.UncompressedSize != encoded.UncompressedSize
                || data.CompressionIndex !=
                    (uint)(encoded.CompressionSlot + 1)
                || ((data.Flags & 1) != 0) != encoded.Encrypted
                || data.Blocks.Count != encoded.CompressionBlockCount
                || (compressed
                    && data.CompressionBlockSize !=
                        encoded.CompressionBlockSize))
            {
                throw new InvalidDataException(
                    "The PAK entry metadata is inconsistent.");
            }

            if (!compressed)
            {
                ulong storedLength = encoded.Encrypted
                    ? (ulong)Align16(checked((int)data.UncompressedSize))
                    : data.UncompressedSize;
                if (data.PayloadOffset + storedLength > (ulong)pakLength)
                {
                    throw new InvalidDataException(
                        "The uncompressed PAK entry is outside the file.");
                }
                return;
            }

            ulong totalCompressed = 0;
            foreach (DataBlock block in data.Blocks)
            {
                if (block.End <= block.Start
                    || encoded.Offset + block.End > (ulong)pakLength)
                {
                    throw new InvalidDataException(
                        "A PAK compression block is invalid.");
                }
                totalCompressed += block.End - block.Start;
            }
            if (totalCompressed != data.CompressedSize)
            {
                throw new InvalidDataException(
                    "The PAK compressed size is inconsistent.");
            }
        }

        private static void WriteDecompressedEntry(
            BinaryReader pak,
            DataEntry data,
            byte[] aesKey,
            string oodleLibraryPath,
            string outputPath)
        {
            if (data.CompressionIndex == 0)
            {
                WriteUncompressedEntry(
                    pak,
                    data,
                    aesKey,
                    outputPath);
                return;
            }
            if (data.CompressionIndex != 1)
            {
                throw new NotSupportedException(
                    "The requested game-data entry uses an unsupported compression method.");
            }

            using (FileStream output = File.Create(outputPath))
            {
                ulong remaining = data.UncompressedSize;
                for (int index = 0; index < data.Blocks.Count; index++)
                {
                    DataBlock block = data.Blocks[index];
                    int compressedLength = checked(
                        (int)(block.End - block.Start));
                    int storedLength = (data.Flags & 1) != 0
                        ? Align16(compressedLength)
                        : compressedLength;
                    pak.BaseStream.Position = checked(
                        (long)(block.Start + data.PakOffset));
                    byte[] compressed = pak.ReadBytes(storedLength);
                    if (compressed.Length != storedLength)
                    {
                        throw new EndOfStreamException();
                    }
                    if ((data.Flags & 1) != 0)
                    {
                        compressed = DecryptAes(compressed, aesKey)
                            .Take(compressedLength)
                            .ToArray();
                    }

                    int rawLength = checked((int)Math.Min(
                        remaining, data.CompressionBlockSize));
                    string token = Guid.NewGuid().ToString("N", CultureInfo.InvariantCulture);
                    string packedPath = Path.Combine(Path.GetTempPath(), "eventradar-" + token + ".ooz");
                    string rawPath = Path.Combine(Path.GetTempPath(), "eventradar-" + token + ".raw");
                    try
                    {
                        using (FileStream blockFile = File.Create(packedPath))
                        using (BinaryWriter writer = new BinaryWriter(blockFile))
                        {
                            writer.Write((ulong)rawLength);
                            writer.Write(compressed);
                        }
                        RunOoz(oodleLibraryPath, packedPath, rawPath);
                        byte[] raw = File.ReadAllBytes(rawPath);
                        if (raw.Length != rawLength)
                        {
                            throw new InvalidDataException("An Oodle block has an unexpected output size.");
                        }
                        output.Write(raw, 0, raw.Length);
                    }
                    finally
                    {
                        try { if (File.Exists(packedPath)) File.Delete(packedPath); } catch { }
                        try { if (File.Exists(rawPath)) File.Delete(rawPath); } catch { }
                    }
                    remaining -= (ulong)rawLength;
                }

                if (remaining != 0
                    || (ulong)output.Length != data.UncompressedSize)
                {
                    throw new InvalidDataException(
                        "The game-data extraction is incomplete.");
                }
            }
        }


        private static void WriteUncompressedEntry(
            BinaryReader pak,
            DataEntry data,
            byte[] aesKey,
            string outputPath)
        {
            int rawLength = checked((int)data.UncompressedSize);
            int storedLength = (data.Flags & 1) != 0
                ? Align16(rawLength)
                : rawLength;
            pak.BaseStream.Position = checked((long)data.PayloadOffset);
            byte[] raw = pak.ReadBytes(storedLength);
            if (raw.Length != storedLength)
            {
                throw new EndOfStreamException();
            }
            if ((data.Flags & 1) != 0)
            {
                raw = DecryptAes(raw, aesKey)
                    .Take(rawLength)
                    .ToArray();
            }
            File.WriteAllBytes(outputPath, raw);
        }

        private static void RunOoz(string oozPath, string packedPath, string rawPath)
        {
            ProcessStartInfo start = new ProcessStartInfo();
            start.FileName = oozPath;
            start.Arguments = "-q -f " + QuoteArgument(packedPath) + " " + QuoteArgument(rawPath);
            start.UseShellExecute = false;
            start.CreateNoWindow = true;
            start.RedirectStandardError = true;
            using (Process process = Process.Start(start))
            {
                string error = process.StandardError.ReadToEnd();
                process.WaitForExit();
                if (process.ExitCode != 0 || !File.Exists(rawPath))
                {
                    throw new InvalidDataException("Oodle decompression failed. " + error.Trim());
                }
            }
        }

        private static string QuoteArgument(string value)
        {
            return "\"" + value.Replace("\"", "\\\"") + "\"";
        }

        private static PakFooter ReadFooter(BinaryReader reader)
        {
            if (reader.BaseStream.Length < PakFooterSize)
            {
                throw new InvalidDataException("The PAK is too small.");
            }
            reader.BaseStream.Position =
                reader.BaseStream.Length - PakFooterSize;
            reader.ReadBytes(16);
            bool encrypted = reader.ReadByte() != 0;
            uint magic = reader.ReadUInt32();
            uint version = reader.ReadUInt32();
            ulong indexOffset = reader.ReadUInt64();
            ulong indexSize = reader.ReadUInt64();
            if (magic != PakMagic || version != DragonSwordPakVersion)
            {
                throw new InvalidDataException(
                    "The DragonSword PAK format is not supported.");
            }
            if (indexOffset + indexSize >
                (ulong)reader.BaseStream.Length)
            {
                throw new InvalidDataException(
                    "The PAK index is outside the file.");
            }
            PakFooter footer = new PakFooter();
            footer.Encrypted = encrypted;
            footer.IndexOffset = indexOffset;
            footer.IndexSize = indexSize;
            return footer;
        }

        private static byte[] ReadAt(
            BinaryReader reader, ulong offset, ulong size)
        {
            if (size > Int32.MaxValue)
            {
                throw new InvalidDataException("The PAK index is too large.");
            }
            reader.BaseStream.Position = checked((long)offset);
            byte[] result = reader.ReadBytes(checked((int)size));
            if ((ulong)result.Length != size)
            {
                throw new EndOfStreamException();
            }
            return result;
        }

        private static byte[] DecryptAes(
            byte[] encrypted, byte[] key)
        {
            if (encrypted.Length % 16 != 0)
            {
                throw new InvalidDataException(
                    "Encrypted PAK data is not AES aligned.");
            }
            using (Aes aes = Aes.Create())
            {
                aes.Mode = CipherMode.ECB;
                aes.Padding = PaddingMode.None;
                aes.Key = key;
                using (ICryptoTransform decryptor = aes.CreateDecryptor())
                {
                    return decryptor.TransformFinalBlock(
                        encrypted, 0, encrypted.Length);
                }
            }
        }

        private static byte InferNumberMask(byte[] index)
        {
            if (index.Length < 8
                || index[1] != index[2]
                || index[2] != index[3])
            {
                throw new InvalidDataException(
                    "The DragonSword PAK index mask is invalid.");
            }
            return index[1];
        }

        private static int Align16(int value)
        {
            return checked((value + 15) & ~15);
        }

    }
}

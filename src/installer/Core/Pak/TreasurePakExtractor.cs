using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Diagnostics;

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

using System;
using System.Collections.Generic;
using System.IO;
using System.Globalization;
using System.Text;

namespace DragonSwordWorldRadar.Installer
{
    public static class OwnerPointerRvaResolver
    {
        private static readonly byte[] Pattern =
        {
            0x48, 0x8B, 0x0D, 0, 0, 0, 0,
            0xE8, 0, 0, 0, 0,
            0x8B, 0xC7, 0x48, 0x8B, 0x5C, 0x24,
            0x40, 0x48, 0x8B, 0x6C, 0x24, 0x50,
        };

        private const string Mask = "xxx????x????xxxxxxxxxxxx";

        public static ulong Resolve(string executablePath)
        {
            if (String.IsNullOrWhiteSpace(executablePath))
            {
                throw new InvalidDataException(
                    "Owner-pointer executable path is missing.");
            }
            return ResolveImage(File.ReadAllBytes(executablePath));
        }

        public static ulong GenerateBoundConfig(
            string executablePath,
            string outputPath,
            string gameFingerprint)
        {
            if (String.IsNullOrWhiteSpace(gameFingerprint)
                || gameFingerprint.Length != 64)
            {
                throw new InvalidDataException(
                    "Owner-pointer game fingerprint is invalid.");
            }
            ulong rva = Resolve(executablePath);
            long executableLength = new FileInfo(executablePath).Length;
            if (rva == 0 || rva >= (ulong)executableLength)
            {
                throw new InvalidDataException(
                    "Resolved owner-pointer RVA is outside executable bounds.");
            }

            Directory.CreateDirectory(Path.GetDirectoryName(outputPath));
            string temporary = outputPath + "." +
                Guid.NewGuid().ToString("N") + ".tmp";
            string text = String.Join(Environment.NewLine, new[]
            {
                "# DragonSwordWorldRadar installer-generated runtime data.",
                "# Contains no SQLCipher key or process address.",
                "schema_version=1",
                "game_fingerprint=" + gameFingerprint,
                "executable_length=" + executableLength.ToString(
                    CultureInfo.InvariantCulture),
                "owner_pointer_rva=0x" + rva.ToString("X"),
                "provenance=install-time-exact-executable-pattern",
                String.Empty,
            });
            File.WriteAllText(
                temporary,
                text,
                new UTF8Encoding(false));
            try
            {
                if (File.Exists(outputPath))
                {
                    File.Replace(temporary, outputPath, null);
                }
                else
                {
                    File.Move(temporary, outputPath);
                }
            }
            finally
            {
                if (File.Exists(temporary))
                {
                    File.Delete(temporary);
                }
            }
            return rva;
        }

        public static ulong ResolveImage(byte[] image)
        {
            if (image == null || image.Length < 0x40
                || image[0] != (byte)'M' || image[1] != (byte)'Z')
            {
                throw new InvalidDataException(
                    "Owner-pointer source is not a valid DOS/PE image.");
            }

            int peOffset = BitConverter.ToInt32(image, 0x3C);
            if (peOffset <= 0 || peOffset > image.Length - 24
                || BitConverter.ToUInt32(image, peOffset) != 0x00004550)
            {
                throw new InvalidDataException(
                    "Owner-pointer source has an invalid PE header.");
            }

            int sectionCount = BitConverter.ToUInt16(image, peOffset + 6);
            int optionalSize = BitConverter.ToUInt16(image, peOffset + 20);
            int optionalOffset = checked(peOffset + 24);
            int sectionTable = checked(optionalOffset + optionalSize);
            if (sectionCount <= 0 || sectionCount > 96
                || optionalSize < 60
                || sectionTable < optionalOffset
                || sectionTable > image.Length
                || sectionCount > (image.Length - sectionTable) / 40)
            {
                throw new InvalidDataException(
                    "Owner-pointer PE section table is invalid.");
            }

            uint sizeOfImage = BitConverter.ToUInt32(
                image,
                optionalOffset + 56);
            if (sizeOfImage == 0)
            {
                throw new InvalidDataException(
                    "Owner-pointer PE image size is invalid.");
            }

            List<ulong> matches = new List<ulong>();
            for (int index = 0; index < sectionCount; index++)
            {
                int header = checked(sectionTable + index * 40);
                uint virtualAddress = BitConverter.ToUInt32(
                    image,
                    header + 12);
                uint rawSize = BitConverter.ToUInt32(image, header + 16);
                uint rawOffset = BitConverter.ToUInt32(image, header + 20);
                if (rawSize == 0)
                {
                    continue;
                }
                if (rawOffset > image.Length
                    || rawSize > image.Length - rawOffset)
                {
                    throw new InvalidDataException(
                        "Owner-pointer PE section exceeds file bounds.");
                }

                int start = checked((int)rawOffset);
                int end = checked(start + (int)rawSize - Pattern.Length);
                for (int offset = start; offset <= end; offset++)
                {
                    if (!MatchesAt(image, offset))
                    {
                        continue;
                    }
                    int displacement = BitConverter.ToInt32(
                        image,
                        offset + 3);
                    long instructionRva = checked(
                        (long)virtualAddress + offset - start);
                    long targetRva = checked(
                        instructionRva + 7L + displacement);
                    if (targetRva <= 0 || targetRva >= sizeOfImage)
                    {
                        throw new InvalidDataException(
                            "Owner-pointer target RVA is outside the PE image.");
                    }
                    matches.Add((ulong)targetRva);
                }
            }

            if (matches.Count == 0)
            {
                throw new InvalidDataException(
                    "Owner-pointer pattern was not found in the executable.");
            }
            if (matches.Count != 1)
            {
                throw new InvalidDataException(
                    "Owner-pointer pattern is ambiguous; matches=" +
                    matches.Count + ".");
            }
            return matches[0];
        }

        private static bool MatchesAt(byte[] image, int offset)
        {
            for (int index = 0; index < Pattern.Length; index++)
            {
                if (Mask[index] != '?'
                    && image[offset + index] != Pattern[index])
                {
                    return false;
                }
            }
            return true;
        }
    }
}

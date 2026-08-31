using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace DragonSwordWorldRadar.Installer
{
    public sealed class MoleDataProvider : IDataProvider
    {
        private const string PakFileName =
            "pakchunk109-WindowsClient.pak";
        private static readonly Regex RewardTreasurePattern = new Regex(
            @"save_id\s*=\s*(?<save>\d+).*uid_name\s*=\s*""DT_MiniGame_G5_(?<game>\d+)""",
            RegexOptions.CultureInvariant);
        private static readonly Regex MoleRecordPattern = new Regex(
            @"mini_game_id\s*=\s*(?<game>\d+)\s*,\s*mask_bit",
            RegexOptions.CultureInvariant);
        public string Id
        {
            get { return "moles"; }
        }

        public DataSetResult Generate(InstallationContext context)
        {
            string pakPath = Path.Combine(
                context.PakRoot,
                PakFileName);
            string sourceDirectory = Path.Combine(
                context.TemporaryRoot,
                "mole-xml");
            string outputPath = Path.Combine(
                context.GeneratedDataRoot,
                "moles.lua");

            Directory.CreateDirectory(sourceDirectory);
            Directory.CreateDirectory(context.GeneratedDataRoot);

            // Coordinates come only from the user's current game PAK during
            // installation. Only ordinary Fly mini-games are emitted
            // here because their start points differ from treasure locations;
            // hide-and-seek or treasure-aligned Mole variants stay covered by
            // treasure markers. The generator resolves the start point with
            // role priority NPC_Start -> Teleport_Start -> first Mole-linked
            // position row from the extracted XML support set.
            string[] xmlPaths = TreasurePakExtractor.ExtractMatching(
                context.ExecutablePath,
                pakPath,
                context.OodleLibraryPath,
                sourceDirectory,
                new[]
                {
                    "MiniGameData.xml",
                    "SectionMiniGameData.xml",
                    "ActorPositionData.xml",
                    "SectionActorData.xml",
                    "MiniGameWorld",
                    "MiniGamePlace",
                    "SectionMiniGame",
                    "MiniGameFly",
                    "Fly",
                    "MiniGameMole",
                    "Mole",
                    "ActorPosition",
                    "MiniGame",
                },
                96);

            string stagedOutputPath = outputPath + ".pending";
            DeleteIfPresent(stagedOutputPath);
            int recordCount;
            try
            {
                // Keep the last-known-good catalog untouched until the new
                // local-PAK dataset has passed every classification, role and
                // count check. The candidate TSV is still written beside the
                // final catalog when diagnosis is needed.
                recordCount = MoleLuaGenerator.Generate(
                    xmlPaths,
                    stagedOutputPath);
                if (recordCount != 33)
                {
                    throw new InvalidDataException(
                    "The base Fly catalog must contain exactly 33 ordinary-world records.");
                }
                AdditionalMiniGameLuaGenerator.Append(xmlPaths, stagedOutputPath);
                recordCount = 83;
                BindRewardTreasureIds(
                    stagedOutputPath,
                    Path.Combine(context.GeneratedDataRoot, "treasures.lua"));
                File.Copy(stagedOutputPath, outputPath, true);
            }
            finally
            {
                DeleteIfPresent(stagedOutputPath);
            }

            return new DataSetResult
            {
                Id = Id,
                OutputPath = outputPath,
                SourcePak = pakPath,
                SourceEntry =
                    "validated local-Pak Fly start points (NPC_Start -> Teleport_Start fallback)",
                RecordCount = recordCount,
            };
        }

        private static void BindRewardTreasureIds(
            string moleCatalogPath,
            string treasureCatalogPath)
        {
            if (!File.Exists(treasureCatalogPath))
            {
                throw new InvalidDataException(
                    "The current-game treasure catalog is required before Mole reward mapping.");
            }

            Dictionary<int, long> rewards = new Dictionary<int, long>();
            HashSet<long> rewardSaveIds = new HashSet<long>();
            foreach (string line in File.ReadLines(treasureCatalogPath))
            {
                Match match = RewardTreasurePattern.Match(line);
                int gameId;
                long saveId;
                if (!match.Success
                    || !Int32.TryParse(match.Groups["game"].Value,
                        NumberStyles.Integer, CultureInfo.InvariantCulture,
                        out gameId)
                    || !Int64.TryParse(match.Groups["save"].Value,
                        NumberStyles.Integer, CultureInfo.InvariantCulture,
                        out saveId)
                    || saveId <= 0)
                {
                    continue;
                }
                // The current PAK labels Wave 13008's reward asset as 13009,
                // while its authoritative save ID remains 13008 and its
                // activity/start record is 13008. Bind only this exact proven
                // tuple; never apply a general numeric offset.
                if (gameId == 13009 && saveId == 13008)
                {
                    gameId = 13008;
                }
                if (!IsSupportedMiniGameId(gameId))
                {
                    continue;
                }
                if (rewards.ContainsKey(gameId)
                    || !rewardSaveIds.Add(saveId))
                {
                    throw new InvalidDataException(
                        "The current-game Mole reward treasure mapping contains duplicates.");
                }
                rewards.Add(gameId, saveId);
            }
            long sharedWaveReward;
            if (!rewards.TryGetValue(13008, out sharedWaveReward)
                || sharedWaveReward != 13008)
            {
                throw new InvalidDataException("The shared Wave 13008-13010 reward tuple is missing or changed.");
            }
            rewards.Add(13009, sharedWaveReward);
            rewards.Add(13010, sharedWaveReward);
            if (rewards.Count != 83)
            {
                throw new InvalidDataException(
                    "The current-game treasure table must provide exactly 83 supported MiniGame activity mappings; found "
                    + rewards.Count.ToString(CultureInfo.InvariantCulture) + ".");
            }

            string[] lines = File.ReadAllLines(moleCatalogPath);
            int bound = 0;
            for (int index = 0; index < lines.Length; index++)
            {
                Match match = MoleRecordPattern.Match(lines[index]);
                int gameId;
                if (!match.Success
                    || !Int32.TryParse(match.Groups["game"].Value,
                        NumberStyles.Integer, CultureInfo.InvariantCulture,
                        out gameId))
                {
                    continue;
                }
                long rewardSaveId;
                if (!rewards.TryGetValue(gameId, out rewardSaveId))
                {
                    throw new InvalidDataException(
                        "No current-game reward treasure maps to Fly "
                        + gameId.ToString(CultureInfo.InvariantCulture) + ".");
                }
                string replacement = "mini_game_id = "
                    + gameId.ToString(CultureInfo.InvariantCulture)
                    + ", reward_save_id = "
                    + rewardSaveId.ToString(CultureInfo.InvariantCulture)
                    + ", mask_bit";
                lines[index] = MoleRecordPattern.Replace(
                    lines[index], replacement, 1);
                bound++;
            }
            if (bound != 83)
            {
                throw new InvalidDataException(
                    "Exactly 83 generated mini-game records must receive current-game reward mappings.");
            }
            File.WriteAllLines(
                moleCatalogPath,
                lines,
                new UTF8Encoding(false));
        }

        private static bool IsSupportedMiniGameId(int id)
        {
            return (id >= 11001 && id <= 11034 && id != 11024)
                || (id >= 12001 && id <= 12040)
                || (id >= 13001 && id <= 13010);
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
                // A stale pending file is never a valid published catalog.
            }
        }
    }
}

using System;
using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace DragonSwordWorldRadar.Installer
{
    public sealed class AssaultDataProvider : IDataProvider
    {
        private const string Pak = "pakchunk109-WindowsClient.pak";
        public string Id { get { return "assaults"; } }

        public DataSetResult Generate(InstallationContext context)
        {
            string pak = Path.Combine(context.PakRoot, Pak);
            string dir = Path.Combine(context.TemporaryRoot, "assault-xml");
            Directory.CreateDirectory(dir);
            Directory.CreateDirectory(context.GeneratedDataRoot);
            string place = ExtractOne(context, pak, dir, "UnexpectedMissionPlaceData.xml");
            string kind = ExtractOne(context, pak, dir, "UnexpectedMissionKindData.xml");
            string monster = ExtractOne(context, pak, dir, "SectionMonsterData.xml");
            string character = ExtractOne(context, pak, dir, "MonsterCharacterData.xml");
            string reveal = ExtractOne(context, pak, dir, "RevealCycleData.xml");
            string output = Path.Combine(context.GeneratedDataRoot, "assaults.lua");
            string policy = Path.Combine(context.ModRoot, "metadata", "assault-inference-policy.xml");
            int count = AssaultLuaGenerator.Generate(place, kind, monster, reveal, policy, context.GameFingerprint, output);
            if (count != 40) throw new InvalidDataException("The generated Assault catalog must contain exactly 40 records.");
            List<KeyValuePair<string, string>> mappings = new List<KeyValuePair<string, string>>();
            foreach (Match match in Regex.Matches(File.ReadAllText(output),
                @"\bplace_id\s*=\s*(\d+)[^\r\n]*\bcid\s*=\s*(\d+)"))
                mappings.Add(new KeyValuePair<string, string>(match.Groups[1].Value, match.Groups[2].Value));
            string actorOutput = Path.Combine(context.GeneratedDataRoot, "assault-actors.tsv");
            int actorCount = EncounterActorCatalogGenerator.GenerateMappings(monster, character, mappings, actorOutput);
            if (actorCount != 40) throw new InvalidDataException("The native Assault actor catalog must contain exactly 40 records.");
            return new DataSetResult { Id = Id, OutputPath = output, SourcePak = pak,
                SourceEntry = "UnexpectedMissionPlaceData.xml + UnexpectedMissionKindData.xml + SectionMonsterData.xml + MonsterCharacterData.xml + RevealCycleData.xml",
                RecordCount = count, GameFingerprint = context.GameFingerprint };

        }

        private static string ExtractOne(InstallationContext context, string pak, string directory, string name)
        {
            string[] paths = TreasurePakExtractor.ExtractMatching(context.ExecutablePath, pak,
                context.OodleLibraryPath, directory, new[] { name }, 8);
            string selected = null;
            foreach (string path in paths)
            {
                string fileName = Path.GetFileName(path);
                if (String.Equals(fileName, name, StringComparison.OrdinalIgnoreCase)
                    || fileName.EndsWith("_" + name, StringComparison.OrdinalIgnoreCase))
                {
                    if (selected != null) throw new InvalidDataException("Assault source entry is not unique: " + name + ".");
                    selected = path;
                }
            }
            if (selected != null) return selected;
            throw new InvalidDataException("Assault source extraction failed for " + name + ".");
        }
    }
}

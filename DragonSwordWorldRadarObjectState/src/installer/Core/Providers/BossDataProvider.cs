using System.IO;
using System.Collections.Generic;

namespace DragonSwordWorldRadar.Installer
{
    public sealed class BossDataProvider : IDataProvider
    {
        private const string PakFileName =
            "pakchunk109-WindowsClient.pak";
        private const string SectionMonsterEntry =
            "SectionMonsterData.xml";
        private const string MonsterCharacterEntry = "MonsterCharacterData.xml";

        public string Id
        {
            get { return "bosses"; }
        }

        public DataSetResult Generate(InstallationContext context)
        {
            string pakPath = Path.Combine(
                context.PakRoot,
                PakFileName);
            string sectionMonsterXml = Path.Combine(
                context.TemporaryRoot,
                SectionMonsterEntry);
            string outputPath = Path.Combine(
                context.GeneratedDataRoot,
                "bosses.lua");
            string characterXml = Path.Combine(context.TemporaryRoot, MonsterCharacterEntry);
            string actorOutput = Path.Combine(context.GeneratedDataRoot, "boss-actors.tsv");

            Directory.CreateDirectory(context.TemporaryRoot);
            Directory.CreateDirectory(context.GeneratedDataRoot);
            TreasurePakExtractor.Extract(
                context.ExecutablePath,
                pakPath,
                context.OodleLibraryPath,
                SectionMonsterEntry,
                sectionMonsterXml);
            TreasurePakExtractor.Extract(context.ExecutablePath, pakPath,
                context.OodleLibraryPath, MonsterCharacterEntry, characterXml);

            int recordCount = BossLuaGenerator.Generate(
                sectionMonsterXml,
                outputPath);
            if (recordCount != 9)
            {
                throw new InvalidDataException(
                    "The generated world-boss catalog must contain 9 records.");
            }
            int actorCount = EncounterActorCatalogGenerator.Generate(sectionMonsterXml,
                characterXml, new[] { "9000010", "9000011", "9000005", "9000012", "9000007",
                    "9000019", "9000022", "9000023", "9000025" }, actorOutput);
            if (actorCount != 9) throw new InvalidDataException("The native world-boss actor catalog must contain 9 records.");

            return new DataSetResult
            {
                Id = Id,
                OutputPath = outputPath,
                SourcePak = pakPath,
                SourceEntry = SectionMonsterEntry + " + " + MonsterCharacterEntry,
                RecordCount = recordCount,
            };
        }
    }
}

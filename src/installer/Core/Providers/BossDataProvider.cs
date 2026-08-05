using System.IO;

namespace DragonSwordWorldRadar.Installer
{
    public sealed class BossDataProvider : IDataProvider
    {
        private const string PakFileName =
            "pakchunk109-WindowsClient.pak";
        private const string SectionMonsterEntry =
            "SectionMonsterData.xml";

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

            Directory.CreateDirectory(context.TemporaryRoot);
            Directory.CreateDirectory(context.GeneratedDataRoot);
            TreasurePakExtractor.Extract(
                context.ExecutablePath,
                pakPath,
                context.OodleLibraryPath,
                SectionMonsterEntry,
                sectionMonsterXml);

            int recordCount = BossLuaGenerator.Generate(
                sectionMonsterXml,
                outputPath);
            if (recordCount != 9)
            {
                throw new InvalidDataException(
                    "The generated world-boss catalog must contain 9 records.");
            }

            return new DataSetResult
            {
                Id = Id,
                OutputPath = outputPath,
                SourcePak = pakPath,
                SourceEntry = SectionMonsterEntry,
                RecordCount = recordCount,
            };
        }
    }
}

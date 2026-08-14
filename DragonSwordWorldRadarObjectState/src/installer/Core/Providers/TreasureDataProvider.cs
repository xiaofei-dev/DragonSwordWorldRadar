using System;
using System.IO;

namespace DragonSwordWorldRadar.Installer
{
    public sealed class TreasureDataProvider : IDataProvider
    {
        private const string PakFileName =
            "pakchunk109-WindowsClient.pak";
        private const string SourceEntry =
            "SectionTreasureBoxData.xml";
        private const string BlueprintSourceEntry =
            "PropTreasureBoxData.xml";

        public string Id
        {
            get { return "treasures"; }
        }

        public DataSetResult Generate(InstallationContext context)
        {
            string pakPath = Path.Combine(
                context.PakRoot,
                PakFileName);
            string xmlPath = Path.Combine(
                context.TemporaryRoot,
                SourceEntry);
            string blueprintXmlPath = Path.Combine(
                context.TemporaryRoot,
                BlueprintSourceEntry);
            string outputPath = Path.Combine(
                context.GeneratedDataRoot,
                "treasures.lua");
            string actorOutputPath = Path.Combine(
                context.GeneratedDataRoot,
                "treasure-actors.tsv");

            Directory.CreateDirectory(context.TemporaryRoot);
            Directory.CreateDirectory(context.GeneratedDataRoot);
            TreasurePakExtractor.Extract(
                context.ExecutablePath,
                pakPath,
                context.OodleLibraryPath,
                xmlPath);
            TreasurePakExtractor.Extract(
                context.ExecutablePath,
                pakPath,
                context.OodleLibraryPath,
                BlueprintSourceEntry,
                blueprintXmlPath);
            int recordCount = TreasureLuaGenerator.Generate(
                xmlPath,
                outputPath);
            if (recordCount < 1000)
            {
                throw new InvalidDataException(
                    "The generated treasure catalog is incomplete.");
            }
            int actorRecordCount =
                TreasureActorCatalogGenerator.Generate(
                    xmlPath,
                    blueprintXmlPath,
                    actorOutputPath);
            if (recordCount - actorRecordCount < 0
                || recordCount - actorRecordCount > 2)
            {
                throw new InvalidDataException(
                    "The generated native treasure actor catalog is incomplete.");
            }

            return new DataSetResult
            {
                Id = Id,
                OutputPath = outputPath,
                SourcePak = pakPath,
                SourceEntry = SourceEntry,
                RecordCount = recordCount,
            };
        }
    }
}

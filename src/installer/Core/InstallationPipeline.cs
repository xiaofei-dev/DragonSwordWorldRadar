using System;
using System.Collections.Generic;
using System.IO;

namespace DragonSwordWorldRadar.Installer
{
    public static class InstallationPipeline
    {
        public static DataSetResult[] GenerateData(
            string modRoot,
            string gameRoot,
            string oodleLibraryPath)
        {
            InstallationContext context = new InstallationContext(
                modRoot,
                gameRoot,
                oodleLibraryPath);
            List<IDataProvider> providers = new List<IDataProvider>
            {
                new TreasureDataProvider(),
                new BossDataProvider(),
            };
            List<DataSetResult> results = new List<DataSetResult>();

            try
            {
                foreach (IDataProvider provider in providers)
                {
                    results.Add(provider.Generate(context));
                }
                return results.ToArray();
            }
            finally
            {
                TryDeleteDirectory(context.TemporaryRoot);
            }
        }

        private static void TryDeleteDirectory(string path)
        {
            try
            {
                if (Directory.Exists(path))
                {
                    Directory.Delete(path, true);
                }
            }
            catch
            {
                // Temporary cleanup failure does not invalidate generation.
            }
        }
    }
}

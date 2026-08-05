using System.IO;
using System.Text;

namespace DragonSwordWorldRadar
{
    internal static class SharedBridgeFile
    {
        public static string ReadAllText(string path)
        {
            using (FileStream stream = new FileStream(
                path,
                FileMode.Open,
                FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete))
            using (StreamReader reader = new StreamReader(
                stream,
                new UTF8Encoding(false, true),
                true))
            {
                return reader.ReadToEnd();
            }
        }
    }
}

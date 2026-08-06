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

        public static int ReadInto(string path, byte[] buffer)
        {
            if (buffer == null || buffer.Length == 0)
            {
                throw new System.ArgumentException(
                    "A non-empty bridge buffer is required.",
                    "buffer");
            }

            // Do not retain a read handle between polls. UE4SS writes the same
            // slots through Lua's io.open("w"), so a short-lived shared handle
            // keeps the producer/consumer interaction identical to stable6.
            using (FileStream stream = new FileStream(
                path,
                FileMode.Open,
                FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete))
            {
                int total = 0;
                while (total < buffer.Length)
                {
                    int read = stream.Read(
                        buffer,
                        total,
                        buffer.Length - total);
                    if (read == 0)
                    {
                        return total;
                    }
                    total += read;
                }
                if (stream.ReadByte() != -1)
                {
                    throw new InvalidDataException(
                        "Bridge record exceeds the fixed read buffer.");
                }
                return total;
            }
        }
    }
}

using System;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal static class ErrorLog
    {
        private static readonly object Sync = new object();
        private static bool _sessionStarted;

        public static readonly string UsePath = ModPath.RuntimePath(
            "logs",
            "DragonSwordNativeWorldRadar.Overlay.Use.log");
        public static readonly string DebugPath = ModPath.RuntimePath(
            "logs",
            "DragonSwordNativeWorldRadar.Overlay.Debug.log");

        // Program.cs uses Path in its fatal-error dialog. Keep it as the
        // low-volume operational log for backward source compatibility.
        public static readonly string Path = UsePath;

        public static void StartSession()
        {
            try
            {
                lock (Sync)
                {
                    if (_sessionStarted)
                    {
                        return;
                    }
                    Directory.CreateDirectory(
                        System.IO.Path.GetDirectoryName(UsePath));
                    RotateToPrevious(UsePath);
                    RotateToPrevious(DebugPath);
                    _sessionStarted = true;
                }
            }
            catch
            {
                // Session rotation is optional. Logging can still append to the
                // existing files if a scanner temporarily holds either path.
            }
        }

        public static void Write(
            string context,
            Exception exception)
        {
            string details = exception == null
                ? "(no exception details)"
                : exception.ToString();
            Append(
                UsePath,
                Timestamp() + " " + context + Environment.NewLine +
                details + Environment.NewLine + Environment.NewLine);
        }

        public static void WriteDebug(string message)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }
            Append(
                DebugPath,
                Timestamp() + " " + message +
                Environment.NewLine);
        }

        public static void WriteMessage(string message)
        {
            Append(
                UsePath,
                Timestamp() + " " + message +
                Environment.NewLine);
        }

        private static string Timestamp()
        {
            return DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
        }

        private static void RotateToPrevious(string path)
        {
            if (!File.Exists(path))
            {
                return;
            }

            string previousPath = path + ".previous.log";
            try
            {
                if (File.Exists(previousPath))
                {
                    File.Delete(previousPath);
                }
                File.Move(path, previousPath);
            }
            catch
            {
                // Retain and append to the current log when rotation is blocked.
            }
        }

        private static void Append(
            string path,
            string text)
        {
            try
            {
                lock (Sync)
                {
                    Directory.CreateDirectory(
                        System.IO.Path.GetDirectoryName(path));
                    File.AppendAllText(path, text);
                }
            }
            catch
            {
                // Logging must never terminate the overlay.
            }
        }
    }
}

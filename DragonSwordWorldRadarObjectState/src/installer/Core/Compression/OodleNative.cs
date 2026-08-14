using System;
using System.IO;
using System.Runtime.InteropServices;

namespace DragonSwordWorldRadar.Installer
{
    internal static class OodleNative
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate long OodleLzDecompressDelegate(
            IntPtr compressedBuffer,
            long compressedSize,
            IntPtr rawBuffer,
            long rawSize,
            int fuzzSafe,
            int checkCrc,
            int verbosity,
            IntPtr decodeBuffer,
            long decodeBufferSize,
            IntPtr callback,
            IntPtr callbackUserData,
            IntPtr decoderMemory,
            long decoderMemorySize,
            int threadPhase);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr LoadLibraryW(string fileName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr module, string name);

        [DllImport("kernel32.dll")]
        private static extern bool FreeLibrary(IntPtr module);

        public static byte[] Decompress(
            string libraryPath,
            byte[] compressed,
            int rawLength)
        {
            if (String.IsNullOrWhiteSpace(libraryPath)
                || !File.Exists(libraryPath))
            {
                throw new FileNotFoundException(
                    "The game Oodle library was not found.",
                    libraryPath);
            }
            if (compressed == null)
            {
                throw new ArgumentNullException("compressed");
            }
            if (rawLength <= 0)
            {
                throw new ArgumentOutOfRangeException("rawLength");
            }

            IntPtr module = LoadLibraryW(libraryPath);
            if (module == IntPtr.Zero)
            {
                throw new InvalidOperationException(
                    "Could not load the game Oodle library. Win32=" +
                    Marshal.GetLastWin32Error());
            }

            GCHandle compressedHandle = default(GCHandle);
            GCHandle rawHandle = default(GCHandle);
            try
            {
                IntPtr export = GetProcAddress(
                    module,
                    "OodleLZ_Decompress");
                if (export == IntPtr.Zero)
                {
                    throw new MissingMethodException(
                        "OodleLZ_Decompress was not exported by " +
                        Path.GetFileName(libraryPath) + ".");
                }

                OodleLzDecompressDelegate decompress =
                    (OodleLzDecompressDelegate)Marshal.GetDelegateForFunctionPointer(
                        export,
                        typeof(OodleLzDecompressDelegate));
                byte[] raw = new byte[rawLength];
                compressedHandle = GCHandle.Alloc(
                    compressed,
                    GCHandleType.Pinned);
                rawHandle = GCHandle.Alloc(raw, GCHandleType.Pinned);

                long result = decompress(
                    compressedHandle.AddrOfPinnedObject(),
                    compressed.LongLength,
                    rawHandle.AddrOfPinnedObject(),
                    raw.LongLength,
                    1,
                    0,
                    0,
                    IntPtr.Zero,
                    0,
                    IntPtr.Zero,
                    IntPtr.Zero,
                    IntPtr.Zero,
                    0,
                    3);
                if (result != rawLength)
                {
                    throw new InvalidDataException(
                        "Oodle decompression returned " + result +
                        " bytes; expected " + rawLength + ".");
                }
                return raw;
            }
            finally
            {
                if (rawHandle.IsAllocated)
                {
                    rawHandle.Free();
                }
                if (compressedHandle.IsAllocated)
                {
                    compressedHandle.Free();
                }
                FreeLibrary(module);
            }
        }
    }
}

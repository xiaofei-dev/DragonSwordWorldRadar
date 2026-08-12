using System;
using System.Diagnostics;
using System.Threading;
using System.Windows.Forms;

namespace DragonSwordWorldRadar
{
    public static class Program
    {
        public const string InstanceMutexName =
            @"Local\DragonSwordNativeWorldRadar.Overlay";

        [STAThread]
        public static void Run()
        {
            bool isFirstInstance;
            using (Mutex instanceMutex = new Mutex(
                true,
                InstanceMutexName,
                out isFirstInstance))
            {
                if (!isFirstInstance)
                {
                    return;
                }

                ErrorLog.StartSession();
                try
                {
                    RunApplication();
                }
                finally
                {
                    try
                    {
                        instanceMutex.ReleaseMutex();
                    }
                    catch (ApplicationException)
                    {
                        // The mutex was not owned when startup failed early.
                    }
                }
            }
        }

        private static void RunApplication()
        {
            DpiAwareness.Enable();
            Application.SetUnhandledExceptionMode(
                UnhandledExceptionMode.CatchException);
            Application.ThreadException += delegate(
                object sender,
                ThreadExceptionEventArgs eventArgs)
            {
                ErrorLog.Write(
                    "UI exception",
                    eventArgs.Exception);
            };
            AppDomain.CurrentDomain.UnhandledException += delegate(
                object sender,
                UnhandledExceptionEventArgs eventArgs)
            {
                ErrorLog.Write(
                    "Unhandled exception",
                    eventArgs.ExceptionObject as Exception);
            };

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            try
            {
                ErrorLog.WriteMessage(
                    "Overlay session started: processId=" +
                    Process.GetCurrentProcess().Id);
                Application.Run(new RadarForm());
            }
            catch (Exception exception)
            {
                ErrorLog.Write(
                    "Fatal startup exception",
                    exception);
                MessageBox.Show(
                    "Treasure Radar could not start.\r\n\r\n" +
                    exception.Message +
                    "\r\n\r\nDetails: " + ErrorLog.Path,
                    "DragonSwordNativeWorldRadar",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            finally
            {
                ErrorLog.WriteMessage("Overlay session stopped.");
            }
        }
    }
}

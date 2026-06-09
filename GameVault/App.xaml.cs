using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using Microsoft.Win32;

namespace GameVault;

public partial class App : System.Windows.Application
{
    private static readonly Mutex _mutex = new(true, "GameVault-SingleInstance");
    public static bool StartSilent { get; private set; }

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    private const int SW_RESTORE = 9;

    public App()
    {
        ShutdownMode = ShutdownMode.OnExplicitShutdown;

        StartSilent = Environment.GetCommandLineArgs().Contains("--silent");

        if (!_mutex.WaitOne(TimeSpan.Zero, true))
        {
            if (!StartSilent)
            {
                var existing = Process.GetProcessesByName(Process.GetCurrentProcess().ProcessName)
                    .FirstOrDefault(p => p.Id != Environment.ProcessId);

                if (existing != null)
                {
                    var handle = FindMainWindow((uint)existing.Id);
                    if (handle != IntPtr.Zero)
                    {
                        ShowWindow(handle, SW_RESTORE);
                        SetForegroundWindow(handle);
                    }
                }
            }

            Environment.Exit(0);
        }

        RegisterStartup();
    }

    private static IntPtr FindMainWindow(uint processId)
    {
        var found = IntPtr.Zero;

        EnumWindows((hWnd, lParam) =>
        {
            GetWindowThreadProcessId(hWnd, out var pid);
            if (pid == processId)
            {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);

        return found;
    }

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        var window = new MainWindow();

        if (StartSilent)
        {
            new System.Windows.Interop.WindowInteropHelper(window).EnsureHandle();
        }
        else
        {
            MainWindow = window;
            window.Show();
        }
    }

    private static void RegisterStartup()
    {
        using var key = Registry.CurrentUser.OpenSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Run", true);

        if (key != null)
        {
            var exePath = Environment.ProcessPath;
            if (!string.IsNullOrEmpty(exePath))
            {
                var value = $"\"{exePath}\" --silent";
                if (key.GetValue("GameVault") as string != value)
                    key.SetValue("GameVault", value);
            }
        }
    }
}

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

        var args = Environment.GetCommandLineArgs();
        StartSilent = args.Contains("--silent");

        if (args.Contains("--uninstall"))
        {
            RunUninstall();
            return;
        }

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

        RegisterUninstallEntry();
    }

    private static void RunUninstall()
    {
        var result = System.Windows.MessageBox.Show(
            "Are you sure you want to uninstall GameVault?\n\n" +
            "This will:\n" +
            "• Remove your saved game library\n" +
            "• Remove the autostart entry\n" +
            "• Delete GameVault.exe",
            "Uninstall GameVault",
            MessageBoxButton.YesNo, MessageBoxImage.Question);

        if (result != MessageBoxResult.Yes)
            Environment.Exit(0);

        var appData = System.IO.Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "GameVault");

        if (System.IO.Directory.Exists(appData))
        {
            try { System.IO.Directory.Delete(appData, true); }
            catch { }
        }

        using (var runKey = Registry.CurrentUser.OpenSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Run", true))
        {
            try { runKey?.DeleteValue("GameVault"); }
            catch { }
        }

        using (var uninstallKey = Registry.CurrentUser.OpenSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Uninstall", true))
        {
            try { uninstallKey?.DeleteSubKey("GameVault"); }
            catch { }
        }

        var exePath = Environment.ProcessPath;
        if (!string.IsNullOrEmpty(exePath))
        {
            var exeDir = System.IO.Path.GetDirectoryName(exePath);
            var batPath = System.IO.Path.Combine(
                System.IO.Path.GetTempPath(), "GameVault_cleanup.bat");

            var bat = $"@echo off\r\n" +
                      $":loop\r\n" +
                      $"taskkill /f /im \"{System.IO.Path.GetFileName(exePath)}\" >nul 2>&1\r\n" +
                      $"ping -n 3 127.0.0.1 >nul\r\n" +
                      $"if exist \"{exePath}\" del /f /q \"{exePath}\"\r\n" +
                      $"if exist \"{exePath}\" goto loop\r\n" +
                      $"if not \"{exeDir}\" == \"\" rmdir /s /q \"{exeDir}\" 2>nul\r\n" +
                      $"del /f /q \"{batPath}\"";

            System.IO.File.WriteAllText(batPath, bat);
            Process.Start(new ProcessStartInfo("cmd.exe", $"/c \"{batPath}\"")
            {
                WindowStyle = ProcessWindowStyle.Hidden,
                CreateNoWindow = true
            });
        }

        Environment.Exit(0);
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

    private static void RegisterUninstallEntry()
    {
        var exePath = Environment.ProcessPath;
        if (string.IsNullOrEmpty(exePath)) return;

        using var existing = Registry.CurrentUser.OpenSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Uninstall\GameVault");
        if (existing != null) return;

        using var key = Registry.CurrentUser.CreateSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Uninstall\GameVault");
        if (key == null) return;

        key.SetValue("DisplayName", "GameVault");
        key.SetValue("UninstallString", $"\"{exePath}\" --uninstall");
        key.SetValue("DisplayIcon", exePath);
        key.SetValue("DisplayVersion", "1.0.1");
        key.SetValue("Publisher", "ioaa909");
        key.SetValue("InstallLocation", System.IO.Path.GetDirectoryName(exePath) ?? "");
        key.SetValue("NoModify", 1, RegistryValueKind.DWord);
        key.SetValue("NoRepair", 1, RegistryValueKind.DWord);
    }
}

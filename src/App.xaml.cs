using System.Diagnostics;
using System.Threading;
using System.Windows;
using Microsoft.Win32;

namespace GameVault;

public partial class App : System.Windows.Application
{
    private static readonly Mutex _mutex = new(true, "GameVault-SingleInstance");
    public static readonly EventWaitHandle ShowSignal = new(false, EventResetMode.AutoReset, "GameVault-ShowSignal");
    public static bool StartSilent { get; private set; }

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
                try { ShowSignal.Set(); }
                catch { }
            }

            Environment.Exit(0);
        }

        RegisterStartup();
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
            var batPath = System.IO.Path.Combine(
                System.IO.Path.GetTempPath(), "GameVault_cleanup.bat");

            var bat = $"@echo off\r\n" +
                      $"chcp 65001 >nul\r\n" +
                      $":loop\r\n" +
                      $"taskkill /f /im \"{System.IO.Path.GetFileName(exePath)}\" >nul 2>&1\r\n" +
                      $"ping -n 3 127.0.0.1 >nul\r\n" +
                      $"del /f /q \"{exePath}\" >nul 2>&1\r\n" +
                      $"if exist \"{exePath}\" goto loop\r\n" +
                      $"rd /s /q \"{appData}\" >nul 2>&1\r\n" +
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

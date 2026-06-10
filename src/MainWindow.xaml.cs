using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Windows;
using System.Windows.Input;

using GameVault.Helpers;
using GameVault.Models;

namespace GameVault;

public partial class MainWindow : Window
{
    private readonly ObservableCollection<GameItem> _games = [];
    private static readonly string SavePath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "GameVault", "games.json");

    private System.Windows.Forms.NotifyIcon? _trayIcon;
    private System.Windows.Forms.ToolStripMenuItem _showItem = null!;
    private System.Windows.Forms.ToolStripMenuItem _exitItem = null!;
    private System.Drawing.Icon? _appIcon;
    private readonly List<(Process Process, string Name)> _runningGames = [];
    private readonly object _runningGamesLock = new();

    public MainWindow()
    {
        InitializeComponent();
        SetWindowIcon();
        GameList.ItemsSource = _games;
        LoadGames();
        SetupTrayIcon();

        if (App.StartSilent && _trayIcon != null)
            _trayIcon.Visible = true;

        new Thread(() =>
        {
            try
            {
                while (App.ShowSignal.WaitOne())
                    Dispatcher.Invoke(() => ShowWindow_Click(null, EventArgs.Empty));
            }
            catch { }
        })
        { IsBackground = true }.Start();
    }

    private void SetWindowIcon()
    {
        try
        {
            var cacheDir = System.IO.Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                "GameVault", "cache");
            System.IO.Directory.CreateDirectory(cacheDir);

            var pngPath = System.IO.Path.Combine(cacheDir, "GameVault.png");
            var icoPath = System.IO.Path.Combine(cacheDir, "GameVault.ico");

            using (var resource = System.Reflection.Assembly.GetExecutingAssembly()
                .GetManifestResourceStream("GameVault.GameVault.png"))
            {
                if (resource != null)
                {
                    using var file = System.IO.File.Create(pngPath);
                    resource.CopyTo(file);
                }
            }

            var pngInfo = new System.IO.FileInfo(pngPath);
            var icoInfo = new System.IO.FileInfo(icoPath);
            if (!icoInfo.Exists || pngInfo.LastWriteTime > icoInfo.LastWriteTime)
                GenerateIco(pngPath, icoPath);

            _appIcon = new System.Drawing.Icon(icoPath);
            Icon = System.Windows.Media.Imaging.BitmapFrame.Create(
                new Uri(icoPath, UriKind.Absolute));
        }
        catch { }
    }

    private static void GenerateIco(string pngPath, string icoPath)
    {
        using var original = new System.Drawing.Bitmap(pngPath);
        int[] sizes = [256, 48, 32, 16];

        using var icoStream = new System.IO.FileStream(icoPath, System.IO.FileMode.Create);
        using var bw = new System.IO.BinaryWriter(icoStream);

        bw.Write((short)0);          // reserved
        bw.Write((short)1);          // type: icon
        bw.Write((short)sizes.Length); // count

        var pngData = new List<byte[]>();
        int dataOffset = 6 + 16 * sizes.Length;

        foreach (var size in sizes)
        {
            using var frame = new System.Drawing.Bitmap(size, size);
            using (var g = System.Drawing.Graphics.FromImage(frame))
            {
                g.Clear(System.Drawing.Color.Transparent);
                float aspect = (float)original.Width / original.Height;
                System.Drawing.Rectangle srcRect;

                if (aspect > 1)
                {
                    float scale = (float)size / original.Height;
                    int srcW = (int)(size / scale);
                    srcRect = new System.Drawing.Rectangle((original.Width - srcW) / 2, 0, srcW, original.Height);
                }
                else
                {
                    float scale = (float)size / original.Width;
                    int srcH = (int)(size / scale);
                    srcRect = new System.Drawing.Rectangle(0, (original.Height - srcH) / 2, original.Width, srcH);
                }

                if (size == 16)
                {
                    using var intermediate = new System.Drawing.Bitmap(32, 32);
                    using (var ig = System.Drawing.Graphics.FromImage(intermediate))
                    {
                        ig.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                        ig.DrawImage(original, new System.Drawing.Rectangle(0, 0, 32, 32), srcRect, System.Drawing.GraphicsUnit.Pixel);
                    }
                    g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
                    g.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.Half;
                    g.DrawImage(intermediate, new System.Drawing.Rectangle(0, 0, 16, 16), 0, 0, 32, 32, System.Drawing.GraphicsUnit.Pixel);
                }
                else
                {
                    g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                    g.DrawImage(original, new System.Drawing.Rectangle(0, 0, size, size), srcRect, System.Drawing.GraphicsUnit.Pixel);
                }
            }

            using var ms = new System.IO.MemoryStream();
            frame.Save(ms, System.Drawing.Imaging.ImageFormat.Png);
            pngData.Add(ms.ToArray());
        }

        for (int i = 0; i < sizes.Length; i++)
        {
            int s = sizes[i];
            bw.Write((byte)(s >= 256 ? 0 : s));
            bw.Write((byte)(s >= 256 ? 0 : s));
            bw.Write((byte)0);  // colors
            bw.Write((byte)0);  // reserved
            bw.Write((short)1); // color planes
            bw.Write((short)32);// bits per pixel
            bw.Write(pngData[i].Length);
            bw.Write(dataOffset);
            dataOffset += pngData[i].Length;
        }

        foreach (var data in pngData)
            bw.Write(data);

        bw.Flush();
    }

    private void SetupTrayIcon()
    {
        _trayIcon = new System.Windows.Forms.NotifyIcon
        {
            Icon = _appIcon ?? System.Drawing.SystemIcons.Application,
            Text = "GameVault",
            Visible = false
        };

        _showItem = new System.Windows.Forms.ToolStripMenuItem("Show GameVault");
        _showItem.Click += ShowWindow_Click;

        _exitItem = new System.Windows.Forms.ToolStripMenuItem("Exit");
        _exitItem.Click += Exit_Click;

        _trayIcon.ContextMenuStrip = new System.Windows.Forms.ContextMenuStrip
        {
            ShowImageMargin = false,
            BackColor = System.Drawing.Color.FromArgb(30, 30, 30),
            ForeColor = System.Drawing.Color.White,
            Renderer = new System.Windows.Forms.ToolStripProfessionalRenderer(
                new DarkColorTable())
        };

        _trayIcon.DoubleClick += ShowWindow_Click;
        RebuildContextMenu();
    }

    private void RebuildContextMenu()
    {
        var menu = _trayIcon?.ContextMenuStrip;
        if (menu == null) return;

        menu.Items.Clear();

        List<(Process, string)> snapshot;
        lock (_runningGamesLock)
            snapshot = _runningGames.ToList();

        for (var i = snapshot.Count - 1; i >= 0; i--)
        {
            var (process, name) = snapshot[i];
            if (process.HasExited) continue;

            var killItem = new System.Windows.Forms.ToolStripMenuItem($"Kill {name}");
            var captured = process;
            killItem.Click += (_, _) => KillProcess(captured);
            menu.Items.Add(killItem);
        }

        menu.Items.Add(_showItem);
        menu.Items.Add(_exitItem);
    }

    private void KillProcess(Process process)
    {
        if (process.HasExited) return;

        try
        {
            process.Kill();
            process.Dispose();
            lock (_runningGamesLock)
                _runningGames.RemoveAll(r => r.Process == process);
            RebuildContextMenu();
            UpdateAddButton();
        }
        catch { }
    }

    private void KillAllGames()
    {
        List<(Process, string)> snapshot;
        lock (_runningGamesLock)
            snapshot = _runningGames.ToList();
        foreach (var (process, _) in snapshot)
            KillProcess(process);
    }

    private void CleanupTrayIcon()
    {
        if (_trayIcon != null)
        {
            _trayIcon.Visible = false;
            _trayIcon.Dispose();
            _trayIcon = null;
        }
    }

    private void TitleBar_MouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left)
            DragMove();
    }

    private void MinimizeBtn_Click(object sender, RoutedEventArgs e)
    {
        if (_trayIcon != null)
            _trayIcon.Visible = true;

        WindowState = WindowState.Minimized;
        Hide();
    }

    private void CloseBtn_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

    protected override void OnClosing(CancelEventArgs e)
    {
        SaveGames();
        KillAllGames();
        CleanupTrayIcon();
        System.Windows.Application.Current.Shutdown();
        base.OnClosing(e);
    }

    private void GameTile_Click(object sender, MouseButtonEventArgs e)
    {
        if (sender is not FrameworkElement element || element.DataContext is not GameItem game)
            return;

        bool alreadyRunning;
        lock (_runningGamesLock)
            alreadyRunning = _runningGames.Any(r => !r.Process.HasExited &&
                r.Process.MainModule?.FileName?.Equals(game.FilePath, StringComparison.OrdinalIgnoreCase) == true);

        if (alreadyRunning)
        {
            System.Windows.MessageBox.Show("This game is already running.", "Game Running",
                MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        Process? process = null;

        try
        {
            process = Process.Start(new ProcessStartInfo(game.FilePath)
            {
                UseShellExecute = false
            });
        }
        catch
        {
            try
            {
                process = Process.Start(new ProcessStartInfo(game.FilePath)
                {
                    UseShellExecute = true
                });
            }
            catch { }
        }

        if (process == null)
        {
            System.Windows.MessageBox.Show($"Could not launch:\n{game.FilePath}",
                "Launch Failed", MessageBoxButton.OK, MessageBoxImage.Error);
            return;
        }

        lock (_runningGamesLock)
            _runningGames.Add((process, game.Name));

        process.EnableRaisingEvents = true;
        process.Exited += (_, _) =>
        {
            Dispatcher.Invoke(() =>
            {
                lock (_runningGamesLock)
                    _runningGames.RemoveAll(r => r.Process == process);
                RebuildContextMenu();
                UpdateAddButton();
            });
        };
        RebuildContextMenu();
        UpdateAddButton();

        if (_trayIcon != null)
            _trayIcon.Visible = true;

        WindowState = WindowState.Minimized;
        Hide();
    }

    private void ShowWindow_Click(object? sender, EventArgs e)
    {
        if (System.Windows.Application.Current.MainWindow == null)
            System.Windows.Application.Current.MainWindow = this;

        if (_trayIcon != null)
            _trayIcon.Visible = false;

        Show();
        WindowState = WindowState.Normal;
        Activate();
    }

    private void Exit_Click(object? sender, EventArgs e)
    {
        CleanupTrayIcon();
        System.Windows.Application.Current.Shutdown();
    }

    private void UpdateAddButton()
    {
        bool anyRunning;
        lock (_runningGamesLock)
            anyRunning = _runningGames.Any(r => !r.Process.HasExited);

        if (anyRunning)
        {
            AddButton.Content = "Kill All Running Games";
            AddButton.Background = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(0xC0, 0x39, 0x2B));
        }
        else
        {
            AddButton.Content = "+ Add Games";
            AddButton.Background = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(0x7B, 0x2D, 0x8E));
        }
    }

    private void AddButton_Click(object sender, RoutedEventArgs e)
    {
        bool running;
        lock (_runningGamesLock)
            running = _runningGames.Any(r => !r.Process.HasExited);
        if (running)
        {
            KillAllGames();
            return;
        }

        var dialog = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Select game executables",
            Filter = "Executable files (*.exe)|*.exe",
            Multiselect = true,
            RestoreDirectory = true
        };

        if (dialog.ShowDialog() != true)
            return;

        var added = false;

        foreach (var filePath in dialog.FileNames)
        {
            if (_games.Any(g => g.FilePath.Equals(filePath, StringComparison.OrdinalIgnoreCase)))
                continue;

            var icon = IconHelper.ExtractIcon(filePath);
            var name = GetGameName(filePath);

            _games.Add(new GameItem
            {
                Name = name,
                FilePath = filePath,
                Icon = icon
            });

            added = true;
        }

        if (added)
            SaveGames();
    }

    private static string GetGameName(string filePath)
    {
        try
        {
            var info = FileVersionInfo.GetVersionInfo(filePath);
            return !string.IsNullOrWhiteSpace(info.FileDescription)
                ? info.FileDescription
                : !string.IsNullOrWhiteSpace(info.ProductName)
                    ? info.ProductName
                    : Path.GetFileNameWithoutExtension(filePath);
        }
        catch
        {
            return Path.GetFileNameWithoutExtension(filePath);
        }
    }

    private void LoadGames()
    {
        try
        {
            if (!File.Exists(SavePath))
                return;

            var json = File.ReadAllText(SavePath);
            var paths = JsonSerializer.Deserialize<List<string>>(json);
            if (paths == null) return;

            foreach (var path in paths)
            {
                if (!File.Exists(path)) continue;

                _games.Add(new GameItem
                {
                    Name = GetGameName(path),
                    FilePath = path,
                    Icon = IconHelper.ExtractIcon(path)
                });
            }
        }
        catch { }
    }

    private void SaveGames()
    {
        try
        {
            var dir = Path.GetDirectoryName(SavePath);
            if (dir != null) Directory.CreateDirectory(dir);

            var paths = _games.Select(g => g.FilePath).ToList();
            var json = JsonSerializer.Serialize(paths, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(SavePath, json);
        }
        catch { }
    }
}

public class DarkColorTable : System.Windows.Forms.ProfessionalColorTable
{
    public override System.Drawing.Color ToolStripDropDownBackground => System.Drawing.Color.FromArgb(30, 30, 30);
    public override System.Drawing.Color ImageMarginGradientBegin => System.Drawing.Color.FromArgb(30, 30, 30);
    public override System.Drawing.Color ImageMarginGradientMiddle => System.Drawing.Color.FromArgb(30, 30, 30);
    public override System.Drawing.Color ImageMarginGradientEnd => System.Drawing.Color.FromArgb(30, 30, 30);
    public override System.Drawing.Color MenuItemSelected => System.Drawing.Color.FromArgb(70, 70, 70);
    public override System.Drawing.Color MenuItemSelectedGradientBegin => System.Drawing.Color.FromArgb(70, 70, 70);
    public override System.Drawing.Color MenuItemSelectedGradientEnd => System.Drawing.Color.FromArgb(70, 70, 70);
    public override System.Drawing.Color MenuItemPressedGradientBegin => System.Drawing.Color.FromArgb(50, 50, 50);
    public override System.Drawing.Color MenuItemPressedGradientEnd => System.Drawing.Color.FromArgb(50, 50, 50);
    public override System.Drawing.Color MenuBorder => System.Drawing.Color.FromArgb(50, 50, 50);
}

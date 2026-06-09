# GameVault

A lightweight Windows desktop app to organize and launch your game library from one place. Built with C# WPF.

## Features

- **Game Library Management** – Add any `.exe` from your PC; games persist across sessions
- **Tray-Centric Workflow** – Launch a game → auto-minimizes to system tray. Click "Show GameVault" to return
- **Process Tracking** – Kill individual games or kill all running games from tray context menu or the main button
- **Silent Autostart** – Optionally launches with Windows (runs in tray, no window shown)
- **Single Instance** – Second launch brings the existing window to front
- **Custom Theme** – Dark background (#1A1A1A) with purple accents (#7B2D8E / #9B59B6)
- **Auto Icon Extraction** – Game icons are extracted directly from each executable

## Screenshots

*Coming soon*

## Download

| Version | Download |
|---------|----------|
| Latest  | See [Releases](https://github.com/YOUR_USERNAME/GameVault/releases) |

## Requirements

- Windows 10 or later
- [.NET 10.0 Runtime](https://dotnet.microsoft.com/en-us/download/dotnet/10.0)

## Quick Start

1. Download the latest release
2. Run `GameVault.exe`
3. Click **+ Add Games** and select your game `.exe` files
4. Click a game tile to launch it
5. The app minimizes to tray while your game runs

### Silent / Autostart Mode

When launched with the `--silent` flag, GameVault starts in the system tray without showing a window. This is configured automatically if you enable startup via the registry (`HKCU\Run`).

## Tech Stack

- **Language:** C#
- **Framework:** WPF (.NET 10.0)
- **Tray:** Windows Forms (`NotifyIcon`)
- **Icon Extraction:** `SHDefExtractIcon` / `SHGetFileInfo`
- **Persistence:** JSON (`%APPDATA%\GameVault\games.json`)

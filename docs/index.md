# GameVault

A lightweight Windows desktop app to organize and launch your game library from one place. Built with C# WPF.

## Features

- **Game Library Management** – Add any `.exe` from your PC; games persist across sessions
- **Tray-Centric Workflow** – Launch a game → auto-minimizes to system tray. Click "Show GameVault" to return
- **Process Tracking** – Kill individual games or kill all running games from tray context menu or the main button
- **Silent Autostart** – Optionally launches with Windows (runs in tray, no window shown)
- **Single Instance** – Second launch brings the existing window to front
- **Minimize to Tray** – Minimize button on title bar hides to system tray
- **Uninstaller** – Shows in Control Panel → Programs and Features; removes all data, registry entries, and itself
- **Custom Theme** – Dark background (#1A1A1A) with purple accents (#7B2D8E / #9B59B6)
- **Auto Icon Extraction** – Game icons are extracted directly from each executable

## Download

**PowerShell (one-liner):**
```powershell
irm https://github.com/ioaa909/GameVault/releases/download/v1.0.1/GameVault-v1.0.1.zip -OutFile GameVault.zip; Expand-Archive GameVault.zip -DestinationPath GameVault -Force; .\GameVault\GameVault.exe
```

**Browser:** Download [GameVault-v1.0.1.zip](https://github.com/ioaa909/GameVault/releases/download/v1.0.1/GameVault-v1.0.1.zip), extract, and run `GameVault.exe`.

No .NET runtime required — the exe is self-contained.

## Quick Start

1. Run the command above or download the latest release
2. Click **+ Add Games** and select your game `.exe` files
3. Click a game tile to launch it

## Tech Stack

- **Language:** C#
- **Framework:** WPF (.NET 10.0)
- **Tray:** Windows Forms (`NotifyIcon`)
- **Icon Extraction:** `SHDefExtractIcon` / `SHGetFileInfo`
- **Persistence:** JSON (`%APPDATA%\GameVault\games.json`)

# GameVault

A lightweight Windows desktop app to organize and launch your game library from one place. Built with C# WPF.

## Features

- **Game Library Management** – Add any `.exe`; games persist across sessions
- **Tray-Centric Workflow** – Launch a game → auto-minimizes to system tray
- **Process Tracking** – Kill individual games or all running games
- **Silent Autostart** – Launches with Windows (runs in tray, no window)
- **Single Instance** – Second launch brings existing window to front
- **Minimize to Tray** – Minimize button on title bar hides to system tray
- **Custom Dark Theme** – Black background with purple accents
- **Uninstaller** – Shows in Control Panel → Programs and Features; removes all data, registry entries, and itself

## Download

**PowerShell (one-liner):**
```powershell
irm https://github.com/ioaa909/GameVault/releases/download/v1.0.1/GameVault-v1.0.1.zip -o g.zip; Expand-Archive g.zip -De . -Fo; ri g.zip; .\GameVault.exe
```

**Browser:** Download [GameVault-v1.0.1.zip](https://github.com/ioaa909/GameVault/releases/download/v1.0.1/GameVault-v1.0.1.zip), extract, and run `GameVault.exe`.

No .NET runtime required — the exe is self-contained.

## Quick Start

1. Run the command above or download the latest release
2. Click **+ Add Games** and select your game `.exe` files
3. Click a game tile to launch it

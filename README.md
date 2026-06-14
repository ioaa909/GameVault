# GameVault

A lightweight Windows desktop app to organize and launch your game library from one place. Built with C++ Win32.

## Features

- **Game Library Management** – Add any `.exe`; games persist across sessions
- **Tray-Centric Workflow** – Launch a game → auto-minimizes to system tray
- **Process Tracking** – Kill individual games or all running games
- **Silent Autostart** – Launches with Windows (runs in tray, no window)
- **Single Instance** – Second launch brings existing window to front
- **Minimize to Tray** – Minimize button on title bar hides to system tray
- **Custom Dark Theme** – Black background with purple accents
- **Uninstaller** – Shows in Control Panel → Programs and Features; removes all data, registry entries, and itself
- **Search** – Live case-insensitive filter with magnifying glass icon
- **Multi-Select** – Add multiple games at once via the file dialog
- **Drag & Drop** – Drop `.exe` or `.lnk` files directly onto the window
- **Auto-Detect Launchers** – Detects installed games from Steam, GOG, Epic, Ubisoft, EA, Riot, Rockstar, Battle.net, and itch.io on first launch
- **Global Hotkey** – `Alt+Space` brings the window to front from anywhere
- **Unicode-Safe** – Full wide-character support for non-ASCII usernames and paths

## Download

**PowerShell (one-liner):**
```powershell
iwr https://github.com/ioaa909/GameVault/releases/download/v1.1.0/GameVault.exe -OutFile GameVault.exe; .\GameVault.exe
```

No .NET or runtime required — the exe is self-contained.

## Quick Start

1. Run the command above or download the latest release
2. Click **+ Add Games** and select your game `.exe` files
3. Click a game tile to launch it

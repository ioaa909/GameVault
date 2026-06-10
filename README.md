# GameVault

A lightweight Windows desktop app to organize and launch your game library from one place. Built with C# WPF.

## Features

- **Game Library Management** – Add any `.exe`; games persist across sessions
- **Tray-Centric Workflow** – Launch a game → auto-minimizes to system tray
- **Process Tracking** – Kill individual games or all running games
- **Silent Autostart** – Launches with Windows (runs in tray, no window)
- **Single Instance** – Second launch brings existing window to front
- **Custom Dark Theme** – Black background with purple accents

## Download & Run

```powershell
iwr https://github.com/ioaa909/GameVault/releases/download/v1.0.0/GameVault-v1.0.0.zip -OutFile GameVault.zip; Expand-Archive GameVault.zip -DestinationPath GameVault -Force; .\GameVault\GameVault.exe
```

Or download the zip manually from [Releases](https://github.com/ioaa909/GameVault/releases), extract, and run `GameVault.exe`. No .NET runtime required.

## Quick Start

1. Run the command above or download the latest release
2. Click **+ Add Games** and select your game `.exe` files
3. Click a game tile to launch it

### Install via Package Managers

**winget** (after submission accepted):
```powershell
winget install ioaa909.GameVault
```

**Chocolatey** (after submission accepted):
```powershell
choco install gamevault
```

# GameVault

A lightweight Windows desktop app to organize and launch your game library from one place. Built with C# WPF.

## Features

- **Game Library Management** – Add any `.exe`; games persist across sessions
- **Tray-Centric Workflow** – Launch a game → auto-minimizes to system tray
- **Process Tracking** – Kill individual games or all running games
- **Silent Autostart** – Launches with Windows (runs in tray, no window)
- **Single Instance** – Second launch brings existing window to front
- **Custom Dark Theme** – Black background with purple accents

## Quick Start

1. Download the latest [release](https://github.com/ioaa909/GameVault/releases)
2. Run `GameVault.exe`
3. Click **+ Add Games** and select your game `.exe` files
4. Click a game tile to launch it

## Requirements

- Windows 10+
- [.NET 10.0 Runtime](https://dotnet.microsoft.com/en-us/download/dotnet/10.0)

## Build

```
dotnet build GameVault\GameVault.csproj
```

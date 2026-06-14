#pragma once
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <psapi.h>
#include <uxtheme.h>
#include <objbase.h>
#include <shobjidl.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDI_MAIN 101
#define APP_NAME L"GameVault"
#define APP_VER L"1.0.1"
#define PUBLISHER L"ioaa909"
#define WM_TRAYICON (WM_APP + 1)
#define WM_SHOWSIGNAL (WM_APP + 2)
#define TILE_W 120
#define TILE_H 105
#define TILE_M 10
#define TB_H 32
#define BB_H 60
#define MAX_G 256
#define RESIZE_BORDER 6

struct GameEntry {
    std::wstring name, filePath;
    HICON icon;
};
struct RunningGame {
    HANDLE process;
    DWORD pid;
    std::wstring name;
};

extern HINSTANCE g_hInst;
extern HWND g_hWnd, g_hwndGame, g_hBtnAdd;
extern HICON g_hAppIcon;
extern NOTIFYICONDATA g_nid;
extern HANDLE g_hMutex;
extern std::vector<GameEntry> g_games;
extern std::vector<RunningGame> g_running;
extern int g_contentH, g_scrollY;
extern HBRUSH g_brBg, g_brTileBg, g_brTileBd, g_brBtn, g_brBtnH, g_brRed, g_brTb;
extern HFONT g_hFont, g_hFontSm, g_hFontB;
extern bool g_silent;
extern int g_hovTile, g_closeHov, g_minHov;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GameAreaProc(HWND, UINT, WPARAM, LPARAM);
void InitTheme();
void RebuildTiles();
void UpdAddBtn();
void AddGames();
void LaunchG(int i);
void KillG(int i);
void KillAll();
std::wstring GetGN(const std::wstring& p);
HICON GetIcon(const std::wstring& p);
void Save();
void Load();
void TraySetup();
void TrayMenuBuild();
void ShowWin();
void RunUninst();
void RegStartup();
void RegUninst();
void ListenShow();
std::wstring AppDataP();
std::wstring SaveP();
std::wstring ShortP(const std::wstring& p);
std::string W2U(const std::wstring& w);
std::wstring U2W(const std::string& u);
std::string JEsc(const std::wstring& s);
void Center(HWND h);
void PaintTiles(HWND hwnd, HDC hdc, int w, int h);
int TileAt(int x, int y, int& cols, int& rows);
void AddGameIfNew(const std::wstring& name, const std::wstring& path);
std::wstring FindExe(const std::wstring& folder);
std::wstring FindExeRec(const std::wstring& folder, int depth=3);
std::wstring JsonGetStr(const std::string& json, const std::string& key);
std::wstring VdfGetStr(const std::string& vdf, const std::string& key);
void DetectSteam();
void DetectGOG();
void DetectEpic();
void DetectUbisoft();
void DetectEA();
void DetectRiot();
void DetectRockstar();
void DetectBattleNet();
void DetectItch();
void DetectFromLaunchers();

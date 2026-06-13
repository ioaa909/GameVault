#include "GameVault.h"

HINSTANCE g_hInst;
HWND g_hWnd, g_hwndGame, g_hBtnAdd;
HICON g_hAppIcon;
NOTIFYICONDATA g_nid = {};
HANDLE g_hMutex;
std::vector<GameEntry> g_games;
std::vector<RunningGame> g_running;
int g_contentH = 0, g_scrollY = 0;
HBRUSH g_brBg, g_brTileBg, g_brTileBd, g_brBtn, g_brBtnH, g_brRed, g_brTb;
HFONT g_hFont, g_hFontSm, g_hFontB;
bool g_silent = false;
int g_hovTile = -1, g_closeHov = 0, g_minHov = 0;
HMENU g_hTrayM;

void InitTheme() {
    g_brBg = CreateSolidBrush(RGB(0x1A,0x1A,0x1A));
    g_brTileBg = CreateSolidBrush(RGB(0x2A,0x2A,0x2A));
    g_brTileBd = CreateSolidBrush(RGB(0x9B,0x59,0xB6));
    g_brBtn = CreateSolidBrush(RGB(0x7B,0x2D,0x8E));
    g_brBtnH = CreateSolidBrush(RGB(0x9B,0x59,0xB6));
    g_brRed = CreateSolidBrush(RGB(0xC0,0x39,0x2B));
    g_brTb = CreateSolidBrush(RGB(0x0D,0x0D,0x0D));
    NONCLIENTMETRICS nm = {sizeof(nm)};
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(nm),&nm,0);
    g_hFont = CreateFontIndirect(&nm.lfMessageFont);
    g_hFontB = CreateFont(-13,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    g_hFontSm = CreateFont(-12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Segoe UI");
}

std::wstring AppDataP() {
    wchar_t p[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, p);
    return std::wstring(p) + L"\\GameVault";
}
std::wstring SaveP() { return AppDataP() + L"\\games.json"; }

std::string W2U(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string r(n-1,0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &r[0], n, NULL, NULL);
    return r;
}
std::wstring U2W(const std::string& u) {
    if (u.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, u.c_str(), -1, NULL, 0);
    std::wstring r(n-1,0);
    MultiByteToWideChar(CP_UTF8, 0, u.c_str(), -1, &r[0], n);
    return r;
}
std::string JEsc(const std::wstring& s) {
    std::string u = W2U(s), o;
    for (char c : u) {
        if (c=='\"') o+="\\\""; else if (c=='\\') o+="\\\\";
        else if (c=='\n') o+="\\n"; else if (c=='\r') o+="\\r";
        else o+=c;
    }
    return o;
}

std::wstring GetGN(const std::wstring& p) {
    DWORD h=0,z=GetFileVersionInfoSize(p.c_str(),&h);
    if(z>0) {
        std::vector<char> b(z);
        if(GetFileVersionInfo(p.c_str(),0,z,&b[0])) {
            wchar_t* d=nullptr; UINT l=0;
            if(VerQueryValue(&b[0],L"\\StringFileInfo\\040904B0\\FileDescription",(void**)&d,&l)&&d&&l>0)
                return std::wstring(d,l-1);
            if(VerQueryValue(&b[0],L"\\StringFileInfo\\040904B0\\ProductName",(void**)&d,&l)&&d&&l>0)
                return std::wstring(d,l-1);
        }
    }
    size_t i=p.rfind(L'\\'); std::wstring n=(i!=std::wstring::npos)?p.substr(i+1):p;
    i=n.rfind(L'.'); return (i!=std::wstring::npos)?n.substr(0,i):n;
}

HICON GetIcon(const std::wstring& p) {
    HICON h=nullptr;
    if(SUCCEEDED(SHDefExtractIcon(p.c_str(),0,0,&h,nullptr,64))&&h) return h;
    SHFILEINFO s={};
    if(SHGetFileInfo(p.c_str(),0,&s,sizeof(s),SHGFI_ICON|SHGFI_LARGEICON)&&s.hIcon) return s.hIcon;
    return nullptr;
}

void Save() {
    CreateDirectory(AppDataP().c_str(),nullptr);
    std::string j="[\n";
    for(size_t i=0;i<g_games.size();i++) { if(i) j+=",\n"; j+="  \""+JEsc(g_games[i].filePath)+"\""; }
    j+="\n]\n"; std::ofstream f(SaveP().c_str()); if(f.is_open()){f<<j;f.close();}
}
void Load() {
    std::ifstream f(SaveP().c_str()); if(!f.is_open()) return;
    std::string c((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>()); f.close();
    size_t p=0;
    while((p=c.find('\"',p))!=std::string::npos) {
        size_t e=c.find('\"',p+1); if(e==std::string::npos) break;
        std::wstring wp=U2W(c.substr(p+1,e-p-1));
        if(GetFileAttributes(wp.c_str())!=INVALID_FILE_ATTRIBUTES) {
            GameEntry ge; ge.filePath=wp; ge.name=GetGN(wp); ge.icon=GetIcon(wp);
            g_games.push_back(ge);
        }
        p=e+1;
    }
}

void RebuildTiles() {
    RECT rc; GetClientRect(g_hwndGame,&rc);
    int visH=rc.bottom-rc.top, cols=std::max<int>(1,rc.right/(TILE_W+TILE_M*2));
    int rows=(int)((g_games.size()+cols-1)/cols);
    g_contentH=20+rows*(TILE_H+TILE_M*2);
    bool needScroll=g_contentH>visH;
    g_contentH=std::max(g_contentH,visH);
    ShowScrollBar(g_hwndGame,SB_VERT,needScroll);
    if(needScroll) {
        SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS};
        si.nMin=0; si.nMax=g_contentH; si.nPage=visH;
        si.nPos=std::min(g_scrollY,g_contentH-(int)si.nPage);
        g_scrollY=si.nPos; SetScrollInfo(g_hwndGame,SB_VERT,&si,TRUE);
    }
    InvalidateRect(g_hwndGame,nullptr,TRUE);
    UpdAddBtn();
}

void UpdAddBtn() {
    bool any=false;
    for(auto& r:g_running){DWORD e;if(GetExitCodeProcess(r.process,&e)&&e==STILL_ACTIVE){any=true;break;}}
    SetWindowText(g_hBtnAdd,any?L"Kill All Running Games":L"+ Add Games");
    InvalidateRect(g_hBtnAdd,nullptr,TRUE);
}

void AddGames() {
    IFileOpenDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) return;
    DWORD flags;
    pfd->GetOptions(&flags);
    pfd->SetOptions(flags | FOS_ALLOWMULTISELECT | FOS_FILEMUSTEXIST);
    pfd->SetTitle(L"Select game executables");
    COMDLG_FILTERSPEC filter = { L"Executable files (*.exe)", L"*.exe" };
    pfd->SetFileTypes(1, &filter);
    hr = pfd->Show(g_hWnd);
    if (FAILED(hr)) { pfd->Release(); return; }
    IShellItemArray* results = nullptr;
    hr = pfd->GetResults(&results);
    if (FAILED(hr)) { pfd->Release(); return; }
    DWORD count = 0;
    results->GetCount(&count);
    bool added = false;
    for (DWORD i = 0; i < count; i++) {
        IShellItem* item = nullptr;
        if (FAILED(results->GetItemAt(i, &item))) continue;
        wchar_t* path = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
            bool dup = false;
            for (auto& g : g_games) {
                if (_wcsicmp(g.filePath.c_str(), path) == 0) { dup = true; break; }
            }
            if (!dup) {
                GameEntry ge;
                ge.filePath = path;
                ge.name = GetGN(path);
                ge.icon = GetIcon(path);
                g_games.push_back(ge);
                added = true;
            }
            CoTaskMemFree(path);
        }
        item->Release();
    }
    results->Release();
    pfd->Release();
    if (added) { Save(); RebuildTiles(); }
}

void LaunchG(int i) {
    if(i<0||i>=(int)g_games.size()) return;
    auto& g=g_games[i];
    for(auto& r:g_running) {
        DWORD e; if(GetExitCodeProcess(r.process,&e)&&e==STILL_ACTIVE) {
            wchar_t rp[MAX_PATH]; DWORD gmf=0;
            gmf=GetModuleFileNameExW(r.process,0,rp,MAX_PATH);
            if(gmf&&_wcsicmp(rp,g.filePath.c_str())==0) {
                MessageBox(g_hWnd,L"This game is already running.",L"Game Running",MB_OK|MB_ICONWARNING);
                return;
            }
        }
    }
    STARTUPINFO si={sizeof(si)}; PROCESS_INFORMATION pi;
    if(!CreateProcess(g.filePath.c_str(),nullptr,nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)) {
        MessageBox(g_hWnd,(std::wstring(L"Could not launch:\n")+g.filePath).c_str(),L"Launch Failed",MB_OK|MB_ICONERROR);
        return;
    }
    CloseHandle(pi.hThread);
    g_running.push_back({pi.hProcess,pi.dwProcessId,g.name});
    TrayMenuBuild(); UpdAddBtn();
    Shell_NotifyIcon(NIM_MODIFY,&g_nid);
    ShowWindow(g_hWnd,SW_HIDE);
}

void KillG(int i) {
    if(i<0||i>=(int)g_running.size()) return;
    DWORD e; if(GetExitCodeProcess(g_running[i].process,&e)&&e==STILL_ACTIVE) TerminateProcess(g_running[i].process,1);
    CloseHandle(g_running[i].process); g_running.erase(g_running.begin()+i);
    TrayMenuBuild(); UpdAddBtn();
}
void KillAll(){while(!g_running.empty())KillG(0);}

std::wstring ShortP(const std::wstring& p) {
    wchar_t s[MAX_PATH];
    DWORD n=GetShortPathName(p.c_str(),s,MAX_PATH);
    return (n>0&&n<MAX_PATH)?std::wstring(s,n):p;
}

void Center(HWND h) {
    RECT r; GetWindowRect(h,&r);
    int w=r.right-r.left, hgt=r.bottom-r.top;
    SetWindowPos(h,nullptr,(GetSystemMetrics(SM_CXSCREEN)-w)/2,(GetSystemMetrics(SM_CYSCREEN)-hgt)/2,w,hgt,SWP_NOZORDER|SWP_NOSIZE);
}

void TraySetup() {
    g_nid.cbSize=sizeof(NOTIFYICONDATA);
    g_nid.hWnd=g_hWnd; g_nid.uID=1;
    g_nid.uFlags=NIF_ICON|NIF_TIP|NIF_MESSAGE;
    g_nid.uCallbackMessage=WM_TRAYICON;
    g_nid.hIcon=g_hAppIcon;
    wcscpy_s(g_nid.szTip,APP_NAME);
    Shell_NotifyIcon(NIM_ADD,&g_nid);
}
void TrayMenuBuild() {
    if(g_hTrayM) DestroyMenu(g_hTrayM);
    g_hTrayM=CreatePopupMenu();
    for(size_t i=g_running.size();i>0;i--) {
        DWORD e; if(GetExitCodeProcess(g_running[i-1].process,&e)&&e==STILL_ACTIVE) {
            AppendMenu(g_hTrayM,MF_STRING,2000+(int)(i-1),(L"Kill "+g_running[i-1].name).c_str());
        }
    }
    AppendMenu(g_hTrayM,MF_STRING,1000,L"Show GameVault");
    AppendMenu(g_hTrayM,MF_STRING,1001,L"Exit");
}
void ShowWin() {
    Shell_NotifyIcon(NIM_MODIFY,&g_nid);
    ShowWindow(g_hWnd,SW_SHOW); ShowWindow(g_hWnd,SW_RESTORE); SetForegroundWindow(g_hWnd);
}
void ListenShow() {
    std::thread([](){
        HANDLE he=CreateEvent(nullptr,FALSE,FALSE,L"GameVault-ShowSignal");
        if(!he) return;
        while(WaitForSingleObject(he,INFINITE)==WAIT_OBJECT_0) PostMessage(g_hWnd,WM_SHOWSIGNAL,0,0);
    }).detach();
}

void RegStartup() {
    HKEY k;
    if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&k)==ERROR_SUCCESS) {
        wchar_t exe[MAX_PATH]; GetModuleFileName(nullptr,exe,MAX_PATH);
        std::wstring v=std::wstring(L"\"")+exe+L"\" --silent";
        RegSetValueEx(k,L"GameVault",0,REG_SZ,(BYTE*)v.c_str(),(DWORD)((v.length()+1)*2));
        RegCloseKey(k);
    }
}
void RegUninst() {
    HKEY k;
    if(RegCreateKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\GameVault",0,nullptr,0,KEY_SET_VALUE,nullptr,&k,nullptr)==ERROR_SUCCESS) {
        wchar_t exe[MAX_PATH]; GetModuleFileName(nullptr,exe,MAX_PATH);
        DWORD o=1;
        RegSetValueEx(k,L"DisplayName",0,REG_SZ,(BYTE*)APP_NAME,(DWORD)((wcslen(APP_NAME)+1)*2));
        std::wstring us=std::wstring(L"\"")+exe+L"\" --uninstall";
        RegSetValueEx(k,L"UninstallString",0,REG_SZ,(BYTE*)us.c_str(),(DWORD)((us.length()+1)*2));
        RegSetValueEx(k,L"DisplayIcon",0,REG_SZ,(BYTE*)exe,(DWORD)((wcslen(exe)+1)*2));
        RegSetValueEx(k,L"DisplayVersion",0,REG_SZ,(BYTE*)APP_VER,(DWORD)((wcslen(APP_VER)+1)*2));
        RegSetValueEx(k,L"Publisher",0,REG_SZ,(BYTE*)PUBLISHER,(DWORD)((wcslen(PUBLISHER)+1)*2));
        wchar_t id[MAX_PATH]; wcscpy_s(id,exe); wchar_t* sep=wcsrchr(id,L'\\'); if(sep)*sep=0;
        RegSetValueEx(k,L"InstallLocation",0,REG_SZ,(BYTE*)id,(DWORD)((wcslen(id)+1)*2));
        RegSetValueEx(k,L"NoModify",0,REG_DWORD,(BYTE*)&o,sizeof(o));
        RegSetValueEx(k,L"NoRepair",0,REG_DWORD,(BYTE*)&o,sizeof(o));
        RegCloseKey(k);
    }
}
void RunUninst() {
    if(MessageBox(nullptr,
        L"Are you sure you want to uninstall GameVault?\n\nThis will:\n\u2022 Remove your saved game library\n\u2022 Remove the autostart entry\n\u2022 Delete GameVault.exe",
        L"Uninstall GameVault",MB_YESNO|MB_ICONQUESTION)!=IDYES) ExitProcess(0);
    wchar_t exe[MAX_PATH]; GetModuleFileName(nullptr,exe,MAX_PATH);
    std::wstring sad=AppDataP();
    wchar_t t[MAX_PATH]; std::wstring stmp; if(GetTempPath(MAX_PATH,t)) stmp=t;
    std::wstring bp=stmp+L"GameVault_cleanup.bat";
    HKEY rk;
    if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&rk)==ERROR_SUCCESS) {
        RegDeleteValue(rk,L"GameVault"); RegCloseKey(rk);
    }
    if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",0,KEY_WRITE,&rk)==ERROR_SUCCESS) {
        SHDeleteKey(rk,L"GameVault"); RegCloseKey(rk);
    }
    wchar_t fn[MAX_PATH]; wcscpy_s(fn,exe); wchar_t* sep=wcsrchr(fn,L'\\');
    std::wstring fname=sep?sep+1:fn;
    std::wstring bat=L"@echo off\r\n:loop\r\ntaskkill /f /im \""+fname+L"\" >nul 2>&1\r\n";
    bat+=L"ping -n 3 127.0.0.1 >nul\r\ndel /f /q \""+std::wstring(exe)+L"\" >nul 2>&1\r\nif exist \""+std::wstring(exe)+L"\" goto loop\r\n";
    bat+=L"rd /s /q \""+sad+L"\" >nul 2>&1\r\ndel /f /q \""+bp+L"\"";
    HANDLE hf=CreateFileW(bp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(hf!=INVALID_HANDLE_VALUE) {
        std::string u=W2U(bat); DWORD written;
        WriteFile(hf,u.data(),(DWORD)u.size(),&written,nullptr);
        CloseHandle(hf);
    }
    ShellExecuteW(nullptr,L"open",L"cmd.exe",(L"/c \""+bp+L"\"").c_str(),nullptr,SW_HIDE);
    ExitProcess(0);
}

int TileAt(int x, int y, int& cols, int& rows) {
    RECT rc; GetClientRect(g_hwndGame,&rc);
    int w=rc.right-rc.left;
    cols=std::max(1,w/(TILE_W+TILE_M*2));
    rows=(int)((g_games.size()+cols-1)/cols);
    if(x<20||y<20) return -1;
    int c=(x-20)/(TILE_W+TILE_M*2);
    int r=(y-20+g_scrollY)/(TILE_H+TILE_M*2);
    if(c<cols&&r<rows) {
        int idx=r*cols+c;
        if(idx<(int)g_games.size()) return idx;
    }
    return -1;
}

void PaintTiles(HWND hwnd, HDC hdc, int w, int h) {
    int cols=0, rows=0;
    RECT rc; GetClientRect(hwnd,&rc);
    w=rc.right; h=rc.bottom;
    cols=std::max(1,w/(TILE_W+TILE_M*2));
    rows=(int)((g_games.size()+cols-1)/cols);
    int total=20+rows*(TILE_H+TILE_M*2);
    if(total<rc.bottom) total=rc.bottom;
    HBRUSH brB = CreateSolidBrush(RGB(0x1A,0x1A,0x1A));
    RECT bg={0,0,w,total}; FillRect(hdc,&bg,brB); DeleteObject(brB);
    HBRUSH brT=CreateSolidBrush(RGB(0x2A,0x2A,0x2A));
    HBRUSH brH=CreateSolidBrush(RGB(0x3A,0x3A,0x3A));
    HPEN hp=CreatePen(PS_SOLID,2,RGB(0x9B,0x59,0xB6));
    SetBkMode(hdc,TRANSPARENT);
    for(size_t i=0;i<g_games.size();i++) {
        int r=(int)i/cols, c=(int)i%cols;
        int x=20+c*(TILE_W+TILE_M*2), y=20+r*(TILE_H+TILE_M*2)-g_scrollY;
        if(y+TILE_H<0||y>h) continue;
        SelectObject(hdc,(int)i==g_hovTile?brH:brT);
        SelectObject(hdc,hp);
        RoundRect(hdc,x,y,x+TILE_W,y+TILE_H,8,8);
        if(g_games[i].icon) {
            DrawIconEx(hdc,x+(TILE_W-64)/2,y+6,g_games[i].icon,64,64,0,nullptr,DI_NORMAL);
        }
        SetTextColor(hdc,RGB(0xE0,0xE0,0xE0));
        SelectObject(hdc,g_hFontSm);
        RECT tr2={x+4,y+72,x+TILE_W-4,y+TILE_H-6};
        DrawText(hdc,g_games[i].name.c_str(),-1,&tr2,DT_CENTER|DT_WORDBREAK|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
    DeleteObject(hp); DeleteObject(brT); DeleteObject(brH);
}

LRESULT CALLBACK GameAreaProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    switch(msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc=BeginPaint(hwnd,&ps);
        RECT rc; GetClientRect(hwnd,&rc);
        HDC mdc=CreateCompatibleDC(hdc);
        HBITMAP bm=CreateCompatibleBitmap(hdc,rc.right-rc.left,rc.bottom-rc.top);
        HBITMAP ob=(HBITMAP)SelectObject(mdc,bm);
        PaintTiles(hwnd,mdc,rc.right-rc.left,rc.bottom-rc.top);
        BitBlt(hdc,0,0,rc.right-rc.left,rc.bottom-rc.top,mdc,0,0,SRCCOPY);
        SelectObject(mdc,ob); DeleteObject(bm); DeleteDC(mdc);
        EndPaint(hwnd,&ps); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDOWN: {
        int x=LOWORD(lParam), y=HIWORD(lParam), cols=0, rows=0;
        int idx=TileAt(x,y,cols,rows);
        if(idx>=0) LaunchG(idx);
        return 0;
    }
    case WM_RBUTTONDOWN: {
        int x=LOWORD(lParam), y=HIWORD(lParam), cols=0, rows=0;
        int idx=TileAt(x,y,cols,rows);
        if(idx>=0) {
            HMENU hm=CreatePopupMenu();
            AppendMenu(hm,MF_STRING,100,L"Remove");
            POINT pt={x,y}; ClientToScreen(hwnd,&pt);
            int cmd=TrackPopupMenu(hm,TPM_RETURNCMD|TPM_NONOTIFY,pt.x,pt.y,0,hwnd,nullptr);
            DestroyMenu(hm);
            if(cmd==100) {
                g_games.erase(g_games.begin()+idx);
                Save(); RebuildTiles();
            }
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        if(!g_games.size()) return 0;
        int x=LOWORD(lParam), y=HIWORD(lParam), cols=0, rows=0;
        int idx=TileAt(x,y,cols,rows);
        if(idx!=g_hovTile){g_hovTile=idx;InvalidateRect(hwnd,nullptr,TRUE);}
        SetCursor(idx>=0?LoadCursor(nullptr,IDC_HAND):LoadCursor(nullptr,IDC_ARROW));
        return 0;
    }
    case WM_MOUSELEAVE: { g_hovTile=-1; InvalidateRect(hwnd,nullptr,TRUE); return 0; }
    case WM_VSCROLL: {
        SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS};
        GetScrollInfo(hwnd,SB_VERT,&si);
        int y=si.nPos;
        switch(LOWORD(wParam)) {
        case SB_LINEUP: y-=20; break; case SB_LINEDOWN: y+=20; break;
        case SB_PAGEUP: y-=si.nPage; break; case SB_PAGEDOWN: y+=si.nPage; break;
        case SB_THUMBTRACK: y=HIWORD(wParam); break;
        }
        if(y<0) y=0; if(y>(int)si.nMax-(int)si.nPage) y=std::max(0,si.nMax-(int)si.nPage);
        if(y!=g_scrollY){g_scrollY=y;si.fMask=SIF_POS;si.nPos=y;SetScrollInfo(hwnd,SB_VERT,&si,TRUE);InvalidateRect(hwnd,nullptr,TRUE);}
        return 0;
    }
    case WM_MOUSEWHEEL: {
        RECT rc; GetClientRect(hwnd,&rc);
        if(g_contentH<=rc.bottom-rc.top) return 0;
        int z=GET_WHEEL_DELTA_WPARAM(wParam);
        SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS};
        GetScrollInfo(hwnd,SB_VERT,&si);
        int y=si.nPos-(z/120)*40;
        if(y<0) y=0; if(y>(int)si.nMax-(int)si.nPage) y=std::max(0,si.nMax-(int)si.nPage);
        if(y!=g_scrollY){g_scrollY=y;si.fMask=SIF_POS;si.nPos=y;SetScrollInfo(hwnd,SB_VERT,&si,TRUE);InvalidateRect(hwnd,nullptr,TRUE);}
        return 0;
    }
    case WM_SIZE: { RebuildTiles(); return 0; }
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    switch(msg) {
    case WM_CREATE: {
        g_hWnd=hWnd; InitTheme();
        RECT rc; GetClientRect(hWnd,&rc);
        g_hwndGame=CreateWindowEx(0,L"GameVaultGameArea",nullptr,
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_CLIPCHILDREN,
            0,TB_H,rc.right,rc.bottom-TB_H-BB_H,hWnd,nullptr,g_hInst,nullptr);
        SetWindowTheme(g_hwndGame,L"DarkMode_Explorer",nullptr);
        g_hBtnAdd=CreateWindow(L"BUTTON",L"+ Add Games",
            WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
            (900-200)/2,rc.bottom-BB_H+12,200,36,hWnd,(HMENU)1,g_hInst,nullptr);
        TraySetup(); Load(); RebuildTiles(); ListenShow(); RegUninst();
        TRACKMOUSEEVENT tme={sizeof(tme),TME_LEAVE,hWnd,0};
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_SHOWSIGNAL: ShowWin(); return 0;
    case WM_TRAYICON:
        if(lParam==WM_LBUTTONDBLCLK) ShowWin();
        else if(lParam==WM_RBUTTONUP) {
            TrayMenuBuild(); POINT pt; GetCursorPos(&pt); SetForegroundWindow(hWnd);
            int c=TrackPopupMenu(g_hTrayM,TPM_RETURNCMD|TPM_NONOTIFY,pt.x,pt.y,0,hWnd,nullptr);
            if(c==1000) ShowWin(); else if(c==1001) PostMessage(hWnd,WM_DESTROY,0,0);
            else if(c>=2000) KillG(c-2000);
            PostMessage(hWnd,WM_NULL,0,0);
        } return 0;
    case WM_NCACTIVATE: return TRUE;
    case WM_NCCALCSIZE: return 0;
    case WM_NCHITTEST: {
        LRESULT r=DefWindowProc(hWnd,msg,wParam,lParam);
        if(r==HTCLIENT) {
            POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
            ScreenToClient(hWnd,&pt);
            RECT cr; GetClientRect(hWnd,&cr);
            int w=cr.right, h=cr.bottom;
            if(pt.x<RESIZE_BORDER&&pt.y<RESIZE_BORDER) return HTTOPLEFT;
            if(pt.x>=w-RESIZE_BORDER&&pt.y<RESIZE_BORDER) return HTTOPRIGHT;
            if(pt.x<RESIZE_BORDER&&pt.y>=h-RESIZE_BORDER) return HTBOTTOMLEFT;
            if(pt.x>=w-RESIZE_BORDER&&pt.y>=h-RESIZE_BORDER) return HTBOTTOMRIGHT;
            if(pt.x<RESIZE_BORDER) return HTLEFT;
            if(pt.x>=w-RESIZE_BORDER) return HTRIGHT;
            if(pt.y<RESIZE_BORDER) return HTTOP;
            if(pt.y>=h-RESIZE_BORDER) return HTBOTTOM;
            return HTCLIENT;
        }
        return r;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc=BeginPaint(hWnd,&ps);
        RECT rc; GetClientRect(hWnd,&rc);
        // title bar
        RECT tb={0,0,rc.right,TB_H};
        FillRect(hdc,&tb,g_brTb);
        // title text
        SetTextColor(hdc,RGB(0x9B,0x59,0xB6));
        SetBkMode(hdc,TRANSPARENT);
        SelectObject(hdc,g_hFontB);
        RECT tt={12,0,200,TB_H}; DrawText(hdc,APP_NAME,-1,&tt,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        // minimize button
        RECT mb={rc.right-92,0,rc.right-46,TB_H};
        SetDCBrushColor(hdc,g_minHov?RGB(0x55,0x55,0x55):RGB(0x0D,0x0D,0x0D));
        FillRect(hdc,&mb,(HBRUSH)GetStockObject(DC_BRUSH));
        SetTextColor(hdc,RGB(0x88,0x88,0x88));
        SelectObject(hdc,g_hFont);
        DrawText(hdc,L"\u2014",-1,&mb,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        // close button
        RECT cb={rc.right-46,0,rc.right,TB_H};
        SetDCBrushColor(hdc,g_closeHov?RGB(0xC0,0x39,0x2B):RGB(0x0D,0x0D,0x0D));
        FillRect(hdc,&cb,(HBRUSH)GetStockObject(DC_BRUSH));
        SetTextColor(hdc,g_closeHov?RGB(0xFF,0xFF,0xFF):RGB(0x88,0x88,0x88));
        DrawText(hdc,L"\u2715",-1,&cb,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        // bottom bar
        HPEN bp=CreatePen(PS_SOLID,2,RGB(0x7B,0x2D,0x8E));
        SelectObject(hdc,bp);
        HBRUSH bbr=CreateSolidBrush(RGB(0x12,0x12,0x12));
        RECT bb={0,rc.bottom-BB_H,rc.right,rc.bottom};
        FillRect(hdc,&bb,bbr);
        MoveToEx(hdc,0,bb.top,nullptr); LineTo(hdc,rc.right,bb.top);
        DeleteObject(bbr); DeleteObject(bp);
        EndPaint(hWnd,&ps); return 0;
    }
    case WM_LBUTTONDOWN: {
        int x=LOWORD(lParam), y=HIWORD(lParam);
        if(y>=TB_H) break;
        RECT rc; GetClientRect(hWnd,&rc);
        int cw=rc.right;
        if(x>=cw-46&&x<cw) { PostMessage(hWnd,WM_DESTROY,0,0); return 0; }
        if(x>=cw-92&&x<cw-46) { ShowWindow(hWnd,SW_HIDE); return 0; }
        POINT pt={x,y}; ClientToScreen(hWnd,&pt);
        SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
        return 0;
    }
    case WM_MOUSEMOVE: {
        int x=LOWORD(lParam), y=HIWORD(lParam);
        if(y>=TB_H) return 0;
        RECT rc; GetClientRect(hWnd,&rc);
        int cw=rc.right, oldClose=g_closeHov, oldMin=g_minHov;
        g_closeHov=(x>=cw-46&&x<cw)?1:0;
        g_minHov=(x>=cw-92&&x<cw-46)?1:0;
        if(oldClose!=g_closeHov||oldMin!=g_minHov) InvalidateRect(hWnd,nullptr,TRUE);
        TRACKMOUSEEVENT tme={sizeof(tme),TME_LEAVE,hWnd,0};
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE: { g_closeHov=0; g_minHov=0; InvalidateRect(hWnd,nullptr,TRUE); return 0; }
    case WM_SIZE: {
        int w=LOWORD(lParam),h=HIWORD(lParam);
        if(g_hwndGame) SetWindowPos(g_hwndGame,nullptr,0,TB_H,w,h-TB_H-BB_H,SWP_NOZORDER);
        if(g_hBtnAdd) SetWindowPos(g_hBtnAdd,nullptr,(w-200)/2,h-BB_H+12,200,36,SWP_NOZORDER);
        return 0;
    }
    case WM_COMMAND: {
        int id=LOWORD(wParam), hi=HIWORD(wParam);
        if(hi==BN_CLICKED||hi==0) {
            if(id==1) {
                bool any=false; for(auto& r:g_running){DWORD e;if(GetExitCodeProcess(r.process,&e)&&e==STILL_ACTIVE){any=true;break;}}
                if(any) KillAll(); else AddGames();
            } else if(id==2||id==3) {
                if(id==3) PostMessage(hWnd,WM_DESTROY,0,0);
                else ShowWindow(hWnd,SW_HIDE);
            }
        }
        return 0;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* di=(DRAWITEMSTRUCT*)lParam;
        if(di->CtlType==ODT_BUTTON&&di->CtlID==1) {
            bool any=false; for(auto& r:g_running){DWORD e;if(GetExitCodeProcess(r.process,&e)&&e==STILL_ACTIVE){any=true;break;}}
            COLORREF bg=any?RGB(0xC0,0x39,0x2B):RGB(0x7B,0x2D,0x8E);
            if(di->itemState&ODS_SELECTED) bg=RGB(0x5A,0x1E,0x6A);
            SetDCBrushColor(di->hDC,bg);
            FillRect(di->hDC,&di->rcItem,(HBRUSH)GetStockObject(DC_BRUSH));
            SetBkMode(di->hDC,TRANSPARENT);
            SetTextColor(di->hDC,RGB(0xFF,0xFF,0xFF));
            SelectObject(di->hDC,g_hFontB);
            DrawText(di->hDC,any?L"Kill All Running Games":L"+ Add Games",-1,&di->rcItem,
                DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
            return TRUE;
        }
        return FALSE;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc=(HDC)wParam;
        SetTextColor(hdc,RGB(0xE0,0xE0,0xE0));
        SetBkMode(hdc,TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    case WM_GETMINMAXINFO:{MINMAXINFO*m=(MINMAXINFO*)lParam;m->ptMinTrackSize.x=400;m->ptMinTrackSize.y=300;return 0;}
    case WM_DESTROY: KillAll(); Shell_NotifyIcon(NIM_DELETE,&g_nid); PostQuitMessage(0); return 0;
    case WM_CLOSE: ShowWindow(hWnd,SW_HIDE); return 0;
    }
    return DefWindowProc(hWnd,msg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE,LPSTR lpCmdLine,int nShow) {
    g_hInst=hInst;
    HMODULE hu=GetModuleHandleW(L"uxtheme.dll");
    if(!hu) hu=LoadLibraryW(L"uxtheme.dll");
    if(hu) { FARPROC f=GetProcAddress(hu,MAKEINTRESOURCEA(135)); if(f) ((void(WINAPI*)(int))f)(2); }
    std::wstring cl=GetCommandLine();
    g_silent=cl.find(L"--silent")!=std::wstring::npos;
    if(cl.find(L"--uninstall")!=std::wstring::npos){RunUninst();return 0;}
    g_hMutex=CreateMutex(nullptr,TRUE,L"GameVault-SingleInstance");
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
        if(!g_silent){HANDLE he=OpenEvent(EVENT_MODIFY_STATE,FALSE,L"GameVault-ShowSignal");if(he){SetEvent(he);CloseHandle(he);}}
        CloseHandle(g_hMutex); return 0;
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    InitCommonControls();
    WNDCLASSEX wc={sizeof(WNDCLASSEX)}; wc.style=CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=WndProc; wc.hInstance=hInst; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=CreateSolidBrush(RGB(0x1A,0x1A,0x1A));
    wc.lpszClassName=APP_NAME;
    wc.hIcon=LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAIN));
    RegisterClassEx(&wc);
    WNDCLASSEX wcg={sizeof(wcg)}; wcg.style=CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
    wcg.lpfnWndProc=GameAreaProc; wcg.hInstance=hInst; wcg.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wcg.hbrBackground=CreateSolidBrush(RGB(0x1A,0x1A,0x1A));
    wcg.lpszClassName=L"GameVaultGameArea";
    RegisterClassEx(&wcg);
    g_hAppIcon=(HICON)LoadImage(hInst,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
    if(!g_hAppIcon) g_hAppIcon=LoadIcon(nullptr,IDI_APPLICATION);
    g_hWnd=CreateWindowEx(0,APP_NAME,APP_NAME,WS_POPUP|WS_SIZEBOX|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
        100,100,900,600,nullptr,nullptr,hInst,nullptr);
    if(!g_hWnd) return 1;
    RegStartup();
    if(!g_silent){ShowWindow(g_hWnd,SW_SHOW);Center(g_hWnd);}
    MSG m; while(GetMessage(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessage(&m);}
    CoUninitialize();
    ReleaseMutex(g_hMutex); CloseHandle(g_hMutex); return (int)m.wParam;
}

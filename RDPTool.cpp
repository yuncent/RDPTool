/**
 * 项目名称：RDP 端口一键修改工具 (RDP Port Modifier)
 * 界面规格：320px * 225px | 左右边距: 25px | 控件间距: 25px
 * 兼容性：全平台支持 (Windows XP / 7 / 8 / 10 / 11)
 * 编码规范：使用 Unicode 字符集，遵循 Win32 API 标准
 */

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <shlobj.h>
#include <shellapi.h>

// 静态链接系统库
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")

// 控件 ID 定义
#define ID_INPUT 101
#define ID_BTN   102
#define ID_RAND  103
#define ID_OPEN  104 
#define ID_LINK  555

// 全局句柄与资源
HWND hInput, hText, hStatus;
HFONT hFontUnderline = NULL;

/**
 * 业务逻辑：从系统注册表检索当前 RDP 服务端口
 */
DWORD GetPort() {
    HKEY hKey;
    DWORD port = 3389, size = sizeof(port);
    const wchar_t* subkey = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"PortNumber", NULL, NULL, (LPBYTE)&port, &size);
        RegCloseKey(hKey);
    }
    return port;
}

/**
 * 业务逻辑：持久化写入 RDP 端口至注册表
 */
BOOL SetPort(DWORD port) {
    HKEY hKey;
    const wchar_t* subkey = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"PortNumber", 0, REG_DWORD, (LPBYTE)&port, sizeof(port));
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

/**
 * 子窗口过程：处理“关于”对话框及超链接交互
 */
LRESULT CALLBACK AboutWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hLink;
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"作者：yuncent", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 30, 230, 20, hwnd, NULL, NULL, NULL);
        hLink = CreateWindowW(L"STATIC", L"访问 GitHub 项目地址", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY, 10, 65, 230, 20, hwnd, (HMENU)ID_LINK, NULL, NULL);
        
        LOGFONTW lf = {0};
        GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
        lf.lfUnderline = TRUE;
        hFontUnderline = CreateFontIndirectW(&lf);
        SendMessage(hLink, WM_SETFONT, (WPARAM)hFontUnderline, TRUE);
        break;
    }
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == hLink) {
            SetTextColor((HDC)wParam, RGB(0, 0, 255));
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetStockObject(HOLLOW_BRUSH);
        }
        break;
    case WM_SETCURSOR:
        if ((HWND)wParam == hLink) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_LINK) 
            ShellExecuteW(NULL, L"open", L"https://github.com/yuncent/RDPTool", NULL, NULL, SW_SHOWNORMAL);
        break;
    case WM_CLOSE:
        if (hFontUnderline) DeleteObject(hFontUnderline);
        DestroyWindow(hwnd);
        break;
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/**
 * 主窗口过程：UI 布局管理与消息分发
 */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 界面设计常量 (基于 320px 宽度)
    const int MARGIN = 25;      // 左右全局边距
    const int CTRL_SP = 25;    // 核心控件间距 (你要求的 25px)
    const int ROW_H  = 25;      // 标准行高

    switch (msg) {
    case WM_CREATE: {
        srand((unsigned int)time(NULL));
        wchar_t pBuf[16]; wsprintfW(pBuf, L"%d", GetPort());

        // 第一行：只读状态展示
        CreateWindowW(L"STATIC", L"当前端口：", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, MARGIN, 20, 75, ROW_H, hwnd, NULL, NULL, NULL);
        hText = CreateWindowW(L"STATIC", pBuf, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, MARGIN + 85, 20, 100, ROW_H, hwnd, NULL, NULL, NULL);

        // 第二行：输入与随机化控件
        // 布局公式：边距(25) + 标签(75) + 间隙(10) + 数字框(80) + 间隙(25) + 按钮(80) = 295px (右留空 25px)
        CreateWindowW(L"STATIC", L"修改端口：", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, MARGIN, 60, 75, ROW_H, hwnd, NULL, NULL, NULL);
        hInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"3389", WS_VISIBLE | WS_CHILD | ES_NUMBER, MARGIN + 85, 60, 80, ROW_H, hwnd, (HMENU)ID_INPUT, NULL, NULL);
        CreateWindowW(L"BUTTON", L"随机端口", WS_VISIBLE | WS_CHILD, MARGIN + 85 + 80 + CTRL_SP, 60, 80, ROW_H, hwnd, (HMENU)ID_RAND, NULL, NULL);

        // 第三行：操作触发按钮区
        // 布局公式：边距(25) + 按钮(122) + 间隙(26) + 按钮(122) = 295px
        CreateWindowW(L"BUTTON", L"确认修改", WS_VISIBLE | WS_CHILD, MARGIN, 105, 122, 35, hwnd, (HMENU)ID_BTN, NULL, NULL);
        CreateWindowW(L"BUTTON", L"远程设置", WS_VISIBLE | WS_CHILD, MARGIN + 122 + 26, 105, 122, 35, hwnd, (HMENU)ID_OPEN, NULL, NULL);

        // 第四行：交互反馈状态栏
        hStatus = CreateWindowW(L"STATIC", L"状态：就绪", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE, MARGIN, 155, 270, 20, hwnd, NULL, NULL, NULL);
        break;
    }
    case WM_SYSCOMMAND: // 拦截上下文帮助消息（标题栏问号点击）
        if ((wParam & 0xFFF0) == SC_CONTEXTHELP) {
            WNDCLASSW ac = {0}; 
            ac.lpfnWndProc = AboutWndProc; 
            ac.hInstance = GetModuleHandle(NULL);
            ac.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); 
            ac.lpszClassName = L"AboutWinClass";
            RegisterClassW(&ac);
            HWND hAbout = CreateWindowExW(WS_EX_TOPMOST, L"AboutWinClass", L"关于软件", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 
                (GetSystemMetrics(0)-265)/2, (GetSystemMetrics(1)-150)/2, 265, 150, hwnd, NULL, NULL, NULL);
            ShowWindow(hAbout, SW_SHOW);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_RAND) {
            wchar_t b[16]; wsprintfW(b, L"%d", 10000 + rand() % 50000); 
            SetWindowTextW(hInput, b);
            SetWindowTextW(hStatus, L"状态：已生成随机高位端口");
        } else if (LOWORD(wParam) == ID_BTN) {
            wchar_t b[16]; GetWindowTextW(hInput, b, 16);
            DWORD port = _wtoi(b);
            if (port > 0 && port <= 65535) {
                if (SetPort(port)) {
                    MessageBoxW(hwnd, L"修改成功！请重启电脑以应用新配置。", L"提示", MB_ICONINFORMATION);
                    SetWindowTextW(hText, b);
                    SetWindowTextW(hStatus, L"状态：端口修改成功，待重启");
                }
            }
        } else if (LOWORD(wParam) == ID_OPEN) {
            SetWindowTextW(hStatus, L"状态：正在启动系统远程设置...");
            ShellExecuteW(NULL, L"open", L"control.exe", L"sysdm.cpl,,5", NULL, SW_SHOWNORMAL);
        }
        break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/**
 * 应用程序入口：处理提权逻辑与初始化主窗口
 */
int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nS) {
    // 强制请求管理员权限（UAC 提升）
    if (!IsUserAnAdmin()) {
        wchar_t p[MAX_PATH]; GetModuleFileNameW(NULL, p, MAX_PATH);
        SHELLEXECUTEINFOW sei = {sizeof(sei), 0, NULL, L"runas", p, NULL, NULL, 5};
        if (ShellExecuteExW(&sei)) return 0;
    }
    
    WNDCLASSW wc = {0}; 
    wc.lpfnWndProc = WndProc; 
    wc.hInstance = hI; 
    wc.lpszClassName = L"RDPMainClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);

    // 创建主窗口：320x225 居中
    HWND hwnd = CreateWindowExW(WS_EX_CONTEXTHELP, L"RDPMainClass", L"RDP端口修改工具", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 
        (GetSystemMetrics(0)-320)/2, (GetSystemMetrics(1)-225)/2, 320, 225, NULL, NULL, hI, NULL);

    ShowWindow(hwnd, nS);
    MSG m; 
    while (GetMessageW(&m, NULL, 0, 0)) { 
        TranslateMessage(&m); 
        DispatchMessageW(&m); 
    }
    return 0;
}
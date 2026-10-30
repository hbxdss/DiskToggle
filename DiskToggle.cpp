// DiskToggle.cpp - 图形化磁盘禁用/启用工具
// 编译: cl /EHsc /O2 /DUNICODE /D_UNICODE DiskToggle.cpp user32.lib gdi32.lib setupapi.lib newdev.lib advapi32.lib shell32.lib /link /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'" /SUBSYSTEM:WINDOWS

#include <windows.h>
#include <setupapi.h>
#include <newdev.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "newdev.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

#define IDC_LIST      1001
#define IDC_BTN_DIS   1002
#define IDC_BTN_EN    1003
#define IDC_BTN_REF   1004
#define IDC_STATUS    1005
#define WM_REFRESH    (WM_USER + 100)

// 磁盘驱动器设备类 GUID
static const GUID GUID_DEVCLASS_DISKDRIVE =
    { 0x4d36e967, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

#ifndef CONFIGFLAG_DISABLED
#define CONFIGFLAG_DISABLED 0x00000040
#endif

static HWND hList = NULL;
static HWND hBtnDis = NULL, hBtnEn = NULL, hBtnRef = NULL;
static HWND hStatus = NULL;
static HINSTANCE hInst;

// 从设备实例句柄获取友好名称
static BOOL GetDeviceString(HDEVINFO hDevInfo, SP_DEVINFO_DATA* did, DWORD prop, LPTSTR buf, DWORD bufLen) {
    if (SetupDiGetDeviceRegistryProperty(hDevInfo, did, prop, NULL, (PBYTE)buf, bufLen, NULL)) {
        return TRUE;
    }
    buf[0] = 0;
    return FALSE;
}

// 判断设备是否被禁用
static BOOL IsDeviceDisabled(HDEVINFO hDevInfo, SP_DEVINFO_DATA* did) {
    DWORD status = 0, prob = 0;
    CONFIGRET cr = CM_Get_DevNode_Status(&status, &prob, did->DevInst, 0);
    if (cr == CR_SUCCESS) {
        if (status & DN_STARTED) return FALSE; // 正在工作
        // 没启动，但可能是因为没驱动。用配置标志再判断
    }
    // 用配置标志判断
    DWORD cfgFlags = 0;
    if (SetupDiGetDeviceRegistryProperty(hDevInfo, did, SPDRP_CONFIGFLAGS, NULL,
            (PBYTE)&cfgFlags, sizeof(cfgFlags), NULL)) {
        return (cfgFlags & CONFIGFLAG_DISABLED) != 0;
    }
    return FALSE;
}

// 禁用或启用设备
static BOOL SetDeviceState(HDEVINFO hDevInfo, SP_DEVINFO_DATA* did, BOOL bDisable) {
    SP_PROPCHANGE_PARAMS pcp;
    ZeroMemory(&pcp, sizeof(pcp));
    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.StateChange = bDisable ? DICS_DISABLE : DICS_ENABLE;
    pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
    pcp.HwProfile = 0;

    if (!SetupDiSetClassInstallParams(hDevInfo, did, &pcp.ClassInstallHeader, sizeof(pcp))) {
        return FALSE;
    }
    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, did)) {
        return FALSE;
    }
    return TRUE;
}

// 刷新列表
static void RefreshList() {
    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)L"无法枚举磁盘设备");
        return;
    }

    SP_DEVINFO_DATA did;
    ZeroMemory(&did, sizeof(did));
    did.cbSize = sizeof(did);

    TCHAR name[512] = {0};
    int idx = 0;

    while (SetupDiEnumDeviceInfo(hDevInfo, idx, &did)) {
        ZeroMemory(name, sizeof(name));

        // 优先友好名称，否则用设备描述
        if (!GetDeviceString(hDevInfo, &did, SPDRP_FRIENDLYNAME, name, _countof(name))) {
            GetDeviceString(hDevInfo, &did, SPDRP_DEVICEDESC, name, _countof(name));
        }

        BOOL disabled = IsDeviceDisabled(hDevInfo, &did);

        TCHAR line[1024];
        _stprintf_s(line, _countof(line), L"[%s]  %s",
            disabled ? L"已禁用" : L"使用中", name[0] ? name : L"(未知设备)");
        LRESULT lidx = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)line);
        // 存设备实例索引
        SendMessage(hList, LB_SETITEMDATA, lidx, (LPARAM)idx);

        ZeroMemory(&did, sizeof(did));
        did.cbSize = sizeof(did);
        idx++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    TCHAR buf[128];
    _stprintf_s(buf, _countof(buf), L"共 %d 块磁盘", idx);
    SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)buf);
}

// 获取当前选中设备的 devinst 索引
static BOOL GetSelectedDevice(HDEVINFO* hDevInfo, SP_DEVINFO_DATA* did) {
    int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) return FALSE;
    int idx = (int)SendMessage(hList, LB_GETITEMDATA, sel, 0);

    *hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
    if (*hDevInfo == INVALID_HANDLE_VALUE) return FALSE;

    ZeroMemory(did, sizeof(*did));
    did->cbSize = sizeof(*did);
    if (!SetupDiEnumDeviceInfo(*hDevInfo, idx, did)) {
        SetupDiDestroyDeviceInfoList(*hDevInfo);
        return FALSE;
    }
    return TRUE;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hList = CreateWindow(L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            10, 10, 560, 280, hWnd, (HMENU)IDC_LIST, hInst, NULL);

        hBtnDis = CreateWindow(L"BUTTON", L"禁用选中磁盘",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 300, 130, 32, hWnd, (HMENU)IDC_BTN_DIS, hInst, NULL);
        hBtnEn = CreateWindow(L"BUTTON", L"启用选中磁盘",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            150, 300, 130, 32, hWnd, (HMENU)IDC_BTN_EN, hInst, NULL);
        hBtnRef = CreateWindow(L"BUTTON", L"刷新列表",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            290, 300, 100, 32, hWnd, (HMENU)IDC_BTN_REF, hInst, NULL);

        hStatus = CreateWindow(L"STATIC", L"就绪",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 345, 560, 20, hWnd, (HMENU)IDC_STATUS, hInst, NULL);

        // 更大字号
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
        SendMessage(hList, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBtnDis, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBtnEn, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBtnRef, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

        PostMessage(hWnd, WM_REFRESH, 0, 0);
        break;
    }

    case WM_REFRESH:
        RefreshList();
        break;

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id == IDC_BTN_REF) {
            RefreshList();
        } else if (id == IDC_BTN_DIS || id == IDC_BTN_EN) {
            HDEVINFO hDevInfo;
            SP_DEVINFO_DATA did;
            if (!GetSelectedDevice(&hDevInfo, &did)) {
                MessageBox(hWnd, L"请先在列表中选择一块磁盘", L"提示", MB_OK | MB_ICONINFORMATION);
                break;
            }
            BOOL bDisable = (id == IDC_BTN_DIS);
            TCHAR msg[512];
            _stprintf_s(msg, _countof(msg),
                L"确定要%s这块磁盘吗？\n\n警告：不要禁用当前正在运行系统的磁盘，否则会立即崩溃！",
                bDisable ? L"禁用" : L"启用");
            if (MessageBox(hWnd, msg, L"确认", MB_YESNO | MB_ICONWARNING) == IDYES) {
                BOOL ok = SetDeviceState(hDevInfo, &did, bDisable);
                SetupDiDestroyDeviceInfoList(hDevInfo);
                if (ok) {
                    SendMessage(hStatus, WM_SETTEXT, 0,
                        (LPARAM)(bDisable ? L"已发送禁用指令" : L"已发送启用指令"));
                    RefreshList();
                } else {
                    DWORD err = GetLastError();
                    TCHAR ebuf[128];
                    _stprintf_s(ebuf, _countof(ebuf), L"操作失败，错误码: %d", err);
                    MessageBox(hWnd, ebuf, L"错误", MB_OK | MB_ICONERROR);
                }
            } else {
                SetupDiDestroyDeviceInfoList(hDevInfo);
            }
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    hInst = hInstance;

    // 检查是否管理员
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt, 2, SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) isAdmin = FALSE;
        FreeSid(adminGroup);
    }
    if (!isAdmin) {
        TCHAR szExe[MAX_PATH];
        GetModuleFileName(NULL, szExe, MAX_PATH);
        ShellExecute(NULL, L"runas", szExe, NULL, NULL, SW_SHOWNORMAL);
        return 0;
    }

    const TCHAR* cls = L"DiskToggleWnd";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = cls;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(cls, L"磁盘禁用/启用工具 (管理员)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

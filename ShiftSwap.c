// Programme shift swap
// Ce programme desactive Verr Maj des l'appui sur shift (priorite aux minuscules).
// Shift + touche = majuscule momentanee, retour en minuscule au relachement.
// Application sans console : icone dans la zone de notification, clic droit pour
// activer le lancement au demarrage ou quitter.
//
// Compilation :
//   windres ShiftSwap.rc -O coff -o ShiftSwap_res.o
//   gcc -mwindows -O2 ShiftSwap.c ShiftSwap_res.o -o ShiftSwap.exe -luser32 -lshell32 -ladvapi32

#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_APP + 1)
#define ID_TRAY_QUIT 1001
#define ID_TRAY_AUTOSTART 1002

#define RUN_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RUN_VALUE "ShiftSwap"

static HHOOK hKeyHook = NULL;

static void ToggleCapsLock(void)
{
    INPUT in[2] = {0};

    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_CAPITAL;

    in[1] = in[0];
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, in, sizeof(INPUT));
}

static BOOL IsAutoStart(void)
{
    HKEY key;
    LONG res;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return FALSE;
    res = RegQueryValueEx(key, RUN_VALUE, NULL, NULL, NULL, NULL);
    RegCloseKey(key);
    return res == ERROR_SUCCESS;
}

static void SetAutoStart(BOOL enable)
{
    HKEY key;
    char path[MAX_PATH + 2];
    DWORD len;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return;

    if (enable)
    {
        // Chemin entre guillemets (gere les espaces)
        path[0] = '"';
        len = GetModuleFileName(NULL, path + 1, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            path[len + 1] = '"';
            path[len + 2] = 0;
            RegSetValueEx(key, RUN_VALUE, 0, REG_SZ, (const BYTE *)path, len + 3);
        }
    }
    else
        RegDeleteValue(key, RUN_VALUE);

    RegCloseKey(key);
}

LRESULT CALLBACK KeyboardProc(int nCode,
                              WPARAM wParam,
                              LPARAM lParam)
{
    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;

    // Ignore les touches injectees (dont notre propre CapsLock)
    if (nCode >= 0 && !(p->flags & LLKHF_INJECTED))
    {
        BOOL isShift = (p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT);

        // Priorite aux minuscules : des que Shift est enfonce, Verr Maj est coupee.
        // Shift + touche donne donc une majuscule momentanee, et le retour
        // en minuscule est automatique au relachement. Shift seul ne bascule rien.
        if (isShift && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) &&
            (GetKeyState(VK_CAPITAL) & 1))
            ToggleCapsLock();
    }

    return CallNextHookEx(hKeyHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
        {
            POINT pt;
            HMENU menu = CreatePopupMenu();

            GetCursorPos(&pt);
            AppendMenu(menu, MF_STRING | (IsAutoStart() ? MF_CHECKED : 0),
                       ID_TRAY_AUTOSTART, "Lancer au demarrage");
            AppendMenu(menu, MF_STRING, ID_TRAY_QUIT, "Quitter");

            // Necessaire pour que le menu se ferme au clic ailleurs
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);
            DestroyMenu(menu);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_AUTOSTART)
            SetAutoStart(!IsAutoStart());
        else if (LOWORD(wParam) == ID_TRAY_QUIT)
            DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int nShow)
{
    MSG msg;
    WNDCLASS wc = {0};
    HWND hwnd;
    NOTIFYICONDATA nid = {0};
    HANDLE mutex;

    (void)hPrev;
    (void)cmdLine;
    (void)nShow;

    // Une seule instance a la fois
    mutex = CreateMutex(NULL, TRUE, "ShiftSwap_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return 0;

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "ShiftSwapWnd";
    RegisterClass(&wc);

    hwnd = CreateWindow(wc.lpszClassName, "ShiftSwap", 0,
                        0, 0, 0, 0, NULL, NULL, hInst, NULL);
    if (hwnd == NULL)
        return 1;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(1));
    if (nid.hIcon == NULL)
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, "ShiftSwap - Shift force les minuscules");
    Shell_NotifyIcon(NIM_ADD, &nid);

    hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc,
                                GetModuleHandle(NULL), 0);

    if (hKeyHook == NULL)
    {
        MessageBox(NULL, "Impossible d'installer le hook clavier.",
                   "ShiftSwap", MB_OK | MB_ICONERROR);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        return 1;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKeyHook);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    ReleaseMutex(mutex);
    CloseHandle(mutex);

    return 0;
}

// Drives the options dialog automatically and logs the engine state *while the dialog is open*.
// Usage: uipreview.exe [dll_path] [live|cancel]
//   live   : tick "enable" after ~2s, then press OK after ~5s
//   cancel : tick "enable" after ~2s, then close with cancel after ~5s (state must roll back)
#include <windows.h>

#include <cstdio>

#include "PluginInterface.h"

namespace
{
    const wchar_t* kDefaultDll = L"bin\\x64\\Release\\MouseJiggler.dll";
    const wchar_t* kLogPath = L"C:\\tmp\\ui_test.log";

    ITMPlugin* g_plugin = nullptr;
    volatile LONG g_stop_poll = 0;
    FILE* g_log = nullptr;

    const wchar_t* DllPath(int argc, wchar_t** argv)
    {
        return (argc > 1 && _wcsicmp(argv[1], L"live") != 0 && _wcsicmp(argv[1], L"cancel") != 0)
            ? argv[1]
            : kDefaultDll;
    }

    const wchar_t* Mode(int argc, wchar_t** argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (_wcsicmp(argv[i], L"live") == 0) return L"live";
            if (_wcsicmp(argv[i], L"cancel") == 0) return L"cancel";
        }
        return L"live";
    }

    void Log(const wchar_t* fmt, ...)
    {
        if (g_log == nullptr)
            return;
        va_list args;
        va_start(args, fmt);
        vfwprintf(g_log, fmt, args);
        va_end(args);
        fflush(g_log);
    }

    HWND FindOwnDialog()
    {
        struct Finder { DWORD pid; HWND found; } finder{ ::GetCurrentProcessId(), nullptr };
        ::EnumWindows([](HWND hwnd, LPARAM param) -> BOOL
        {
            auto* f = reinterpret_cast<Finder*>(param);
            DWORD pid = 0;
            ::GetWindowThreadProcessId(hwnd, &pid);
            if (pid == f->pid)
            {
                wchar_t cls[64]{};
                ::GetClassNameW(hwnd, cls, _countof(cls));
                if (wcscmp(cls, L"#32770") == 0) { f->found = hwnd; return FALSE; }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&finder));
        return finder.found;
    }

    // 对话框开着的时候每 500ms 采一次引擎状态，证明"勾一下立刻生效"
    DWORD WINAPI PollThread(LPVOID)
    {
        int tick = 0;
        while (::InterlockedCompareExchange(&g_stop_poll, 0, 0) == 0)
        {
            Log(L"  [poll %2ds] value=%s | tooltip=%s\n", tick,
                g_plugin->GetItem(0)->GetItemValueText(), g_plugin->GetTooltipInfo());
            ::Sleep(500);
            tick = (tick + 1) / 2;
        }
        return 0;
    }

    DWORD WINAPI AutoOperate(LPVOID param)
    {
        const bool press_ok = (*static_cast<bool*>(param));

        HWND dlg = nullptr;
        for (int i = 0; i < 80 && dlg == nullptr; ++i)
        {
            ::Sleep(100);
            dlg = FindOwnDialog();
        }
        if (dlg == nullptr)
        {
            Log(L"dialog not found\n");
            return 0;
        }

        ::Sleep(2000);
        HWND enable_check = ::GetDlgItem(dlg, 1001);   // IDC_ENABLE_CHECK
        const LRESULT before = ::SendMessageW(enable_check, BM_GETCHECK, 0, 0);
        ::SendMessageW(enable_check, BM_CLICK, 0, 0);  // 真实点击：切换 + 发 BN_CLICKED
        const LRESULT after = ::SendMessageW(enable_check, BM_GETCHECK, 0, 0);
        Log(L"clicked enable checkbox: %s -> %s\n", before == BST_CHECKED ? L"checked" : L"unchecked",
            after == BST_CHECKED ? L"checked" : L"unchecked");

        ::Sleep(3000);
        if (press_ok)
        {
            Log(L"pressing OK\n");
            ::SendMessageW(dlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), reinterpret_cast<LPARAM>(::GetDlgItem(dlg, IDOK)));
        }
        else
        {
            Log(L"pressing Cancel\n");
            ::SendMessageW(dlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), reinterpret_cast<LPARAM>(::GetDlgItem(dlg, IDCANCEL)));
        }
        return 0;
    }
}

int wmain(int argc, wchar_t** argv)
{
    ::SetProcessDPIAware();

    _wfopen_s(&g_log, kLogPath, L"w, ccs=UTF-8");
    Log(L"mode=%s dll=%s\n", Mode(argc, argv), DllPath(argc, argv));

    HMODULE module = ::LoadLibraryW(DllPath(argc, argv));
    if (module == nullptr)
    {
        Log(L"LoadLibrary failed: %lu\n", ::GetLastError());
        return 1;
    }

    auto get_instance = reinterpret_cast<ITMPlugin * (*)()>(::GetProcAddress(module, "TMPluginGetInstance"));
    g_plugin = get_instance();
    g_plugin->OnInitialize(nullptr);
    ::Sleep(1200);

    Log(L"before dialog: value=%s\n", g_plugin->GetItem(0)->GetItemValueText());

    ::CreateThread(nullptr, 0, PollThread, nullptr, 0, nullptr);

    bool press_ok = (_wcsicmp(Mode(argc, argv), L"live") == 0);
    ::CreateThread(nullptr, 0, AutoOperate, &press_ok, 0, nullptr);

    g_plugin->ShowOptionsDialog(nullptr);
    ::InterlockedExchange(&g_stop_poll, 1);

    ::Sleep(1500);
    wchar_t ini_enable[32]{};
    ::GetPrivateProfileStringW(L"general", L"enable", L"?", ini_enable, _countof(ini_enable),
                               L"bin\\x64\\Release\\plugins\\MouseJiggler.ini");
    Log(L"after dialog : value=%s | ini.enable=%s | tooltip=%s\n",
        g_plugin->GetItem(0)->GetItemValueText(), ini_enable, g_plugin->GetTooltipInfo());

    ::FreeLibrary(module);
    if (g_log != nullptr)
        fclose(g_log);
    return 0;
}

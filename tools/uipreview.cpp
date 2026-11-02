// Opens the plugin options dialog for visual inspection, auto closes it after a few seconds.
#include <windows.h>

#include <cstdio>

#include "PluginInterface.h"

namespace
{
    // 默认加载 x64 Release 版本，可用第一个参数覆盖
    const wchar_t* DllPath(int argc, wchar_t** argv)
    {
        static const wchar_t* kDefault = L"bin\\x64\\Release\\MouseJiggler.dll";
        return (argc > 1) ? argv[1] : kDefault;
    }
}

namespace
{
    DWORD WINAPI CloseLater(LPVOID)
    {
        ::Sleep(5000);
        HWND dlg = ::FindWindowW(nullptr, L"MouseJiggler 选项");
        if (dlg != nullptr)
            ::PostMessageW(dlg, WM_CLOSE, 0, 0);
        return 0;
    }
}

int wmain(int argc, wchar_t** argv)
{
    // Make this process DPI aware so the dialog renders (and is measured) in real pixels
    ::SetProcessDPIAware();

    HMODULE module = ::LoadLibraryW(DllPath(argc, argv));
    if (module == nullptr)
    {
        wprintf(L"LoadLibrary failed: %lu\n", ::GetLastError());
        return 1;
    }

    auto get_instance = reinterpret_cast<ITMPlugin * (*)()>(::GetProcAddress(module, "TMPluginGetInstance"));
    ITMPlugin* plugin = get_instance();

    plugin->OnInitialize(nullptr);
    ::Sleep(1500);   // let the engine accumulate a few jiggles

    ::CreateThread(nullptr, 0, CloseLater, nullptr, 0, nullptr);
    plugin->ShowOptionsDialog(nullptr);

    ::FreeLibrary(module);
    return 0;
}

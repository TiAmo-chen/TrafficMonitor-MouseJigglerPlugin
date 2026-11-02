// Timing test: watch the jiggle counter over a few seconds.
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

int wmain(int argc, wchar_t** argv)
{
    HMODULE module = ::LoadLibraryW(DllPath(argc, argv));
    auto get_instance = reinterpret_cast<ITMPlugin * (*)()>(::GetProcAddress(module, "TMPluginGetInstance"));
    ITMPlugin* plugin = get_instance();
    plugin->OnInitialize(nullptr);

    IPluginItem* item = plugin->GetItem(0);
    for (int i = 0; i < 12; ++i)
    {
        plugin->DataRequired();
        const wchar_t* tooltip = plugin->GetTooltipInfo();
        wprintf(L"t=%4dms value=%s tooltip=%s\n", i * 250, item->GetItemValueText(), tooltip);
        ::Sleep(250);
    }

    ::FreeLibrary(module);
    return 0;
}

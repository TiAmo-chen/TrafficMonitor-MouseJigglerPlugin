// Timing test: watch the jiggle counter over a few seconds.
#include <windows.h>

#include <cstdio>

#include "PluginInterface.h"

int wmain()
{
    HMODULE module = ::LoadLibraryW(L"bin\\Release\\MouseJiggler.dll");
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

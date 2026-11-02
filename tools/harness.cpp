// Standalone smoke test for the plugin dll: load it, query the interface, run a few ticks.
#include <windows.h>

#include <cstdio>

#include "PluginInterface.h"

int wmain()
{
    HMODULE module = ::LoadLibraryW(L"bin\\Release\\MouseJiggler.dll");
    if (module == nullptr)
    {
        wprintf(L"LoadLibrary failed, GetLastError=%lu\n", ::GetLastError());
        return 1;
    }

    auto get_instance = reinterpret_cast<ITMPlugin * (*)()>(::GetProcAddress(module, "TMPluginGetInstance"));
    if (get_instance == nullptr)
    {
        wprintf(L"GetProcAddress(TMPluginGetInstance) failed\n");
        return 2;
    }

    ITMPlugin* plugin = get_instance();
    wprintf(L"api version = %d\n", plugin->GetAPIVersion());

    for (int i = 0; i < ITMPlugin::TMI_MAX; ++i)
    {
        const wchar_t* text = plugin->GetInfo(static_cast<ITMPlugin::PluginInfoIndex>(i));
        wprintf(L"info[%d] = %s%s\n", i, text != nullptr ? text : L"", text == nullptr ? L"  <-- NULL!" : L"");
    }

    int index = 0;
    while (IPluginItem* item = plugin->GetItem(index))
    {
        wprintf(L"item[%d] name=%s id=%s label=%s value=%s sample=%s\n", index,
                item->GetItemName(), item->GetItemId(), item->GetItemLableText(),
                item->GetItemValueText(), item->GetItemValueSampleText());
        ++index;
    }
    wprintf(L"item count = %d (GetItem(1) = %p)\n", index, static_cast<void*>(plugin->GetItem(1)));

    wprintf(L"commands = %d\n", plugin->GetCommandCount());
    for (int i = 0; i < plugin->GetCommandCount(); ++i)
    {
        const wchar_t* name = plugin->GetCommandName(i);
        wprintf(L"  cmd[%d] = %s, checked = %d\n", i, name != nullptr ? name : L"<null>", plugin->IsCommandChecked(i));
    }

    // No ITrafficMonitor available here: the plugin falls back to its own folder
    plugin->OnInitialize(nullptr);
    for (int i = 0; i < 6; ++i)
    {
        plugin->DataRequired();
        ::Sleep(300);
    }

    IPluginItem* item = plugin->GetItem(0);
    wprintf(L"after ticks: value=%s\n", item->GetItemValueText());
    wprintf(L"after ticks: tooltip=%s\n", plugin->GetTooltipInfo());

    ::FreeLibrary(module);
    wprintf(L"unloaded cleanly\n");
    return 0;
}

// Watches the plugin state over time: prints the display value / tooltip / ini state periodically.
// Usage: switchtest.exe [dll_path] [seconds]
#include <windows.h>

#include <cstdio>
#include <string>

#include "PluginInterface.h"

namespace
{
    const wchar_t* DllPath(int argc, wchar_t** argv)
    {
        static const wchar_t* kDefault = L"bin\\x64\\Release\\MouseJiggler.dll";
        return (argc > 1) ? argv[1] : kDefault;
    }

    int Seconds(int argc, wchar_t** argv)
    {
        if (argc > 2)
        {
            const int value = _wtoi(argv[2]);
            if (value > 0)
                return value;
        }
        return 300;
    }

    std::wstring ReadIniEnable()
    {
        wchar_t buffer[32]{};
        GetPrivateProfileStringW(L"general", L"enable", L"?", buffer, _countof(buffer), L"bin\\x64\\Release\\plugins\\MouseJiggler.ini");
        return buffer;
    }
}

int wmain(int argc, wchar_t** argv)
{
    // 宽字符直接重定向到文件时 wprintf 不输出，改成自己开 UTF-8 文件
    const wchar_t* log_path = (argc > 3) ? argv[3] : L"switchtest.log";
    FILE* log = nullptr;
    _wfopen_s(&log, log_path, L"w, ccs=UTF-8");

    HMODULE module = ::LoadLibraryW(DllPath(argc, argv));
    if (module == nullptr)
    {
        wprintf(L"LoadLibrary failed: %lu\n", ::GetLastError());
        return 1;
    }

    auto get_instance = reinterpret_cast<ITMPlugin * (*)()>(::GetProcAddress(module, "TMPluginGetInstance"));
    ITMPlugin* plugin = get_instance();
    plugin->OnInitialize(nullptr);
    IPluginItem* item = plugin->GetItem(0);

    const int total_seconds = Seconds(argc, argv);
    const bool toggle_test = (argc > 4) && (_wcsicmp(argv[4], L"toggle") == 0);
    const int step_seconds = 10;
    const DWORD start = ::GetTickCount();

    // 15 秒时模拟点击一次"启用鼠标晃动"命令，验证计划外手动开启
    bool toggle_done = false;

    for (int elapsed = 0; elapsed <= total_seconds; elapsed += 0)
    {
        if (toggle_test && !toggle_done && elapsed >= 15)
        {
            toggle_done = true;
            plugin->OnPluginCommand(0, nullptr, nullptr);
            if (log != nullptr) { fwprintf(log, L"           >>> toggle command fired\n"); fflush(log); }
        }

        SYSTEMTIME now{};
        ::GetLocalTime(&now);
        // DataRequired 每秒会被 TrafficMonitor 调用一次，这里模拟同样的节奏
        plugin->DataRequired();

        wprintf(L"%02d:%02d:%02d  t=%3ds  value=%-6s  ini.enable=%s  tooltip=%s\n",
                now.wHour, now.wMinute, now.wSecond, elapsed, item->GetItemValueText(), ReadIniEnable().c_str(),
                plugin->GetTooltipInfo());
        if (log != nullptr)
        {
            fwprintf(log, L"%02d:%02d:%02d  t=%3ds  value=%-6s  ini.enable=%s  tooltip=%s\n",
                     now.wHour, now.wMinute, now.wSecond, elapsed, item->GetItemValueText(), ReadIniEnable().c_str(),
                     plugin->GetTooltipInfo());
            fflush(log);
        }
        ::Sleep(step_seconds * 1000);
        elapsed = static_cast<int>(::GetTickCount() - start) / 1000;
    }

    ::FreeLibrary(module);
    if (log != nullptr)
        fclose(log);
    return 0;
}

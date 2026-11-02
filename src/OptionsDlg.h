#pragma once

#include <windows.h>

#include "JiggleEngine.h"
#include "Settings.h"

struct OptionsContext
{
    PluginSettings settings;
    CJiggleEngine* engine{ nullptr };
    int dpi{ 96 };
};

class COptionsDlg
{
public:
    // 返回 true 表示用户点了"确定"
    static bool Show(HWND parent, OptionsContext& context);
};

#pragma once

#include <windows.h>

#include <functional>

#include "JiggleEngine.h"
#include "Settings.h"

struct OptionsContext
{
    PluginSettings settings;          // 当前生效的设置（与引擎同步）
    PluginSettings original;          // 打开对话框时的快照，用于"取消"回滚
    CJiggleEngine* engine{ nullptr };
    int dpi{ 96 };
    bool changed{ false };
    // 改动即时生效：勾一下就把设置推给插件（应用 + 写盘）
    std::function<void(const PluginSettings&)> apply;
};

class COptionsDlg
{
public:
    // 返回 true 表示用户点了"确定"
    static bool Show(HWND parent, OptionsContext& context);
};

#pragma once

#include <windows.h>

#include <string>

#include "JiggleEngine.h"
#include "PluginInterface.h"
#include "Settings.h"

extern HINSTANCE g_hInst;

// Display item shown in the taskbar / main window
class CJiggleItem : public IPluginItem
{
public:
    explicit CJiggleItem(CJiggleEngine* engine) : m_engine(engine) {}

    const wchar_t* GetItemName() const override { return L"MouseJiggler"; }
    const wchar_t* GetItemId() const override { return L"MouseJiggler01"; }
    const wchar_t* GetItemLableText() const override { return L"晃动"; }
    const wchar_t* GetItemValueText() const override { return m_engine->ItemValueText(); }
    const wchar_t* GetItemValueSampleText() const override { return L"手动"; }

    int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

private:
    CJiggleEngine* m_engine;
};

class CMouseJigglerPlugin : public ITMPlugin
{
public:
    static CMouseJigglerPlugin& Instance();

    CJiggleEngine& Engine() { return m_engine; }
    PluginSettings& MutableSettings() { return m_settings; }
    int TaskbarDpi() const;

    void ToggleEnable();
    void ToggleZen();
    void ShowOptions(HWND parent);
    void ApplyAndSave();

    // ITMPlugin
    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    OptionReturn ShowOptionsDialog(void* hParent) override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    const wchar_t* GetTooltipInfo() override;
    void OnInitialize(ITrafficMonitor* pApp) override;
    void* GetPluginIcon() override;

    // Plugin commands, require interface version 5 or newer
    int GetCommandCount() override;
    const wchar_t* GetCommandName(int command_index) override;
    void* GetCommandIcon(int command_index) override;
    void OnPluginCommand(int command_index, void* hWnd, void* para) override;
    int IsCommandChecked(int command_index) override;

private:
    CMouseJigglerPlugin();
    ~CMouseJigglerPlugin();

    static CMouseJigglerPlugin m_instance;

    CJiggleEngine m_engine;
    CJiggleItem m_item{ &m_engine };
    ITrafficMonitor* m_app{ nullptr };
    HICON m_icon{ nullptr };
    PluginSettings m_settings;
    std::wstring m_config_path;
};

#include "MouseJigglerPlugin.h"

#include "OptionsDlg.h"
#include "resource.h"

HINSTANCE g_hInst = nullptr;

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInst = module;
        ::DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

CMouseJigglerPlugin CMouseJigglerPlugin::m_instance;

CMouseJigglerPlugin::CMouseJigglerPlugin() = default;

CMouseJigglerPlugin::~CMouseJigglerPlugin()
{
    m_engine.Stop();
    if (!m_config_path.empty())
        SettingsStore::Save(m_config_path, m_settings);
    if (m_icon != nullptr)
    {
        ::DestroyIcon(m_icon);
        m_icon = nullptr;
    }
}

CMouseJigglerPlugin& CMouseJigglerPlugin::Instance()
{
    return m_instance;
}

int CMouseJigglerPlugin::TaskbarDpi() const
{
    if (m_app != nullptr)
    {
        const int dpi = m_app->GetDPI(ITrafficMonitor::DPI_TASKBAR);
        if (dpi > 0)
            return dpi;
    }
    return 96;
}

void CMouseJigglerPlugin::OnInitialize(ITrafficMonitor* pApp)
{
    m_app = pApp;

    std::wstring config_dir;
    if (pApp != nullptr)
        config_dir = pApp->GetPluginConfigDir();

    if (config_dir.empty())
    {
        // Fallback: plugins folder next to the plugin dll
        wchar_t module_path[MAX_PATH]{};
        ::GetModuleFileNameW(g_hInst, module_path, MAX_PATH);
        const std::wstring path{ module_path };
        const size_t pos = path.find_last_of(L"\\/");
        if (pos != std::wstring::npos)
            config_dir = path.substr(0, pos + 1) + L"plugins\\";
    }

    if (!config_dir.empty() && config_dir.back() != L'\\')
        config_dir += L'\\';
    if (!config_dir.empty())
        ::CreateDirectoryW(config_dir.c_str(), nullptr);

    m_config_path = config_dir + L"MouseJiggler.ini";

    SettingsStore::Load(m_config_path, m_settings);
    if (!m_settings.start_on_load)
        m_settings.enable = false;

    m_engine.Start(m_settings, config_dir + L"MouseJiggler.log");
    SettingsStore::Save(m_config_path, m_settings);
}

IPluginItem* CMouseJigglerPlugin::GetItem(int index)
{
    if (index == 0)
        return &m_item;
    return nullptr;
}

void CMouseJigglerPlugin::DataRequired()
{
    if (m_app == nullptr)
        return;

    std::wstring message = m_engine.TakeNotification();
    while (!message.empty())
    {
        m_app->ShowNotifyMessage(message.c_str());
        message = m_engine.TakeNotification();
    }
}

const wchar_t* CMouseJigglerPlugin::GetTooltipInfo()
{
    return m_engine.TooltipText();
}

const wchar_t* CMouseJigglerPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"MouseJiggler";
    case TMI_DESCRIPTION:
        return L"定时晃动鼠标，让系统保持活动状态，避免锁屏、休眠或 IM 变成离开。"
               L"支持禅动（指针不动）和普通（±4 像素微动）两种方式，可限定只在工作时间生效。";
    case TMI_AUTHOR:
        return L"mousejiggler-plugin contributors";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026";
    case TMI_URL:
        return L"https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin";
    case TMI_VERSION:
        return L"1.0";
    default:
        return L"";
    }
}

void* CMouseJigglerPlugin::GetPluginIcon()
{
    if (m_icon == nullptr)
    {
        const int size = ::MulDiv(16, TaskbarDpi(), 96);
        m_icon = static_cast<HICON>(::LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_PLUGIN_ICON), IMAGE_ICON,
                                                 size, size, LR_DEFAULTCOLOR));
    }
    return m_icon;
}

void CMouseJigglerPlugin::ApplyAndSave()
{
    m_engine.Apply(m_settings);
    if (!m_config_path.empty())
        SettingsStore::Save(m_config_path, m_settings);
}

void CMouseJigglerPlugin::ToggleEnable()
{
    m_settings.enable = !m_settings.enable;
    ApplyAndSave();

    if (m_app != nullptr)
    {
        const CJiggleEngine::Status status = m_engine.GetStatus();
        if (m_settings.enable && !status.in_schedule)
            m_app->ShowNotifyMessage(L"MouseJiggler：当前不在工作时间，只在周一~周五 08:30-17:30 内晃动。");
        else
            m_app->ShowNotifyMessage(m_settings.enable ? L"MouseJiggler：已开始晃动。" : L"MouseJiggler：已停止晃动。");
    }
}

void CMouseJigglerPlugin::ToggleZen()
{
    m_settings.zen = !m_settings.zen;
    ApplyAndSave();

    if (m_app != nullptr)
        m_app->ShowNotifyMessage(m_settings.zen ? L"MouseJiggler：禅动模式（指针不动）。" : L"MouseJiggler：普通模式（±4 像素微动）。");
}

void CMouseJigglerPlugin::ShowOptions(HWND parent)
{
    OptionsContext context;
    context.settings = m_settings;
    context.engine = &m_engine;
    context.dpi = TaskbarDpi();

    if (COptionsDlg::Show(parent, context))
    {
        m_settings = context.settings;
        ApplyAndSave();
    }
}

ITMPlugin::OptionReturn CMouseJigglerPlugin::ShowOptionsDialog(void* hParent)
{
    OptionsContext context;
    context.settings = m_settings;
    context.engine = &m_engine;
    context.dpi = TaskbarDpi();

    if (!COptionsDlg::Show(static_cast<HWND>(hParent), context))
        return OR_OPTION_UNCHANGED;

    m_settings = context.settings;
    ApplyAndSave();
    return OR_OPTION_CHANGED;
}

int CMouseJigglerPlugin::GetCommandCount()
{
    return 4;
}

const wchar_t* CMouseJigglerPlugin::GetCommandName(int command_index)
{
    switch (command_index)
    {
    case 0: return L"启用鼠标晃动";
    case 1: return L"禅动模式";
    case 2: return L"立即晃动一次";
    case 3: return L"选项…";
    default: return nullptr;
    }
}

void* CMouseJigglerPlugin::GetCommandIcon(int command_index)
{
    if (command_index < 0 || command_index >= GetCommandCount())
        return nullptr;
    return GetPluginIcon();
}

void CMouseJigglerPlugin::OnPluginCommand(int command_index, void* hWnd, void* /*para*/)
{
    switch (command_index)
    {
    case 0:
        ToggleEnable();
        break;
    case 1:
        ToggleZen();
        break;
    case 2:
        m_engine.RequestJiggleOnce();
        break;
    case 3:
        ShowOptions(static_cast<HWND>(hWnd));
        break;
    default:
        break;
    }
}

int CMouseJigglerPlugin::IsCommandChecked(int command_index)
{
    switch (command_index)
    {
    case 0: return m_settings.enable ? 1 : 0;
    case 1: return m_settings.zen ? 1 : 0;
    default: return 0;
    }
}

int CJiggleItem::OnMouseEvent(MouseEventType type, int /*x*/, int /*y*/, void* /*hWnd*/, int /*flag*/)
{
    if (type == MT_LCLICKED)
    {
        CMouseJigglerPlugin::Instance().ToggleEnable();
        return 1;
    }
    // Right click and double click fall through to the TrafficMonitor context menu
    return 0;
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

ITMPlugin* TMPluginGetInstance()
{
    return &CMouseJigglerPlugin::Instance();
}

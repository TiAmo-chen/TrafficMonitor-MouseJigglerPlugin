#include "OptionsDlg.h"

#include <commctrl.h>

#include <cstdio>

#include "MouseJigglerPlugin.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
    constexpr UINT_PTR kRefreshTimer = 1;
    constexpr UINT kRefreshIntervalMs = 300;

    OptionsContext* GetContext(HWND dlg)
    {
        return reinterpret_cast<OptionsContext*>(::GetWindowLongPtrW(dlg, GWLP_USERDATA));
    }

    // Cached dialog font, created once per DPI to avoid leaking a font per dialog instance
    HFONT GetDialogFont(int dpi)
    {
        static HFONT cached_font = nullptr;
        static int cached_dpi = 0;

        const int use_dpi = dpi > 0 ? dpi : 96;
        if (cached_font != nullptr && cached_dpi == use_dpi)
            return cached_font;

        if (cached_font != nullptr)
        {
            ::DeleteObject(cached_font);
            cached_font = nullptr;
        }

        LOGFONTW log_font{};
        log_font.lfHeight = -::MulDiv(9, use_dpi, 72);
        log_font.lfWeight = FW_NORMAL;
        log_font.lfCharSet = DEFAULT_CHARSET;
        wcscpy_s(log_font.lfFaceName, L"微软雅黑");

        cached_font = ::CreateFontIndirectW(&log_font);
        cached_dpi = use_dpi;
        if (cached_font == nullptr)
            return static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
        return cached_font;
    }

    void ApplyFontToAll(HWND dlg, HFONT font)
    {
        ::SendMessageW(dlg, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        for (HWND child = ::GetWindow(dlg, GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
            ::SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    // The dialog template is designed for 96 DPI, scale everything for higher DPI
    void ScaleForDpi(HWND dlg, int dpi)
    {
        if (dpi <= 0 || dpi == 96)
            return;

        RECT rc{};
        ::GetWindowRect(dlg, &rc);
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;

        for (HWND child = ::GetWindow(dlg, GW_CHILD); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
        {
            RECT r{};
            ::GetWindowRect(child, &r);
            POINT points[2] = { { r.left, r.top }, { r.right, r.bottom } };
            ::MapWindowPoints(HWND_DESKTOP, dlg, points, 2);
            ::SetWindowPos(child, nullptr,
                           ::MulDiv(points[0].x, dpi, 96), ::MulDiv(points[0].y, dpi, 96),
                           ::MulDiv(points[1].x - points[0].x, dpi, 96), ::MulDiv(points[1].y - points[0].y, dpi, 96),
                           SWP_NOZORDER | SWP_NOACTIVATE);
        }

        ::SetWindowPos(dlg, nullptr, 0, 0, ::MulDiv(width, dpi, 96), ::MulDiv(height, dpi, 96),
                       SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void CreateIntervalSpin(HWND dlg, int dpi)
    {
        HWND edit = ::GetDlgItem(dlg, IDC_INTERVAL_EDIT);
        if (edit == nullptr)
            return;

        HWND spin = ::CreateWindowExW(0, UPDOWN_CLASSW, nullptr,
                                      WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                      0, 0, 0, 0, dlg,
                                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INTERVAL_SPIN)),
                                      g_hInst, nullptr);
        if (spin == nullptr)
            return;

        ::SendMessageW(spin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(edit), 0);
        ::SendMessageW(spin, UDM_SETRANGE32, 1, 60);
        ::SendMessageW(spin, UDM_SETPOS32, 0, static_cast<LPARAM>(::GetDlgItemInt(dlg, IDC_INTERVAL_EDIT, nullptr, FALSE)));

        const HFONT font = reinterpret_cast<HFONT>(::SendMessageW(dlg, WM_GETFONT, 0, 0));
        ::SendMessageW(spin, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

        RECT r{};
        ::GetWindowRect(edit, &r);
        ::MapWindowPoints(HWND_DESKTOP, dlg, reinterpret_cast<POINT*>(&r), 2);
        ::SetWindowPos(spin, nullptr, 0, 0, ::MulDiv(12, dpi, 96), r.bottom - r.top,
                       SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    std::wstring BuildWindowLine(const OptionsContext* ctx, const CJiggleEngine::Status& status)
    {
        wchar_t start_text[8]{};
        wchar_t end_text[8]{};
        FormatHhMm(ctx->settings.schedule_start_min, start_text, _countof(start_text));
        FormatHhMm(ctx->settings.schedule_start_min + ctx->settings.schedule_duration_min, end_text, _countof(end_text));

        wchar_t buffer[256]{};
        swprintf_s(buffer, L"%s %s-%s",
                   ctx->settings.schedule_weekdays_only ? L"周一~周五" : L"每天", start_text, end_text);

        std::wstring line{ buffer };
        if (!ctx->settings.schedule_enabled)
        {
            line += L" · 计划未启用";
        }
        else if (status.in_schedule)
        {
            swprintf_s(buffer, L" · 计划内，%d 分钟后自动停止", status.minutes_to_change);
            line += buffer;
        }
        else
        {
            swprintf_s(buffer, L" · 计划外，%d 分钟后自动开始", status.minutes_to_change);
            line += buffer;
        }
        return line;
    }

    std::wstring FormatAge(unsigned long age_ms)
    {
        wchar_t buffer[32]{};
        if (age_ms < 1000)
            swprintf_s(buffer, L"%.1f 秒前", age_ms / 1000.0);
        else
            swprintf_s(buffer, L"%lu 秒前", age_ms / 1000);
        return buffer;
    }

    std::wstring BuildStatusLine(const CJiggleEngine::Status& status)
    {
        wchar_t buffer[256]{};
        if (status.running)
        {
            swprintf_s(buffer, L"运行中 · 每 %d 秒 · %s · 累计 %llu 次",
                       status.interval_sec, status.zen ? L"禅动" : L"普通", status.count);
            std::wstring line{ buffer };
            if (status.count > 0)
            {
                line += L" · ";
                line += FormatAge(status.last_jiggle_age_ms);
            }
            return line;
        }
        if (!status.enable)
            return L"已停止 · 手动关闭";
        if (status.schedule_enabled)
            return L"已停止 · 不在工作时间";
        return L"已停止";
    }

    void RefreshStatus(HWND dlg)
    {
        OptionsContext* ctx = GetContext(dlg);
        if (ctx == nullptr || ctx->engine == nullptr)
            return;

        // The schedule checkbox may have been toggled, use the UI value for the preview
        ctx->settings.schedule_enabled = ::IsDlgButtonChecked(dlg, IDC_SCHEDULE_CHECK) == BST_CHECKED;

        const CJiggleEngine::Status status = ctx->engine->GetStatus();
        const std::wstring window_line = BuildWindowLine(ctx, status);
        const std::wstring status_line = BuildStatusLine(status);
        ::SetDlgItemTextW(dlg, IDC_SCHEDULE_STATUS, window_line.c_str());
        ::SetDlgItemTextW(dlg, IDC_STATUS_STATIC, status_line.c_str());
    }

    void ReadControls(HWND dlg, OptionsContext& ctx)
    {
        ctx.settings.enable = ::IsDlgButtonChecked(dlg, IDC_ENABLE_CHECK) == BST_CHECKED;
        ctx.settings.zen = ::IsDlgButtonChecked(dlg, IDC_MODE_ZEN) == BST_CHECKED;
        ctx.settings.schedule_enabled = ::IsDlgButtonChecked(dlg, IDC_SCHEDULE_CHECK) == BST_CHECKED;
        ctx.settings.start_on_load = ::IsDlgButtonChecked(dlg, IDC_STARTONLOAD_CHECK) == BST_CHECKED;
        ctx.settings.skip_when_user_active = ::IsDlgButtonChecked(dlg, IDC_SKIPACTIVE_CHECK) == BST_CHECKED;

        BOOL translated = FALSE;
        UINT interval = ::GetDlgItemInt(dlg, IDC_INTERVAL_EDIT, &translated, FALSE);
        if (!translated || interval < 1) interval = 1;
        if (interval > 60) interval = 60;
        ctx.settings.interval_sec = static_cast<int>(interval);
        ::SetDlgItemInt(dlg, IDC_INTERVAL_EDIT, interval, FALSE);
    }

    INT_PTR CALLBACK DialogProc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_INITDIALOG:
        {
            OptionsContext* ctx = reinterpret_cast<OptionsContext*>(lParam);
            ::SetWindowLongPtrW(dlg, GWLP_USERDATA, lParam);
            if (ctx == nullptr)
                return TRUE;

            ::CheckDlgButton(dlg, IDC_ENABLE_CHECK, ctx->settings.enable ? BST_CHECKED : BST_UNCHECKED);
            ::CheckDlgButton(dlg, IDC_MODE_ZEN, ctx->settings.zen ? BST_CHECKED : BST_UNCHECKED);
            ::CheckDlgButton(dlg, IDC_MODE_NORMAL, ctx->settings.zen ? BST_UNCHECKED : BST_CHECKED);
            ::CheckDlgButton(dlg, IDC_SCHEDULE_CHECK, ctx->settings.schedule_enabled ? BST_CHECKED : BST_UNCHECKED);
            ::CheckDlgButton(dlg, IDC_STARTONLOAD_CHECK, ctx->settings.start_on_load ? BST_CHECKED : BST_UNCHECKED);
            ::CheckDlgButton(dlg, IDC_SKIPACTIVE_CHECK, ctx->settings.skip_when_user_active ? BST_CHECKED : BST_UNCHECKED);
            ::SetDlgItemInt(dlg, IDC_INTERVAL_EDIT, static_cast<UINT>(ctx->settings.interval_sec), FALSE);

            // Keep the same look as the TrafficMonitor dialogs
            ApplyFontToAll(dlg, GetDialogFont(ctx->dpi));
            ScaleForDpi(dlg, ctx->dpi);

            INITCOMMONCONTROLSEX common{ sizeof(common), ICC_UPDOWN_CLASS };
            ::InitCommonControlsEx(&common);
            CreateIntervalSpin(dlg, ctx->dpi);

            HICON icon = static_cast<HICON>(::LoadImageW(g_hInst, MAKEINTRESOURCEW(IDI_PLUGIN_ICON), IMAGE_ICON,
                                                         ::GetSystemMetrics(SM_CXSMICON),
                                                         ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
            if (icon != nullptr)
            {
                ::SendMessageW(dlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
                ::SendMessageW(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
            }

            RefreshStatus(dlg);
            ::SetTimer(dlg, kRefreshTimer, kRefreshIntervalMs, nullptr);
            return TRUE;
        }

        case WM_TIMER:
            if (wParam == kRefreshTimer)
            {
                RefreshStatus(dlg);
                return TRUE;
            }
            break;

        case WM_COMMAND:
        {
            const int id = LOWORD(wParam);
            const int code = HIWORD(wParam);

            if (id == IDOK && code == BN_CLICKED)
            {
                ::KillTimer(dlg, kRefreshTimer);
                OptionsContext* ctx = GetContext(dlg);
                if (ctx != nullptr)
                    ReadControls(dlg, *ctx);
                ::EndDialog(dlg, IDOK);
                return TRUE;
            }

            if (id == IDCANCEL)
            {
                ::KillTimer(dlg, kRefreshTimer);
                ::EndDialog(dlg, IDCANCEL);
                return TRUE;
            }

            if (id == IDC_JIGGLE_ONCE && code == BN_CLICKED)
            {
                OptionsContext* ctx = GetContext(dlg);
                if (ctx != nullptr && ctx->engine != nullptr)
                    ctx->engine->RequestJiggleOnce();
                return TRUE;
            }

            if ((id == IDC_SCHEDULE_CHECK || id == IDC_ENABLE_CHECK) && code == BN_CLICKED)
            {
                RefreshStatus(dlg);
                return TRUE;
            }
            break;
        }

        case WM_CLOSE:
            ::KillTimer(dlg, kRefreshTimer);
            ::EndDialog(dlg, IDCANCEL);
            return TRUE;

        default:
            break;
        }
        return FALSE;
    }
}

bool COptionsDlg::Show(HWND parent, OptionsContext& context)
{
    const INT_PTR result = ::DialogBoxParamW(g_hInst, MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG), parent,
                                             DialogProc, reinterpret_cast<LPARAM>(&context));
    return result == IDOK;
}

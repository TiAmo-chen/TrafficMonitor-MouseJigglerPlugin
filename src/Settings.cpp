#include "Settings.h"

#include <windows.h>

namespace
{
    const wchar_t* kGeneral = L"general";
    const wchar_t* kSchedule = L"schedule";

    int ReadInt(const std::wstring& path, const wchar_t* section, const wchar_t* key, int fallback)
    {
        return static_cast<int>(::GetPrivateProfileIntW(section, key, fallback, path.c_str()));
    }

    void WriteInt(const std::wstring& path, const wchar_t* section, const wchar_t* key, int value)
    {
        wchar_t buffer[32];
        swprintf_s(buffer, L"%d", value);
        ::WritePrivateProfileStringW(section, key, buffer, path.c_str());
    }
}

namespace SettingsStore
{
    void Load(const std::wstring& ini_path, PluginSettings& settings)
    {
        settings.enable = ReadInt(ini_path, kGeneral, L"enable", settings.enable ? 1 : 0) != 0;
        settings.zen = ReadInt(ini_path, kGeneral, L"zen", settings.zen ? 1 : 0) != 0;
        settings.interval_sec = ReadInt(ini_path, kGeneral, L"interval", settings.interval_sec);
        settings.start_on_load = ReadInt(ini_path, kGeneral, L"start_on_load", settings.start_on_load ? 1 : 0) != 0;
        settings.skip_when_user_active = ReadInt(ini_path, kGeneral, L"skip_when_user_active", settings.skip_when_user_active ? 1 : 0) != 0;

        settings.schedule_enabled = ReadInt(ini_path, kSchedule, L"enabled", settings.schedule_enabled ? 1 : 0) != 0;
        settings.schedule_weekdays_only = ReadInt(ini_path, kSchedule, L"weekdays_only", settings.schedule_weekdays_only ? 1 : 0) != 0;
        settings.schedule_start_min = ReadInt(ini_path, kSchedule, L"start_min", settings.schedule_start_min);
        settings.schedule_duration_min = ReadInt(ini_path, kSchedule, L"duration_min", settings.schedule_duration_min);

        if (settings.interval_sec < 1) settings.interval_sec = 1;
        if (settings.interval_sec > 60) settings.interval_sec = 60;
        if (settings.schedule_start_min < 0 || settings.schedule_start_min >= 24 * 60) settings.schedule_start_min = 8 * 60 + 30;
        if (settings.schedule_duration_min < 1 || settings.schedule_duration_min > 24 * 60) settings.schedule_duration_min = 9 * 60;
    }

    void Save(const std::wstring& ini_path, const PluginSettings& settings)
    {
        WriteInt(ini_path, kGeneral, L"enable", settings.enable ? 1 : 0);
        WriteInt(ini_path, kGeneral, L"zen", settings.zen ? 1 : 0);
        WriteInt(ini_path, kGeneral, L"interval", settings.interval_sec);
        WriteInt(ini_path, kGeneral, L"start_on_load", settings.start_on_load ? 1 : 0);
        WriteInt(ini_path, kGeneral, L"skip_when_user_active", settings.skip_when_user_active ? 1 : 0);

        WriteInt(ini_path, kSchedule, L"enabled", settings.schedule_enabled ? 1 : 0);
        WriteInt(ini_path, kSchedule, L"weekdays_only", settings.schedule_weekdays_only ? 1 : 0);
        WriteInt(ini_path, kSchedule, L"start_min", settings.schedule_start_min);
        WriteInt(ini_path, kSchedule, L"duration_min", settings.schedule_duration_min);
    }
}

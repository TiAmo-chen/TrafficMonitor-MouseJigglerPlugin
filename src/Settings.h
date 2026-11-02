#pragma once

#include <string>

// All configurable options of the plugin
struct PluginSettings
{
    bool enable = true;                 // user wants the mouse to be jiggled
    bool zen = true;                    // zen mode: pointer does not move, zero-delta move event only
    int  interval_sec = 1;              // jiggle interval, 1-60 seconds
    bool start_on_load = true;          // start jiggling right after TrafficMonitor loads the plugin
    bool skip_when_user_active = false; // skip a jiggle while the user is moving the mouse
    bool schedule_enabled = true;       // only run inside the configured work window

    // Work window. Not exposed in the UI, can be overridden by editing the ini file.
    int  schedule_start_min = 8 * 60 + 30;   // 08:30
    int  schedule_duration_min = 9 * 60;     // 9 hours -> 17:30
    bool schedule_weekdays_only = true;
};

namespace SettingsStore
{
    void Load(const std::wstring& ini_path, PluginSettings& settings);
    void Save(const std::wstring& ini_path, const PluginSettings& settings);
}

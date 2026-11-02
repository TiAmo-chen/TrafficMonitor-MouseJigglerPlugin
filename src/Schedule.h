#pragma once

#include <windows.h>

#include <atomic>

// Work window scheduler: jiggling is allowed only inside a window on the allowed days.
// The window is described as (start minute, duration minutes) and may cross midnight.
class CSchedule
{
public:
    void Configure(int start_min, int duration_min, bool weekdays_only);
    void GetConfig(int& start_min, int& duration_min, bool& weekdays_only) const;

    // Is the given local time inside the window
    bool InWindow(const SYSTEMTIME& local) const;

    // Minutes until the next state change. change_is_start tells if that change enters the window.
    int MinutesToChange(const SYSTEMTIME& local, bool& change_is_start) const;

private:
    bool DayAllowed(int wday) const;

    std::atomic<int>  m_start{ 8 * 60 + 30 };
    std::atomic<int>  m_duration{ 9 * 60 };
    std::atomic<bool> m_weekdays_only{ true };
};

// "08:30" -> 510, fallback on parse error
int ParseHhMm(const wchar_t* text, int fallback);
void FormatHhMm(int minutes, wchar_t* buffer, int buffer_count);

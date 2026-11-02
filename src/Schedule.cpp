#include "Schedule.h"

#include <cwchar>

namespace
{
    constexpr int kMinutesPerDay = 24 * 60;

    int ClampDuration(int duration)
    {
        if (duration < 1) return 1;
        if (duration > kMinutesPerDay) return kMinutesPerDay;
        return duration;
    }
}

void CSchedule::Configure(int start_min, int duration_min, bool weekdays_only)
{
    if (start_min < 0 || start_min >= kMinutesPerDay) start_min = 8 * 60 + 30;
    m_start.store(start_min);
    m_duration.store(ClampDuration(duration_min));
    m_weekdays_only.store(weekdays_only);
}

void CSchedule::GetConfig(int& start_min, int& duration_min, bool& weekdays_only) const
{
    start_min = m_start.load();
    duration_min = m_duration.load();
    weekdays_only = m_weekdays_only.load();
}

bool CSchedule::DayAllowed(int wday) const
{
    if (!m_weekdays_only.load())
        return true;
    // SYSTEMTIME::wDayOfWeek: 0 = Sunday ... 6 = Saturday
    return wday >= 1 && wday <= 5;
}

bool CSchedule::InWindow(const SYSTEMTIME& local) const
{
    const int start = m_start.load();
    const int duration = ClampDuration(m_duration.load());
    const int minute = local.wHour * 60 + local.wMinute;

    if (start + duration <= kMinutesPerDay)
    {
        // Window stays inside one day
        return DayAllowed(local.wDayOfWeek) && minute >= start && minute < start + duration;
    }

    // Window crosses midnight: part on the starting day plus a tail on the next day
    if (minute >= start)
        return DayAllowed(local.wDayOfWeek);

    const int tail = start + duration - kMinutesPerDay;
    if (minute < tail)
    {
        const int previous_day = (local.wDayOfWeek + 6) % 7;
        return DayAllowed(previous_day);
    }
    return false;
}

int CSchedule::MinutesToChange(const SYSTEMTIME& local, bool& change_is_start) const
{
    const int start = m_start.load();
    const int duration = ClampDuration(m_duration.load());
    const int minute = local.wHour * 60 + local.wMinute;

    if (InWindow(local))
    {
        change_is_start = false;
        const int end = (start + duration) % kMinutesPerDay;
        int delta = (end - minute + kMinutesPerDay) % kMinutesPerDay;
        return delta == 0 ? kMinutesPerDay : delta;
    }

    change_is_start = true;
    int delta = (start - minute + kMinutesPerDay) % kMinutesPerDay;
    if (delta == 0)
        delta = kMinutesPerDay;

    if (m_weekdays_only.load())
    {
        // Skip forward over days that are not allowed
        int wday = local.wDayOfWeek;
        int guard = 0;
        while (!DayAllowed(wday) && guard < 7)
        {
            wday = (wday + 1) % 7;
            delta += kMinutesPerDay;
            ++guard;
        }

        // Today is allowed but the window already ended: move to the next allowed day
        if (guard == 0 && start + duration <= kMinutesPerDay && minute >= start + duration)
        {
            if (wday == 5)              // Friday -> Monday
                delta += 2 * kMinutesPerDay;
            else if (wday == 6 || wday == 0)
                delta += kMinutesPerDay;
        }
    }

    if (delta <= 0)
        delta = kMinutesPerDay;
    return delta;
}

int ParseHhMm(const wchar_t* text, int fallback)
{
    if (text == nullptr || *text == L'\0')
        return fallback;
    int hour = 0;
    int minute = 0;
    if (swscanf_s(text, L"%d:%d", &hour, &minute) != 2)
        return fallback;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
        return fallback;
    return hour * 60 + minute;
}

void FormatHhMm(int minutes, wchar_t* buffer, int buffer_count)
{
    if (buffer == nullptr || buffer_count <= 0)
        return;
    minutes = ((minutes % kMinutesPerDay) + kMinutesPerDay) % kMinutesPerDay;
    swprintf_s(buffer, static_cast<size_t>(buffer_count), L"%02d:%02d", minutes / 60, minutes % 60);
}

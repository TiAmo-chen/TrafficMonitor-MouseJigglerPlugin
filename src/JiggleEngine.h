#pragma once

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "Schedule.h"
#include "Settings.h"

// Double buffered text: the engine thread writes, the TrafficMonitor UI thread reads.
// The returned pointer stays valid until the next write.
class CTextSlot
{
public:
    void Set(const std::wstring& text);
    const wchar_t* Get() const;

private:
    std::wstring m_buf[2];
    std::atomic<int> m_index{ 0 };
    std::mutex m_write_mutex;
};

// Jiggle engine. Own worker thread, independent of the DataRequired cadence.
class CJiggleEngine
{
public:
    struct Status
    {
        bool enable{ true };
        bool schedule_enabled{ true };
        bool in_schedule{ true };
        bool zen{ true };
        bool running{ false };
        bool change_is_start{ false };
        int interval_sec{ 1 };
        int minutes_to_change{ 0 };
        unsigned long long count{ 0 };
        unsigned long last_jiggle_age_ms{ 0 };
        bool started{ false };
    };

    CJiggleEngine() = default;
    ~CJiggleEngine();

    CJiggleEngine(const CJiggleEngine&) = delete;
    CJiggleEngine& operator=(const CJiggleEngine&) = delete;

    void Start(const PluginSettings& settings, const std::wstring& log_path);
    void Stop();
    void Apply(const PluginSettings& settings);
    void RequestJiggleOnce();

    Status GetStatus() const;

    const wchar_t* ItemValueText() const { return m_value_text.Get(); }
    const wchar_t* TooltipText() const { return m_tooltip_text.Get(); }

    // Consumed by the UI thread (from DataRequired). Empty string means no pending message.
    std::wstring TakeNotification();

private:
    void ThreadProc();
    void Tick();
    void DoJiggle();
    void UpdateTexts(const SYSTEMTIME& local, bool running);
    void PushNotification(const std::wstring& text);
    void LogError(const wchar_t* what, DWORD error);
    bool EffectiveNow(const SYSTEMTIME& local) const;

    static constexpr int kJiggleDelta = 4;
    static constexpr int kTickMs = 200;

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop{ false };
    std::wstring m_log_path;

    CSchedule m_schedule;
    std::atomic<bool> m_enable{ true };
    std::atomic<bool> m_zen{ true };
    std::atomic<bool> m_schedule_enabled{ true };
    std::atomic<bool> m_skip_when_user_active{ false };
    std::atomic<int>  m_interval_sec{ 1 };
    std::atomic<bool> m_jiggle_once{ false };
    std::atomic<bool> m_started{ false };

    std::atomic<unsigned long long> m_count{ 0 };
    std::atomic<unsigned long long> m_last_jiggle{ 0 };

    // Engine thread only
    bool m_zig{ true };
    bool m_prev_in_schedule{ true };
    bool m_first_tick{ true };

    CTextSlot m_value_text;
    CTextSlot m_tooltip_text;

    mutable std::mutex m_notification_mutex;
    std::deque<std::wstring> m_notifications;
};

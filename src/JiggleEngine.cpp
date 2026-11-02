#include "JiggleEngine.h"

#include <cstdio>

void CTextSlot::Set(const std::wstring& text)
{
    std::lock_guard<std::mutex> lock(m_write_mutex);
    const int next = 1 - m_index.load(std::memory_order_relaxed);
    m_buf[next] = text;
    m_index.store(next, std::memory_order_release);
}

const wchar_t* CTextSlot::Get() const
{
    const int index = m_index.load(std::memory_order_acquire);
    return m_buf[index].c_str();
}

CJiggleEngine::~CJiggleEngine()
{
    Stop();
}

void CJiggleEngine::Start(const PluginSettings& settings, const std::wstring& log_path)
{
    m_log_path = log_path;
    Apply(settings);

    if (m_started.exchange(true))
        return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = false;
    }
    m_thread = std::thread(&CJiggleEngine::ThreadProc, this);
}

void CJiggleEngine::Stop()
{
    if (!m_started.exchange(false))
        return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();

    if (m_thread.joinable())
        m_thread.join();
}

void CJiggleEngine::Apply(const PluginSettings& settings)
{
    m_enable.store(settings.enable);
    m_zen.store(settings.zen);
    m_schedule_enabled.store(settings.schedule_enabled);
    m_skip_when_user_active.store(settings.skip_when_user_active);
    m_interval_sec.store(settings.interval_sec < 1 ? 1 : (settings.interval_sec > 60 ? 60 : settings.interval_sec));
    m_schedule.Configure(settings.schedule_start_min, settings.schedule_duration_min, settings.schedule_weekdays_only);
}

void CJiggleEngine::RequestJiggleOnce()
{
    m_jiggle_once.store(true);
}

CJiggleEngine::Status CJiggleEngine::GetStatus() const
{
    SYSTEMTIME now{};
    ::GetLocalTime(&now);

    Status status;
    status.enable = m_enable.load();
    status.zen = m_zen.load();
    status.schedule_enabled = m_schedule_enabled.load();
    status.interval_sec = m_interval_sec.load();
    status.started = m_started.load();
    status.in_schedule = !status.schedule_enabled || m_schedule.InWindow(now);
    status.running = status.started && status.enable && status.in_schedule;
    status.count = m_count.load();

    bool change_is_start = false;
    status.minutes_to_change = status.schedule_enabled ? m_schedule.MinutesToChange(now, change_is_start) : 0;
    status.change_is_start = change_is_start;

    const unsigned long long last = m_last_jiggle.load();
    const unsigned long long tick = ::GetTickCount64();
    status.last_jiggle_age_ms = (last != 0 && tick >= last) ? static_cast<unsigned long>(tick - last) : 0;
    return status;
}

std::wstring CJiggleEngine::TakeNotification()
{
    std::lock_guard<std::mutex> lock(m_notification_mutex);
    if (m_notifications.empty())
        return std::wstring();
    std::wstring text = m_notifications.front();
    m_notifications.pop_front();
    return text;
}

void CJiggleEngine::PushNotification(const std::wstring& text)
{
    std::lock_guard<std::mutex> lock(m_notification_mutex);
    if (m_notifications.size() < 8)
        m_notifications.push_back(text);
}

void CJiggleEngine::ThreadProc()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (true)
    {
        const bool should_stop = m_stop;
        lock.unlock();

        if (should_stop)
            break;

        Tick();

        lock.lock();
        if (m_stop)
            break;
        m_cv.wait_for(lock, std::chrono::milliseconds(kTickMs));
    }
}

bool CJiggleEngine::EffectiveNow(const SYSTEMTIME& local) const
{
    if (!m_enable.load())
        return false;
    if (m_schedule_enabled.load() && !m_schedule.InWindow(local))
        return false;
    return true;
}

void CJiggleEngine::Tick()
{
    SYSTEMTIME local{};
    ::GetLocalTime(&local);

    const bool running = EffectiveNow(local);
    const unsigned long long tick = ::GetTickCount64();

    if (m_jiggle_once.exchange(false))
    {
        // Manual one-shot jiggle ignores both the switch and the schedule
        DoJiggle();
        m_last_jiggle.store(tick);
    }
    else if (running)
    {
        const unsigned long long interval = static_cast<unsigned long long>(m_interval_sec.load()) * 1000ULL;
        if (tick - m_last_jiggle.load() >= interval)
        {
            bool skip = false;
            if (m_skip_when_user_active.load())
            {
                LASTINPUTINFO info{};
                info.cbSize = sizeof(info);
                if (::GetLastInputInfo(&info))
                {
                    const DWORD idle = ::GetTickCount() - info.dwTime;
                    if (idle < 1500)
                        skip = true;
                }
            }
            if (!skip)
            {
                DoJiggle();
                m_last_jiggle.store(tick);
            }
        }
    }
    else
    {
        // Reset the timer so that jiggling starts immediately once it becomes effective
        m_last_jiggle.store(tick);
    }

    UpdateTexts(local, running);
}

void CJiggleEngine::DoJiggle()
{
    int delta = 0;
    if (!m_zen.load())
    {
        delta = m_zig ? kJiggleDelta : -kJiggleDelta;
        m_zig = !m_zig;
    }

    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = delta;
    input.mi.dy = delta;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    if (::SendInput(1, &input, sizeof(INPUT)) != 1)
        LogError(L"SendInput", ::GetLastError());

    m_count.fetch_add(1);
}

void CJiggleEngine::UpdateTexts(const SYSTEMTIME& local, bool running)
{
    const bool schedule_enabled = m_schedule_enabled.load();
    const bool in_schedule = !schedule_enabled || m_schedule.InWindow(local);

    if (schedule_enabled)
    {
        if (m_first_tick)
        {
            m_prev_in_schedule = in_schedule;
        }
        else if (in_schedule != m_prev_in_schedule)
        {
            PushNotification(in_schedule ? L"MouseJiggler：已进入工作时间，自动开始晃动。"
                                         : L"MouseJiggler：已离开工作时间，自动停止晃动。");
            m_prev_in_schedule = in_schedule;
        }
    }
    m_first_tick = false;

    std::wstring value;
    if (!m_enable.load())
        value = L"关";
    else if (!in_schedule)
        value = L"计划外";
    else
        value = L"开";
    m_value_text.Set(value);

    std::wstring tooltip;
    wchar_t buffer[256];

    if (running)
    {
        swprintf_s(buffer, L"运行中 · 每 %d 秒 · %s", m_interval_sec.load(), m_zen.load() ? L"禅动" : L"普通");
        tooltip = buffer;

        if (schedule_enabled)
        {
            bool change_is_start = false;
            const int minutes = m_schedule.MinutesToChange(local, change_is_start);
            swprintf_s(buffer, L" · 距自动%s %d 分钟", change_is_start ? L"开始" : L"停止", minutes);
            tooltip += buffer;
        }
        else
        {
            tooltip += L" · 全天生效";
        }

        if (m_count.load() > 0)
        {
            swprintf_s(buffer, L" · 累计 %llu 次", m_count.load());
            tooltip += buffer;
        }
    }
    else if (!m_enable.load())
    {
        tooltip = L"已停止（手动关闭）";
    }
    else if (schedule_enabled)
    {
        bool change_is_start = false;
        const int minutes = m_schedule.MinutesToChange(local, change_is_start);
        swprintf_s(buffer, L"已停止 · 不在工作时间，%d 分钟后自动开始", minutes);
        tooltip = buffer;
    }
    else
    {
        tooltip = L"已停止";
    }

    m_tooltip_text.Set(tooltip);
}

void CJiggleEngine::LogError(const wchar_t* what, DWORD error)
{
    if (m_log_path.empty())
        return;

    FILE* file = nullptr;
    if (_wfopen_s(&file, m_log_path.c_str(), L"a, ccs=UTF-8") != 0 || file == nullptr)
        return;

    SYSTEMTIME local{};
    ::GetLocalTime(&local);
    fwprintf(file, L"[%04d-%02d-%02d %02d:%02d:%02d] %s failed, GetLastError=%lu\n",
             local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute, local.wSecond,
             what, static_cast<unsigned long>(error));
    fclose(file);
}

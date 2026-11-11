#pragma once
#include "animation_clock.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace tenhou {
    // UI-thread owned waitable timers; business/sound WM_TIMERs stay independent.
    class MotionTimer {
    public:
        MotionTimer() = default;

        MotionTimer(const MotionTimer &) = delete;

        MotionTimer &operator=(const MotionTimer &) = delete;

        ~MotionTimer() { Stop(); }

        void Start(HWND window, UINT_PTR id) {
            if (window_ == window && id_ == id) return;
            Stop();
            timer_ = CreateWaitableTimerExW(nullptr, nullptr, 0x2, TIMER_ALL_ACCESS);
            if (!timer_) timer_ = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS);
            if (!timer_) throw std::runtime_error("Cannot create motion waitable timer");
            window_ = window;
            id_ = id;
            try {
                registry_.push_back(this);
                RefreshRate();
                next_ = AnimationNow() + period_;
                Arm(next_);
            } catch (...) {
                Stop();
                throw;
            }
        }

        void Stop() {
            if (timer_) {
                CancelWaitableTimer(timer_);
                CloseHandle(timer_);
            }
            registry_.erase(std::remove(registry_.begin(), registry_.end(), this), registry_.end());
            timer_ = nullptr;
            window_ = nullptr;
            id_ = 0;
        }

        // Same filtering/dispatch contract as GetMessage, plus animation wakeups.
        static BOOL GetMessage(MSG *message) {
            for (;;) {
                for (auto *timer: registry_) {
                    const bool hidden = !IsWindowVisible(timer->window_) || IsIconic(
                                            GetAncestor(timer->window_, GA_ROOT));
                    if (hidden != timer->suspended_) {
                        timer->suspended_ = hidden;
                        const auto now = AnimationNow();
                        timer->next_ = now + timer->period_;
                        timer->Arm(hidden ? now + 250 : timer->next_);
                    }
                }
                std::array<HANDLE, MAXIMUM_WAIT_OBJECTS - 1> handles{};
                const auto count = static_cast<DWORD>(registry_.size());
                if (count > handles.size()) throw std::runtime_error("Too many animation timers");
                for (DWORD i = 0; i < count; ++i) handles[i] = registry_[i]->timer_;
                // A continuous input queue must not postpone an already-due frame.
                // Alternate a scheduled notification with ordinary input so neither
                // side can monopolize the dispatcher when a paint is expensive.
                if (!scheduled_last_ && count) {
                    const auto ready = WaitForMultipleObjects(count, handles.data(), FALSE, 0);
                    if (ready < WAIT_OBJECT_0 + count) {
                        if (registry_[ready - WAIT_OBJECT_0]->NextMessage(message)) {
                            scheduled_last_ = true;
                            return TRUE;
                        }
                        continue; // NextMessage may have removed an invalid window.
                    }
                }
                // Bound priority paints to monitor cadence. Painting after every
                // queued click would itself turn input bursts into a render backlog.
                if (!scheduled_last_ && AnimationNow() >= next_paint_ &&
                    PeekMessageW(message, nullptr, WM_PAINT, WM_PAINT, PM_REMOVE)) {
                    next_paint_ = AnimationNow() + PaintPeriod(message->hwnd);
                    scheduled_last_ = true;
                    return message->message == WM_QUIT ? FALSE : TRUE;
                }
                if (!scheduled_last_ && PeekMessageW(message, nullptr, WM_TIMER, WM_TIMER, PM_REMOVE)) {
                    scheduled_last_ = true;
                    return message->message == WM_QUIT ? FALSE : TRUE;
                }
                if (PeekMessageW(message, nullptr, 0, 0, PM_REMOVE)) {
                    scheduled_last_ = false;
                    return message->message == WM_QUIT ? FALSE : TRUE;
                }
                scheduled_last_ = false;
                const DWORD result = MsgWaitForMultipleObjectsEx(count, handles.data(), INFINITE, QS_ALLINPUT,
                                                                 MWMO_INPUTAVAILABLE);
                if (result == WAIT_FAILED) return -1;
                if (result < WAIT_OBJECT_0 + count && registry_[result - WAIT_OBJECT_0]->NextMessage(message)) return
                        TRUE;
            }
        }

    private:
        static double PaintPeriod(HWND window) {
            static double refresh_at = 0, period = 1000.0 / 60;
            if (AnimationNow() >= refresh_at) {
                MONITORINFOEXW info{};
                info.cbSize = sizeof(info);
                DEVMODEW mode{};
                mode.dmSize = sizeof(mode);
                if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info) &&
                    EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
                    period = 1000.0 / std::clamp(mode.dmDisplayFrequency, 30ul, 360ul);
                refresh_at = AnimationNow() + 500;
            }
            return period;
        }

        void RefreshRate() {
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            unsigned hz = 60;
            if (GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
                hz = mode.dmDisplayFrequency;
            period_ = 1000.0 / std::clamp(hz, 30u, 360u);
            refresh_at_ = AnimationNow() + 500;
        }

        void Arm(double deadline) {
            LARGE_INTEGER due{};
            due.QuadPart = -std::max<LONGLONG>(
                1, static_cast<LONGLONG>(std::ceil((deadline - AnimationNow()) * 10000.0)));
            if (!SetWaitableTimer(timer_, &due, 0, nullptr, nullptr, FALSE))
                throw std::runtime_error("Cannot arm motion waitable timer");
        }

        bool NextMessage(MSG *message) {
            if (!IsWindow(window_)) {
                Stop();
                return false;
            }
            const auto now = AnimationNow();
            if (now >= refresh_at_) RefreshRate();
            if ((!IsWindowVisible(window_) || IsIconic(GetAncestor(window_, GA_ROOT)))) {
                next_ = now + period_;
                Arm(now + 250);
                return false;
            }
            // Absolute QPC deadlines skip missed work. Return one notification
            // directly to the normal dispatcher; nothing accumulates in the queue.
            const auto missed = std::max(0.0, std::floor((now - next_) / period_));
            next_ += (missed + 1) * period_;
            if (next_ <= now) next_ = now + period_;
            Arm(next_);

            *message = {};
            message->hwnd = window_;
            message->message = WM_TIMER;
            message->wParam = id_;
            return true;
        }

        HWND window_ = nullptr;
        UINT_PTR id_ = 0;
        HANDLE timer_ = nullptr;
        bool suspended_ = false;
        double period_ = 1000.0 / 60, next_ = 0, refresh_at_ = 0;
        inline static std::vector<MotionTimer *> registry_;
        inline static bool scheduled_last_ = false;
        inline static double next_paint_ = 0;
    };
} // namespace tenhou

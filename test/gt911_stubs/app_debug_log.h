#pragma once
// Evaluate format arguments without output, preserving compile-time checks/use.
template <class... T> inline void fake_log(T &&...)
{
}
#define APP_TOUCH_DEBUG_LOGI(...) fake_log(__VA_ARGS__)
#define APP_TOUCH_DEBUG_LOGW(...) fake_log(__VA_ARGS__)
#define APP_TOUCH_DEBUG_LOGE(...) fake_log(__VA_ARGS__)

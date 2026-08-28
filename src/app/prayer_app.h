#pragma once

class Canvas;

// Runs one wake cycle and enters deep sleep after the page is refreshed.
// 运行一次唤醒周期，刷新页面后进入深度睡眠。
[[noreturn]] void prayer_app_run(Canvas &canvas);

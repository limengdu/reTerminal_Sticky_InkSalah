#!/usr/bin/env python3

import os
from pathlib import Path
import subprocess
import tempfile


def source_block(source: str, marker: str) -> str:
    """Extract one brace-delimited production definition or branch."""
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main() -> None:
    project_root = Path(__file__).resolve().parents[1]
    source = (project_root / "src/app/prayer_app.cpp").read_text()
    sleep_function = source_block(source, "[[noreturn]] void enter_deep_sleep(")
    unchanged_branch = source_block(
        source, "if (initial_cycle && woke_from_orientation &&"
    )
    mode = ""
    if "enum class SleepDisplayMode" in source:
        mode = source_block(source, "enum class SleepDisplayMode") + ";"

    # Execute the production sleep function and wake branch with host board hooks.
    # 使用电脑端硬件替身执行生产代码中的休眠函数和唤醒分支。
    harness = r'''
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "canvas.h"
#include "sleep_indicator.h"
#include "status_bar_layout.h"

using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
#define STICKY_LOGI(...) ((void)0)
#define STICKY_LOGW(...) ((void)0)

Canvas *active_canvas = nullptr;
std::vector<uint8_t> panel;
int refresh_count = 0;
int cancel_count = 0;
int touch_stop_count = 0;
int panel_sleep_count = 0;
int imu_arm_count = 0;
esp_err_t imu_result = ESP_OK;
esp_err_t touch_result = ESP_OK;
esp_err_t panel_result = ESP_OK;
struct SleepEntered { uint64_t timer; bool orientation; bool touch; uint32_t level; };

void require(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

esp_err_t sticky_imu_prepare_orientation_wakeup() { ++imu_arm_count; return imu_result; }
esp_err_t sticky_touch_stop() { ++touch_stop_count; return touch_result; }
void sticky_display_cancel_app_transition_refresh() { ++cancel_count; }
esp_err_t sticky_display_refresh_monochrome()
{
    ++refresh_count;
    panel.assign(active_canvas->data(), active_canvas->data() + panel.size());
    return ESP_OK;
}
esp_err_t sticky_display_sleep() { ++panel_sleep_count; return panel_result; }
[[noreturn]] void board_power_enter_deep_sleep(uint64_t timer, bool orientation,
                                               bool touch, uint32_t level)
{
    throw SleepEntered{timer, orientation, touch, level};
}
uint64_t sleep_interval_us(int, int) { return 578000000ULL; }

@MODE@
@SLEEP_FUNCTION@

bool run_wakeup_branch(Canvas &canvas, bool initial_cycle, bool woke_from_orientation,
                        CanvasRotation detected_rotation, CanvasRotation previous_rotation)
{
    const int next = 0;
    const int now = 0;
    @UNCHANGED_BRANCH@
    return true;
}

void reset_calls()
{
    refresh_count = cancel_count = touch_stop_count = panel_sleep_count = imu_arm_count = 0;
}

void check_sleep(const SleepEntered &sleep, uint64_t timer)
{
    require(sleep.timer == timer, "The scheduled wake interval is preserved");
    require(sleep.orientation == (imu_result == ESP_OK), "IMU readiness controls orientation wake");
    require(!sleep.touch && sleep.level == 0, "Touch wake configuration is preserved");
    require(touch_stop_count == 1 && imu_arm_count == 1 && panel_sleep_count == 1,
            "Sleep prepares the IMU, stops touch, and powers down the panel");
}

int main()
{
    constexpr size_t size = 800U * 480U / 4U;
    std::vector<uint8_t> buffer(size);
    Canvas canvas(800, 480, buffer.data(), size);
    active_canvas = &canvas;
    for (const auto rotation : {CanvasRotation::Deg0, CanvasRotation::Deg90Clockwise,
                                CanvasRotation::Deg180, CanvasRotation::Deg90CounterClockwise}) {
        canvas.set_rotation(rotation);
        for (int failure : {0, 1, 2, 3}) {
            imu_result = failure == 1 ? -1 : ESP_OK;
            touch_result = failure == 2 ? -1 : ESP_OK;
            panel_result = failure == 3 ? -1 : ESP_OK;
            for (int repeat = 0; repeat < 3; ++repeat) {
                canvas.clear();
                const auto blank_frame = buffer;
                panel.assign(size, 0x55);
                const auto retained_frame = panel;
                reset_calls();
                try {
                    run_wakeup_branch(canvas, true, true, rotation, rotation);
                    require(false, "An unchanged orientation returns to sleep");
                } catch (const SleepEntered &sleep) {
                    check_sleep(sleep, 578000000ULL);
                }
                require(panel == retained_frame, "An unchanged orientation preserves every retained panel pixel");
                require(buffer == blank_frame, "An unchanged orientation leaves the fresh canvas untouched");
                require(refresh_count == 0 && cancel_count == 0,
                        "An unchanged orientation sleeps without a display refresh");
            }
        }
        imu_result = touch_result = panel_result = ESP_OK;
        for (const auto detected : {CanvasRotation::Deg0, CanvasRotation::Deg90Clockwise,
                                   CanvasRotation::Deg180, CanvasRotation::Deg90CounterClockwise}) {
            if (detected != rotation)
                require(run_wakeup_branch(canvas, true, true, detected, rotation),
                        "A changed orientation continues to page rendering");
        }
        require(run_wakeup_branch(canvas, true, false, rotation, rotation),
                "Timer, button, and cold boots continue to page rendering");
        require(run_wakeup_branch(canvas, false, true, rotation, rotation),
                "Later application cycles continue to page rendering");

        for (bool prayer_header : {false, true}) {
            canvas.clear(GrayLevel::Black);
            canvas.set_prayer_header(prayer_header);
            canvas.fill_rect(100, 120, 160, 100, GrayLevel::White);
            auto before_sleep = buffer;
            panel.assign(size, 0xFF);
            reset_calls();
            try {
                enter_deep_sleep(canvas, 15000000ULL);
            } catch (const SleepEntered &sleep) {
                check_sleep(sleep, 15000000ULL);
            }
            require(refresh_count == 1 && cancel_count == 1,
                    "Normal sleep refreshes the complete canvas once");
            require(panel == buffer && buffer != before_sleep,
                    "Normal sleep publishes the canvas with its crescent");
            Canvas previous(800, 480, before_sleep.data(), size);
            previous.set_rotation(rotation);
            const auto moon = sleep_status_area(canvas);
            for (int y = 0; y < canvas.height(); ++y)
                for (int x = 0; x < canvas.width(); ++x)
                    if (x < moon.x || x >= moon.x + moon.width ||
                        y < moon.y || y >= moon.y + moon.height)
                        require(canvas.pixel_at(x, y) == previous.pixel_at(x, y),
                                "Normal sleep preserves all content outside the crescent");
        }
    }
    std::puts("PASS: retained panel, normal sleep, wake routing, and peripheral error paths");
}
'''
    harness = harness.replace("@MODE@", mode).replace(
        "@SLEEP_FUNCTION@", sleep_function
    ).replace("@UNCHANGED_BRANCH@", unchanged_branch)
    build_dir = Path(tempfile.mkdtemp(prefix="inksalah-sleep-test-"))
    test_source = build_dir / "prayer_sleep_display_test.cpp"
    test_source.write_text(harness)
    executable = build_dir / "prayer_sleep_display_test"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-Isrc/ui", str(test_source), "src/ui/canvas.cpp", "src/ui/font.cpp",
        "src/ui/sleep_indicator.cpp", "-o", str(executable),
    ], cwd=project_root, check=True)
    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()

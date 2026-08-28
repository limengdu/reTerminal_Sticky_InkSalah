#!/usr/bin/env bash

set -euo pipefail

# Compile hardware-independent regression tests with strict warnings.
# 使用严格警告选项编译全部不依赖硬件的回归测试。
readonly PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly BUILD_DIR="${TMPDIR:-/tmp}/sticky_salah_host_tests"
readonly CXX_BIN="${CXX:-g++}"
readonly COMMON_FLAGS=(-std=c++17 -Wall -Wextra -Werror)

mkdir -p "${BUILD_DIR}"
cd "${PROJECT_ROOT}"

passed_count=0

compile_and_run() {
    local test_name="$1"
    shift
    echo "[host-test] ${test_name}"
    "${CXX_BIN}" "${COMMON_FLAGS[@]}" "$@" \
        -o "${BUILD_DIR}/${test_name}"
    "${BUILD_DIR}/${test_name}"
    passed_count=$((passed_count + 1))
}

compile_and_run prayer_time_test \
    -Isrc/prayer \
    test/prayer_time_test.cpp \
    src/prayer/prayer_time.cpp

compile_and_run prayer_schedule_test \
    -Isrc/prayer \
    test/prayer_schedule_test.cpp \
    src/prayer/prayer_schedule.cpp \
    src/prayer/prayer_time.cpp

compile_and_run provisioning_form_test \
    -Isrc/network -Isrc/storage \
    test/provisioning_form_test.cpp \
    src/network/provisioning_form.cpp \
    src/storage/settings_model.cpp

compile_and_run prayer_pages_test \
    -Isrc/prayer -Isrc/storage -Isrc/ui \
    test/prayer_pages_test.cpp \
    src/prayer/prayer_time.cpp \
    src/storage/settings_model.cpp \
    src/ui/arabic_prayer_assets.cpp \
    src/ui/canvas.cpp src/ui/font.cpp src/ui/prayer_pages.cpp

compile_and_run display_orientation_test \
    -Isrc/app -Isrc/sensors -Isrc/ui \
    test/display_orientation_test.cpp \
    src/app/display_orientation.cpp

compile_and_run setup_network_readiness_test \
    -Isrc/network \
    test/setup_network_readiness_test.cpp \
    src/network/setup_network_readiness.cpp

echo "[host-test] PASS ${passed_count}/6"

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

check_http_header_capacity() {
    local minimum_length=4096
    local config_path
    local configured_length
    local config_paths=("sdkconfig.defaults")
    local generated_config_paths=(
        "sdkconfig.sticky-salah-debug"
        "sdkconfig.sticky-salah-release"
    )

    for config_path in "${generated_config_paths[@]}"; do
        if [[ -f "${config_path}" ]]; then
            config_paths+=("${config_path}")
        fi
    done

    echo "[host-test] http_header_capacity_test"
    for config_path in "${config_paths[@]}"; do
        configured_length="$(sed -n 's/^CONFIG_HTTPD_MAX_REQ_HDR_LEN=//p' "${config_path}")"
        if [[ ! "${configured_length}" =~ ^[0-9]+$ ]] ||
            (( configured_length < minimum_length )); then
            echo "${config_path}: CONFIG_HTTPD_MAX_REQ_HDR_LEN must be at least ${minimum_length}" >&2
            return 1
        fi
    done
    passed_count=$((passed_count + 1))
}

compile_and_run() {
    local test_name="$1"
    shift
    echo "[host-test] ${test_name}"
    "${CXX_BIN}" "${COMMON_FLAGS[@]}" "$@" \
        -o "${BUILD_DIR}/${test_name}"
    "${BUILD_DIR}/${test_name}"
    passed_count=$((passed_count + 1))
}

check_http_header_capacity

echo "[host-test] setup_portal_page_test"
python3 test/setup_portal_page_test.py
passed_count=$((passed_count + 1))

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
    src/ui/islamic_background_asset.cpp \
    src/ui/canvas.cpp src/ui/font.cpp src/ui/prayer_pages.cpp

compile_and_run display_orientation_test \
    -Isrc/app -Isrc/sensors -Isrc/ui \
    test/display_orientation_test.cpp \
    src/app/display_orientation.cpp

compile_and_run setup_network_readiness_test \
    -Isrc/network \
    test/setup_network_readiness_test.cpp \
    src/network/setup_network_readiness.cpp

compile_and_run http_retry_policy_test \
    -Isrc/services \
    test/http_retry_policy_test.cpp \
    src/services/http_retry_policy.cpp

compile_and_run captive_dns_protocol_test \
    -Isrc/network \
    test/captive_dns_protocol_test.cpp \
    src/network/captive_dns_protocol.cpp

compile_and_run wifi_scan_model_test \
    -Isrc/network \
    test/wifi_scan_model_test.cpp \
    src/network/wifi_scan_model.cpp

compile_and_run location_catalog_test \
    -Isrc/location \
    test/location_catalog_test.cpp \
    src/location/location_catalog.cpp

echo "[host-test] PASS ${passed_count}/12"

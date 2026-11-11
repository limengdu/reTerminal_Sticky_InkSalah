#!/usr/bin/env bash
set -euo pipefail

readonly PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly BUILD_DIR="${TMPDIR:-/tmp}/sticky_salah_gt911_host_tests"
readonly CXX_BIN="${CXX:-c++}"

mkdir -p "${BUILD_DIR}"
"${CXX_BIN}" -std=c++20 -Wall -Wextra -Werror \
    -I"${PROJECT_ROOT}/test/gt911_stubs" \
    -I"${PROJECT_ROOT}/components/gt911/include" \
    "${PROJECT_ROOT}/test/gt911_startup_test.cpp" \
    "${PROJECT_ROOT}/components/gt911/gt911.cpp" \
    -o "${BUILD_DIR}/gt911_startup_test"
"${BUILD_DIR}/gt911_startup_test" "$@"

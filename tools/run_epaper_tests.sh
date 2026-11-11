#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build="${TMPDIR:-/tmp}/sticky_salah_epaper_tests"
mkdir -p "$build"
bin="$build/epaper_driver_test"
"${CC:-cc}" -std=c11 -g -O1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
  ${EPAPER_TEST_CFLAGS:-} -Itest/epaper_stubs -Icomponents/seeed_epaper \
  test/epaper_driver_test.c components/seeed_epaper/epaper_panel.c \
  components/seeed_epaper/driver/{ssd1677,ssd2677,uc8179}.c -o "$bin"
"$bin"

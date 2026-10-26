#!/bin/sh
# Builds and runs the PC simulator of the UI (needs g++ and, for screenshots, Python with Pillow).
set -e
cd "$(dirname "$0")"
g++ -std=gnu++17 -O1 -Wall -Wextra -o /tmp/cyd_test_ui test_ui.cpp host_gfx.cpp host_hal.cpp \
    ../src/ui.cpp ../src/text.cpp ../src/settings.cpp ../src/clock_core.cpp ../src/fonts_data.cpp
/tmp/cyd_test_ui
# python3 render.py out.png /tmp/ui_01_main.raw /tmp/ui_03_general.raw   # turns the dumps into a picture

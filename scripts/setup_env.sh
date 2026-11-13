#!/usr/bin/env zsh
# From the project root, run: source scripts/setup_env.sh
# Source this file so the exports apply to your current terminal.

export PICO_SDK_PATH="$PWD/.deps/pico-sdk"
export PATH="$PWD/.deps/venv/bin:$PWD/.deps/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi/bin:$PWD/.deps/openocd:$PWD/.deps/picotool/picotool:$PATH"
export OPENOCD_SCRIPTS="$PWD/.deps/openocd/scripts"
rehash

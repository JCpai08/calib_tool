#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "$script_dir/.." && pwd)"

if [[ -n "${QT_BIN_PATH:-}" ]]; then
  export PATH="$QT_BIN_PATH:$PATH"
fi

if [[ -n "${OPENCV_BIN_PATH:-}" ]]; then
  export PATH="$OPENCV_BIN_PATH:$PATH"
fi

if [[ -n "${QT_LIB_PATH:-}" ]]; then
  export LD_LIBRARY_PATH="$QT_LIB_PATH:${LD_LIBRARY_PATH:-}"
fi

if [[ -n "${OPENCV_LIB_PATH:-}" ]]; then
  export LD_LIBRARY_PATH="$OPENCV_LIB_PATH:${LD_LIBRARY_PATH:-}"
fi

exec "$repo_dir/build/calib_tool" "$@"

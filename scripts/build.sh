#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "$script_dir/.." && pwd)"

cmake_args=(
  -B "$repo_dir/build"
  -S "$repo_dir"
)

if [[ -n "${OpenCV_DIR:-}" ]]; then
  cmake_args+=("-DOpenCV_DIR=$OpenCV_DIR")
elif [[ -n "${OPENCV_DIR:-}" ]]; then
  cmake_args+=("-DOpenCV_DIR=$OPENCV_DIR")
fi

if [[ -n "${Qt6_DIR:-}" ]]; then
  cmake_args+=("-DQt6_DIR=$Qt6_DIR")
elif [[ -n "${QT6_DIR:-}" ]]; then
  cmake_args+=("-DQt6_DIR=$QT6_DIR")
fi

cmake "${cmake_args[@]}"
cmake --build "$repo_dir/build" --config Release

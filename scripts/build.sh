#!/usr/bin/env bash
# Build the firmware locally with Docker, the same way as GitHub Actions.
#
# Usage:
#   scripts/build.sh                 Build every target in build.yaml.
#   scripts/build.sh nickey44a_r     Build targets whose name contains nickey44a_r.
#   scripts/build.sh --update        Also refresh branch-based west projects.
#
# The .uf2 files and build logs are written to build/ (ignored by git).
# firmware/ holds the released firmware and is not touched.
#
# The west workspace (ZMK, Zephyr and the modules) lives in the Docker volume
# nickey44a-zmk-ws, so only the first run downloads it and later builds are
# incremental. It is refreshed automatically when config/west.yml changes.
# Delete it with: docker volume rm nickey44a-zmk-ws
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
# ZMK v0.3 is on Zephyr 3.5, the same image as the GitHub Actions workflow.
image="zmkfirmware/zmk-build-arm:3.5"
volume="nickey44a-zmk-ws"

mkdir -p "$repo/build"

docker run --rm \
    -v "$volume:/ws" \
    -v "$repo:/src:ro" \
    -v "$repo/build:/out" \
    "$image" bash /src/scripts/build-in-container.sh "$@"

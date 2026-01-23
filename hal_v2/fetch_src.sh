#!/bin/bash
#
# Downloading Telink BLE sources.
# !!! This script is for Telink internal usage only !!!
# !!! Use ./fetch_sdk instead !!!
#
# Usage: ./fetch_src.sh
# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

REPO_URL="http://192.168.48.36/src/ble/telink_b91m_ble_multi_connection_src.git"
TARGET_COMMIT="d8b779b1ed138c0579b548d3164cb255955e6f09"
DEST_DIR="$SCRIPT_DIR/tl_ble_src"

echo "dest dir: $DEST_DIR"

if [ ! -d "$DEST_DIR/.git" ]; then
    echo "cloning repo..."
    git clone $REPO_URL "$DEST_DIR"
else
    echo "repo already exists, updating..."
fi

cd "$DEST_DIR" || exit

echo "fetching latest data and checking out Commit: $TARGET_COMMIT"
git fetch origin
git checkout $TARGET_COMMIT

CURRENT_COMMIT=$(git rev-parse HEAD)
echo "------------------------------------------"
echo "Sync complete!"
echo "Current directory: $(pwd)"
echo "Current Commit: $CURRENT_COMMIT"

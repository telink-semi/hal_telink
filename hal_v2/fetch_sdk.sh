#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

REPO_URL="http://192.168.48.36/sdk_app/ble/tl_ble_sdk.git"
TARGET_COMMIT="bf886feb6cb78dac40b4fc34c8cc1cec1316bf14"
DEST_DIR="$SCRIPT_DIR/tl_ble_sdk"

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
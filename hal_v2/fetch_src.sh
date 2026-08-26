#!/bin/bash
#
# Downloading Telink BLE sources.
# !!! This script is for Telink internal usage only !!!
# !!! Use ./fetch_sdk instead !!!
#
# Usage: ./fetch_src.sh [repo_url] [target]
# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

# default values
DEFAULT_REPO_URL="http://192.168.48.36/src/ble/telink_b91m_ble_multi_connection_src.git"
DEFAULT_TARGET="59f98c5122291c8eb23e3ea0b3666c42e9f81b8e"

# parse parameters
REPO_URL="${1:-$DEFAULT_REPO_URL}"
TARGET="${2:-$DEFAULT_TARGET}"
DEST_DIR="$SCRIPT_DIR/tl_ble_src"

# show help message
show_help() {
    echo "Usage: $0 [repo_url] [target]"
    echo ""
    echo "Parameters:"
    echo "  repo_url    - Git repository URL (default: $DEFAULT_REPO_URL)"
    echo "  target      - Commit hash, branch name, or tag (default: $DEFAULT_TARGET)"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Use default values"
    echo "  $0 https://github.com/xxx/src.git main  # Clone main branch"
    echo "  $0 https://github.com/xxx/src.git v1.2.3  # Clone tag v1.2.3"
    echo "  $0 https://github.com/xxx/src.git abc123  # Clone specific commit"
    echo ""
}

# check parameters
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
    exit 0
fi

echo "=========================================="
echo "Fetching Telink BLE sources"
echo "=========================================="
echo "Repository: $REPO_URL"
echo "Target: $TARGET"
echo "Destination: $DEST_DIR"
echo "=========================================="

# check if target repo exists
if [ -d "$DEST_DIR/.git" ]; then
    echo "Repository already exists, updating..."
    cd "$DEST_DIR" || exit 1
    
    # acquire latest data
    git fetch --all --tags --prune
    
    # check if target commit exists in current remote
    TARGET_EXISTS=false
    
    if git rev-parse --quiet --verify "$TARGET^{commit}" >/dev/null 2>&1; then
        TARGET_EXISTS=true
    else
        # Try to fetch from other remotes
        echo "Target not found in current remote, trying to add alternative remote..."
        
        # Check if we need to switch to different repository
        CURRENT_REMOTE=$(git remote get-url origin 2>/dev/null)
        if [ "$CURRENT_REMOTE" != "$REPO_URL" ]; then
            echo "Current remote: $CURRENT_REMOTE"
            echo "Target remote: $REPO_URL"
            echo "Switching remote to target repository..."
            
            # Add new remote
            git remote add target_repo "$REPO_URL" 2>/dev/null || git remote set-url target_repo "$REPO_URL"
            
            # Fetch from target remote
            git fetch target_repo --tags --prune
            
            # Check if target exists in target remote
            if git rev-parse --quiet --verify "target_repo/$TARGET^{commit}" >/dev/null 2>&1; then
                TARGET_EXISTS=true
                # Create a local branch tracking the target remote
                git checkout -b "target_$TARGET" "target_repo/$TARGET" 2>/dev/null || git checkout "$TARGET"
            fi
        fi
    fi
    
    CURRENT_COMMIT=$(git rev-parse HEAD)
    
    # try to checkout target as commit, branch, or tag
    if [ "$TARGET_EXISTS" = true ] || git rev-parse --quiet --verify "$TARGET^{commit}" >/dev/null 2>&1; then
        # TARGET is valid commit hash
        if [ "$CURRENT_COMMIT" = "$TARGET" ]; then
            echo "Already at target commit: $TARGET"
            echo "------------------------------------------"
            echo "Sync complete!"
            echo "Current directory: $(pwd)"
            echo "Current Commit: $CURRENT_COMMIT"
            exit 0
        fi
        git checkout "$TARGET"
    elif git show-ref --quiet --verify "refs/heads/$TARGET"; then
        # TARGET is local branch
        if [ "$CURRENT_COMMIT" = "$(git rev-parse "$TARGET")" ]; then
            echo "Already at target branch: $TARGET (commit: $CURRENT_COMMIT)"
            echo "------------------------------------------"
            echo "Sync complete!"
            echo "Current directory: $(pwd)"
            echo "Current Commit: $CURRENT_COMMIT"
            exit 0
        fi
        git checkout "$TARGET"
        git pull origin "$TARGET"
    elif git show-ref --quiet --verify "refs/remotes/origin/$TARGET"; then
        # TARGET is remote branch
        git checkout -b "$TARGET" "origin/$TARGET" 2>/dev/null || git checkout "$TARGET"
        git pull origin "$TARGET"
    else
        echo "Error: Target '$TARGET' not found. Please check the commit hash, branch name, or tag."
        exit 1
    fi
else
    # repository does not exist, clone it
    echo "Cloning repository..."
    git clone "$REPO_URL" "$DEST_DIR"
    cd "$DEST_DIR" || exit 1
    
    # try to checkout target
    if git rev-parse --quiet --verify "$TARGET^{commit}" >/dev/null 2>&1; then
        git checkout "$TARGET"
    elif git show-ref --quiet --verify "refs/heads/$TARGET"; then
        git checkout "$TARGET"
    elif git show-ref --quiet --verify "refs/remotes/origin/$TARGET"; then
        git checkout -b "$TARGET" "origin/$TARGET"
    else
        echo "Error: Target '$TARGET' not found. Please check the commit hash, branch name, or tag."
        exit 1
    fi
fi

CURRENT_COMMIT=$(git rev-parse HEAD)
echo "------------------------------------------"
echo "Sync complete!"
echo "Current directory: $(pwd)"
echo "Current Commit: $CURRENT_COMMIT"

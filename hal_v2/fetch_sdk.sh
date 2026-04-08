#!/bin/bash
#
# Downloading Telink BLE SDK.
#
# Usage: ./fetch_sdk.sh [repo_url] [target]
# Copyright (c) 2026 Telink Semiconductor
# SPDX-License-Identifier: Apache-2.0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

# default values
DEFAULT_REPO_URL="https://github.com/telink-semi/tl_ble_sdk_zephyr.git"
DEFAULT_TARGET="20eb59b1003eec87e547eac663a6e0a97b709472"

# parse parameters
REPO_URL="${1:-$DEFAULT_REPO_URL}"
TARGET="${2:-$DEFAULT_TARGET}"
DEST_DIR="$SCRIPT_DIR/tl_ble_sdk"

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
    echo "  $0 https://github.com/xxx/sdk.git main  # Clone main branch"
    echo "  $0 https://github.com/xxx/sdk.git v1.2.3  # Clone tag v1.2.3"
    echo "  $0 https://github.com/xxx/sdk.git abc123  # Clone specific commit"
    echo ""
}

# check parameters
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
    exit 0
fi

echo "=========================================="
echo "Fetching Telink BLE SDK"
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
            exit 0
        fi
        git checkout "$TARGET"
    elif git show-ref --quiet --verify "refs/heads/$TARGET"; then
        # TARGET is local branch
        echo "Switching to branch: $TARGET"
        git checkout "$TARGET"
        git pull origin "$TARGET"
    elif git ls-remote --exit-code --heads origin "$TARGET" >/dev/null 2>&1; then
        # TARGET is remote branch
        echo "Switching to remote branch: $TARGET"
        git checkout -B "$TARGET" "origin/$TARGET"
    elif git ls-remote --exit-code --tags origin "$TARGET" >/dev/null 2>&1; then
        # TARGET is tag
        echo "Switching to tag: $TARGET"
        git checkout "tags/$TARGET" -b "tag-$TARGET"
    elif git ls-remote --exit-code --heads target_repo "$TARGET" >/dev/null 2>&1; then
        # TARGET is branch in target remote
        echo "Switching to branch from target remote: $TARGET"
        git checkout -B "$TARGET" "target_repo/$TARGET"
    elif git ls-remote --exit-code --tags target_repo "$TARGET" >/dev/null 2>&1; then
        # TARGET is tag in target remote
        echo "Switching to tag from target remote: $TARGET"
        git checkout "tags/$TARGET" -b "tag-$TARGET"
    else
        echo "Error: Cannot find target '$TARGET'"
        echo "Available commits in current repository:"
        git log --oneline -10
        exit 1
    fi
else
    echo "Cloning repository..."
    git clone "$REPO_URL" "$DEST_DIR"
    cd "$DEST_DIR" || exit 1
    
    # switch to specific target
    if git rev-parse --quiet --verify "$TARGET^{commit}" >/dev/null 2>&1; then
        git checkout "$TARGET"
    elif git ls-remote --exit-code origin "$TARGET" >/dev/null 2>&1; then
        git checkout -B "$TARGET" "origin/$TARGET"
    elif git ls-remote --exit-code --tags origin "$TARGET" >/dev/null 2>&1; then
        git checkout "tags/$TARGET" -b "tag-$TARGET"
    else
        echo "Warning: Target '$TARGET' not found, using default branch"
    fi
fi

# show final status
FINAL_COMMIT=$(git rev-parse HEAD)
FINAL_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "detached HEAD")

echo "=========================================="
echo "Sync complete!"
echo "Current directory: $(pwd)"
echo "Current branch: $FINAL_BRANCH"
echo "Current commit: $FINAL_COMMIT"
echo "=========================================="

# show recent commits
echo "Recent commits:"
git log --oneline -3
echo "=========================================="
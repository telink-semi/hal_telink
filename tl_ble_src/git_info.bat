@echo off
echo generate git information file...
for /f "tokens=*" %%a in ('git rev-parse --abbrev-ref HEAD') do set "BRANCH=%%a"
for /f "tokens=*" %%b in ('git log -1 --format^="%%h"') do set "COMMIT_ID=%%b"
for /f "tokens=*" %%c in ('git log -1 --format^="%%ad"') do set "LOG_TIME=%%c"
set "DATE=%date:~0,4%-%date:~5,2%-%date:~8,2% %time:~0,2%:%time:~3,2%:%time:~6,2%"
echo #define GIT_BRANCH "%BRANCH%"> ../../../stack/ble/git_info_stack.h
echo #define GIT_COMMIT_ID "%COMMIT_ID%">> ../../../stack/ble/git_info_stack.h
echo #define GIT_LOG_TIME "%LOG_TIME%">> ../../../stack/ble/git_info_stack.h
echo #define BUILD_TIME "%DATE%">> ../../../stack/ble/git_info_stack.h

@REM ignore this file.
git update-index --assume-unchanged ../../../stack/ble/git_info_stack.h


@echo off
REM Remove all untracked files and directories according to .gitignore
echo Cleaning repository...
git clean -fdx
echo Deleting legacy output directories...
rmdir /s /q tmpclaude-* 2>nul
rmdir /s /q states 2>nul
del /q benchmark_results.* 2>nul
echo Cleaning complete.

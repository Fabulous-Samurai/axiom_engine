# Build Scripts

This directory contains build automation scripts for the AXIOM Engine project.

## Available Scripts

### Windows

- `ninja_build.bat` - Ninja build script for Windows
- `fast_build.ps1` - Fast build PowerShell script
- `setup_other_device_windows.ps1` - One-command setup for standard Windows CPython environments
- `setup_other_device_msys2_ucrt.ps1` - One-command setup for MSYS2 UCRT environments

### Unix/Linux/macOS

- `ninja_build.sh` - Ninja build script for Unix-like systems
- `setup_other_device_unix.sh` - One-command setup for Linux/macOS

### Python Utilities

- `sonar_download.py` - Download SonarQube project file components as JSON
- `download_cicd_logs.py` - Download GitHub Actions CI/CD workflow run logs

## Usage

See individual script files for detailed usage instructions.

### download_cicd_logs.py

Downloads GitHub Actions workflow run logs for a repository.  Requires a GitHub
personal access token with the `actions:read` (or `repo`) scope.

```bash
# List recent workflow runs without downloading
python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN --list

# Download logs for the latest workflow run (saved to ./cicd-logs/<run-id>/)
python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN

# Download logs for a specific run ID
python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN --run-id 123456789

# Filter by workflow file and branch
python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN \
    --workflow ci.yml --branch master

# Save to a custom output directory
python scripts/download_cicd_logs.py --repo owner/repo --token YOUR_TOKEN --out ./logs
```


$ErrorActionPreference = "Stop"

$ideRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $ideRoot
try {
    python -m pip install -r requirements.txt pyinstaller
    python -m PyInstaller `
        --noconfirm `
        --clean `
        --onefile `
        --windowed `
        --name "RoseV IDE" `
        --icon "assets\rosemod-logo.ico" `
        --add-data "assets;assets" `
        --add-data "samples;samples" `
        "rosev_ide.py"
} finally {
    Pop-Location
}

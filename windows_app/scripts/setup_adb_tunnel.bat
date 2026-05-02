@echo off
echo ==================================================
echo   AudioStream: ADB USB Tunnel Setup
echo ==================================================
echo.
echo Starting ADB Server...
adb start-server

echo.
echo Forwarding TCP Port 5000 (Windows) to TCP Port 5000 (Android)...
adb forward tcp:5000 tcp:5000

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] USB Tunnel Established! 
    echo AudioStream Windows can now connect to localhost:5000.
) else (
    echo.
    echo [ERROR] Failed to establish USB tunnel.
    echo Please ensure:
    echo 1. Your Android phone is plugged in via USB.
    echo 2. USB Debugging is enabled in Developer Options.
    echo 3. You have accepted the RSA fingerprint prompt on your phone.
)
echo.
pause

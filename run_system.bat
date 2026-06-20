@echo off
title Industrial Safety System Launcher
echo ==========================================================
echo       LAUNCHING INDUSTRIAL SAFETY INTEGRATED SYSTEM
echo ==========================================================
echo.

:: 1. Start the PlatformIO Serial Device Monitor for the ESP32 Node
echo [INFO] Spinning up ESP32 Hardware Monitor on COM9...
start "ESP32 Sensor Node Monitor" cmd /k "cd /d C:\Users\User\Downloads\AAE6183-Industrial-Safety-System-main\AAE6183-Industrial-Safety-System-main\esp32_sensor_node && C:\Users\User\.platformio\penv\Scripts\platformio.exe device monitor"

:: 2. Start the Backend Dashboard Server
echo [INFO] Launching Local NodeJS Web Server on Port 3000...
start "Dashboard Web Server" cmd /k "cd /d C:\Users\User\Downloads\AAE6183-Industrial-Safety-System-main\AAE6183-Industrial-Safety-System-main\dashboard && node server.js"

:: 3. Start the Edge AI Python Camera Script
echo [INFO] Initializing Edge AI Computer Vision Pipeline...
start "Edge AI Fire Detection Camera" cmd /k "cd /d C:\Users\User\Downloads\AAE6183-Industrial-Safety-System-main\AAE6183-Industrial-Safety-System-main\edge_ai_host && python fire_detection.py"

echo.
echo ==========================================================
echo [SUCCESS] All 3 subsystems have been launched successfully!
echo           Keep these background processing windows open.
echo ==========================================================
pause
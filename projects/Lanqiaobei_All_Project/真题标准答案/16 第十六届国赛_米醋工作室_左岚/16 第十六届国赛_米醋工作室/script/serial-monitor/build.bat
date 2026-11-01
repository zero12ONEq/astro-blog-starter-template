@echo off
chcp 65001 >nul
echo ========================================
echo 蓝桥杯串口监控助手 - 打包脚本
echo ========================================
echo.

echo [1/4] 清理旧的构建文件...
if exist "src-tauri\target\release" (
    rmdir /s /q "src-tauri\target\release\bundle"
    echo 已清理旧文件
) else (
    echo 无需清理
)
echo.

echo [2/4] 开始构建应用...
echo 这可能需要几分钟时间，请耐心等待...
echo.
call npm run tauri build

if %errorlevel% neq 0 (
    echo.
    echo ❌ 构建失败！请检查错误信息
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✅ 构建成功！
echo ========================================
echo.

echo [3/4] 查找生成的文件...
echo.

if exist "src-tauri\target\release\bundle\msi" (
    echo 📦 MSI安装包：
    dir /b "src-tauri\target\release\bundle\msi\*.msi"
    echo    位置: src-tauri\target\release\bundle\msi\
    echo.
)

if exist "src-tauri\target\release\bundle\nsis" (
    echo 📦 NSIS安装包：
    dir /b "src-tauri\target\release\bundle\nsis\*.exe"
    echo    位置: src-tauri\target\release\bundle\nsis\
    echo.
)

if exist "src-tauri\target\release\serial-monitor.exe" (
    echo 📦 独立可执行文件：
    echo    serial-monitor.exe
    echo    位置: src-tauri\target\release\
    echo.
)

echo [4/4] 打开输出目录...
start "" "src-tauri\target\release\bundle"

echo.
echo ========================================
echo 打包完成！文件已自动打开
echo ========================================
pause

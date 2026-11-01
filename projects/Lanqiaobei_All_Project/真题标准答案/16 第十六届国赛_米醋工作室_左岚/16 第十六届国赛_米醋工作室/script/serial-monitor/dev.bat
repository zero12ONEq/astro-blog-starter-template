@echo off
chcp 65001 >nul
echo ========================================
echo 蓝桥杯串口监控助手 - 开发模式
echo ========================================
echo.

REM 检查node_modules是否存在
if not exist "node_modules" (
    echo ⚠️  检测到未安装依赖，正在自动安装...
    echo.
    call npm install
    echo.
    if %errorlevel% neq 0 (
        echo ❌ 依赖安装失败！
        echo.
        echo 请检查：
        echo 1. 是否已安装 Node.js
        echo 2. 网络连接是否正常
        echo 3. 尝试手动运行: npm install
        echo.
        pause
        exit /b 1
    )
    echo ✅ 依赖安装完成！
    echo.
)

echo 🚀 正在启动开发服务器...
echo.
echo 提示：
echo - 首次启动需要编译Rust代码，可能需要5-10分钟
echo - 修改前端代码会自动热重载
echo - 修改后端代码需要手动重启（Ctrl+C 停止，然后重新双击此文件）
echo - 按 Ctrl+C 可以停止服务器
echo.
echo ========================================
echo.

npm run tauri dev

if %errorlevel% neq 0 (
    echo.
    echo ❌ 启动失败！
    echo.
    echo 常见问题：
    echo 1. 端口被占用 - 关闭其他占用1420端口的程序
    echo 2. Rust未安装 - 请安装 Rust 工具链
    echo 3. 编译错误 - 查看上面的错误信息
    echo.
    pause
    exit /b 1
)

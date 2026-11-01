@echo off
chcp 65001 >nul
echo ========================================
echo 源码清理脚本 - 删除所有缓存和编译产物
echo ========================================
echo.
echo ⚠️  警告：此操作将删除以下内容：
echo    - node_modules (npm依赖)
echo    - src-tauri\target (Rust编译缓存，约1GB)
echo    - dist (前端构建输出)
echo    - 各种缓存文件
echo.
echo 删除后需要重新运行 npm install 才能使用
echo.
pause

echo.
echo [1/4] 删除 node_modules...
if exist "node_modules" (
    rmdir /s /q "node_modules"
    echo ✅ 已删除 node_modules
) else (
    echo ℹ️  node_modules 不存在
)

echo.
echo [2/4] 删除 src-tauri\target（这个最大，约1GB）...
if exist "src-tauri\target" (
    rmdir /s /q "src-tauri\target"
    echo ✅ 已删除 src-tauri\target
) else (
    echo ℹ️  src-tauri\target 不存在
)

echo.
echo [3/4] 删除 dist...
if exist "dist" (
    rmdir /s /q "dist"
    echo ✅ 已删除 dist
) else (
    echo ℹ️  dist 不存在
)

echo.
echo [4/4] 删除其他缓存...
if exist ".vite" rmdir /s /q ".vite"
if exist ".tauri" rmdir /s /q ".tauri"
if exist ".cache" rmdir /s /q ".cache"
echo ✅ 已清理其他缓存

echo.
echo ========================================
echo ✅ 清理完成！
echo ========================================
echo.
echo 现在你可以压缩整个文件夹发给别人了
echo.
echo 📝 别人收到后需要执行：
echo    1. npm install              （安装依赖）
echo    2. npm run tauri dev        （开发模式）
echo    或
echo    2. npm run tauri build      （打包模式）
echo.
pause

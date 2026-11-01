# 蓝桥杯串口监控助手 - 打包说明

## 🚀 快速打包

### 方法1：使用批处理脚本（推荐）
双击运行 `build.bat` 文件即可自动完成打包。

### 方法2：手动命令
```bash
npm run tauri build
```

---

## 📦 打包输出

构建完成后，会生成以下文件：

### 1. MSI安装包（推荐分发）
- **位置**：`src-tauri\target\release\bundle\msi\`
- **文件名**：`串口监控助手_1.0.0_x64_zh-CN.msi`
- **用途**：标准Windows安装包，双击即可安装
- **优点**：正规安装流程，支持卸载

### 2. NSIS安装包（备用）
- **位置**：`src-tauri\target\release\bundle\nsis\`
- **文件名**：`串口监控助手_1.0.0_x64-setup.exe`
- **用途**：另一种安装包格式
- **优点**：体积更小，安装更快

### 3. 独立可执行文件
- **位置**：`src-tauri\target\release\serial-monitor.exe`
- **用途**：免安装版本，可直接运行
- **注意**：需要依赖WebView2运行时

---

## ⚙️ 构建要求

### 必需环境
- ✅ Node.js 18+
- ✅ Rust 工具链
- ✅ Visual Studio Build Tools（Windows）

### 首次构建
首次构建会下载并编译依赖，需要较长时间（约10-20分钟）。

---

## 📋 版本信息修改

修改版本号请编辑以下文件：

### 1. `package.json`
```json
{
  "version": "1.0.0"
}
```

### 2. `src-tauri/tauri.conf.json`
```json
{
  "package": {
    "version": "1.0.0"
  }
}
```

### 3. `src-tauri/Cargo.toml`
```toml
[package]
version = "1.0.0"
```

---

## 🎯 分发建议

### 给用户
推荐使用 **MSI安装包**，双击安装即可。

### 给开发者
可以直接使用 **独立exe**，但需要确保系统已安装WebView2。

---

## ❓ 常见问题

### Q1: 打包失败怎么办？
**A**: 确保所有依赖已安装：
```bash
npm install
```

### Q2: 缺少WebView2？
**A**: Windows 10/11通常已预装，如未安装可从以下地址下载：
https://developer.microsoft.com/microsoft-edge/webview2/

### Q3: 如何减小体积？
**A**: Release构建已经优化，Tauri应用体积通常在10-15MB左右。

### Q4: 可以改图标吗？
**A**: 可以，替换 `src-tauri/icons/icon.ico` 文件。

### Q5: 打包需要多久？
**A**:
- 首次构建：10-20分钟（需编译Rust依赖）
- 后续构建：2-5分钟

---

## 🔧 高级配置

### 自定义安装程序
编辑 `src-tauri/tauri.conf.json` 中的 `bundle` 配置。

### 只生成MSI
```json
"targets": ["msi"]
```

### 只生成NSIS
```json
"targets": ["nsis"]
```

---

## 📝 构建日志

构建过程中的日志保存在：
- `src-tauri/target/release/build.log`（如果构建失败）

---

**祝你打包顺利！** 🎉

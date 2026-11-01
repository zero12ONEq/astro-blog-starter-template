# 蓝桥杯串口监控助手

蓝桥杯第十六届单片机设计与开发项目国赛 - PC端上位机软件 by 左岚

---

## 📋 技术栈

- **前端**: React 18 + TypeScript + Ant Design
- **桌面框架**: Tauri 1.5 (Rust)
- **图表**: Apache ECharts
- **状态管理**: Zustand
- **构建工具**: Vite

---

## ✨ 功能特性

- ✅ 串口通信 (波特率9600, 8N1)
- ✅ 实时数据监控 (液位、温度、重量、频率、声速、体积、剩余空间)
- ✅ 参数配置 (8个系统参数 + 查询功能)
- ✅ 异常告警管理 (5种告警类型)
- ✅ 实时曲线图表 (7个数据维度)
- ✅ 称重传感器5点标定
- ✅ JetBrains Darcula深色主题

---

## 🚀 快速开始

### 方式1：使用批处理脚本（推荐）

```
双击 dev.bat      → 启动开发模式（自动安装依赖）
双击 build.bat    → 打包成exe安装包
双击 clean.bat    → 清理缓存（发源码前使用）
```

### 方式2：手动命令

#### 1. 安装依赖
```bash
npm install
```
**预计时间**：2-5分钟（取决于网速）

#### 2. 运行开发模式
```bash
npm run tauri dev
```
**预计时间**：首次编译约5-10分钟（Rust编译较慢）

#### 3. 打包成exe
```bash
npm run tauri build
```
**预计时间**：首次打包约10-20分钟

---

## 📦 打包输出位置

打包成功后，生成的文件在：

```
src-tauri\target\release\bundle\
├── msi\
│   └── 串口监控助手_1.0.0_x64_zh-CN.msi      (约4MB - 推荐)
└── nsis\
    └── 串口监控助手_1.0.0_x64-setup.exe        (约3MB)
```

**推荐分发MSI安装包**，双击即可安装，自动处理WebView2依赖。

---

## 📁 项目结构

```
serial-monitor/
├── src/                      # React前端源码
│   ├── components/          # UI组件
│   │   ├── DataMonitor/    # 实时数据监控
│   │   ├── ParamConfig/    # 参数配置
│   │   ├── ChartView/      # 图表组件
│   │   └── AlarmPanel/     # 告警面板
│   ├── pages/              # 页面
│   │   └── Dashboard/      # 主控制面板
│   ├── stores/             # Zustand状态管理
│   │   ├── serialStore.ts  # 串口状态
│   │   ├── dataStore.ts    # 传感器数据
│   │   ├── configStore.ts  # 参数配置
│   │   └── alarmStore.ts   # 告警管理
│   ├── services/           # Tauri API调用
│   └── types/              # TypeScript类型定义
├── src-tauri/              # Rust后端源码
│   ├── src/
│   │   └── main.rs        # 主程序（串口通信逻辑）
│   └── Cargo.toml         # Rust依赖配置
├── src/user/              # 下位机源码（C51）
│   └── main.c            # 单片机主程序
├── package.json          # npm依赖配置
├── dev.bat              # 开发模式脚本
├── build.bat            # 打包脚本
├── clean.bat            # 清理脚本
└── BUILD.md             # 详细打包说明
```

---

## 📡 串口通信协议

### 实时数据上报（下位机→上位机）
```
L:100,T:25,W:15.00,F:1500,S:343.0,V:0.012,R:50\r\n
```
- L: 液位高度(cm)
- T: 温度(℃)
- W: 重量(t)
- F: 频率(Hz)
- S: 声速(m/s)
- V: 体积(m³)
- R: 剩余空间(%)

### 参数配置（上位机→下位机）
```
(H1,2.0)(H2,0.3)(F,2000)(S,1)(r,1.0)(L,2.0)(W,2.0)(H,3.0)\r\n
```

### 参数查询（上位机→下位机）
```
(H1,?)\r\n
```

### 参数响应（下位机→上位机）
```
(H1,2.0)\r\n
```

### 异常告警（下位机→上位机）
```
[500,080002,2.13]\r\n
```
- 500: 液位超限
- 501: 液位低限
- 502: 频率异常
- 503: 重量异常

---

## ⚙️ 环境要求

### 开发环境
- ✅ **Node.js 18+**
  下载：https://nodejs.org/

- ✅ **Rust 工具链**
  下载：https://www.rust-lang.org/tools/install

- ✅ **Visual Studio Build Tools**（Windows）
  下载：https://visualstudio.microsoft.com/zh-hans/downloads/
  安装时选择"C++桌面开发"工作负载

### 用户运行环境
- Windows 10/11（已预装WebView2）
- 或 Windows 7/8 + WebView2运行时

---

## ❓ 常见问题

### Q1: npm install 失败
**A**: 尝试切换npm镜像源：
```bash
npm config set registry https://registry.npmmirror.com
```

### Q2: Rust编译失败
**A**: 确保已安装Visual Studio Build Tools，并重启电脑

### Q3: 首次编译很慢
**A**: 正常现象，Rust首次编译需要下载和编译所有依赖（10-20分钟）

### Q4: 打包后exe很大
**A**: 正常，Tauri应用包含Rust运行时，通常在5-15MB

### Q5: 端口1420被占用
**A**: 关闭其他Tauri/Vite进程，或重启电脑

### Q6: 图表不显示数据
**A**: 确保：
1. 串口已连接
2. 下位机正在发送数据
3. 数据格式符合协议

---

## 🗑️ 清理缓存

发源码给别人前，运行 `clean.bat` 删除以下内容：

- `node_modules` (200-500MB)
- `src-tauri\target` (1-2GB - 这是最大的缓存)
- `dist` (1-5MB)

清理后文件夹大小：**约5-20MB**

---

## 🔧 开发说明

### 修改前端
- 编辑 `src/` 目录下的文件
- 保存后自动热重载（开发模式下）

### 修改后端（串口通信）
- 编辑 `src-tauri/src/main.rs`
- 保存后需要重新启动（Ctrl+C 停止，然后重新运行 `dev.bat`）

### 修改版本号
同时修改以下文件：
- `package.json` → `"version": "1.0.0"`
- `src-tauri/tauri.conf.json` → `"package": { "version": "1.0.0" }`
- `src-tauri/Cargo.toml` → `version = "1.0.0"`

---

## 📜 许可证

MIT License

---

**祝你使用愉快！** 🎉

# Windows 11 测试指南 🪟

## 快速测试（推荐 - Mock 模式）

无需安装 FFmpeg，直接运行 Electron UI 测试：

```powershell
# 1. 克隆项目
git clone https://github.com/LionelHao/ElectronDemo.git
cd ElectronDemo

# 2. 安装依赖
npm install

# 3. 运行应用（Mock 模式）
npm start
```

**预期结果：**
- ✅ 应用窗口正常打开
- ✅ UI 界面完整显示（时间轴、预览区、属性面板）
- ✅ 播放/暂停按钮工作
- ✅ 时间轴拖拽正常
- ✅ Canvas 显示测试图案
- ✅ FPS 监控正常

---

## 完整测试（需要 FFmpeg）

### 前置条件

#### 1. 安装 Node.js

下载并安装：https://nodejs.org/ (LTS 版本 18+)

验证安装：
```powershell
node --version  # 应显示 v18.x.x 或更高
npm --version   # 应显示 9.x.x 或更高
```

#### 2. 安装 Visual Studio Build Tools

下载：https://visualstudio.microsoft.com/downloads/

**必须勾选的工作负载：**
- ✅ 使用 C++ 的桌面开发
- ✅ Windows 10 SDK (或更新版本)

安装后重启电脑。

#### 3. 安装 FFmpeg 开发库

**方式 A: 使用 vcpkg (推荐)**

```powershell
# 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装 FFmpeg
.\vcpkg install ffmpeg:x64-windows

# 设置环境变量
setx FFMPEG_DIR "C:\path\to\vcpkg\installed\x64-windows"
setx PATH "%FFMPEG_DIR%\bin;%PATH%"
```

**方式 B: 手动下载**

1. 下载开发包：https://github.com/BtbN/FFmpeg-Builds/releases
2. 解压到 `C:\ffmpeg`
3. 添加环境变量：
   - `FFMPEG_DIR = C:\ffmpeg`
   - `PATH = %FFMPEG_DIR%\bin;%PATH%`

#### 4. 配置 npm

```powershell
# 设置 npm 使用 VS 2019/2022
npm config set msvs_version 2022
```

### 编译和运行

```powershell
# 1. 克隆项目
git clone https://github.com/LionelHao/ElectronDemo.git
cd ElectronDemo

# 2. 安装依赖
npm install

# 3. 编译 Native Module
npm run build:native

# 4. 运行应用
npm start
```

---

## 测试清单

### UI 测试
- [ ] 应用窗口正常打开 (1400x900)
- [ ] 标题栏显示 "🎬 Video Editor Demo"
- [ ] 工具栏按钮可见（新建、打开、保存、导出）
- [ ] 左侧资源库面板显示
- [ ] 中间预览区 Canvas 显示
- [ ] 右侧属性面板显示
- [ ] 底部时间轴显示（3 条轨道）
- [ ] 状态栏显示 FPS、分辨率等信息

### 功能测试
- [ ] 播放/暂停按钮工作（Space 快捷键）
- [ ] 跳到开头/结尾按钮工作
- [ ] 前进/后退 5 秒按钮工作
- [ ] 时间轴拖拽改变播放位置
- [ ] 时间显示正确更新
- [ ] 缩放按钮改变时间轴显示
- [ ] 属性滑块可调节

### 性能测试
- [ ] FPS 显示在 55-60 之间（绿色）
- [ ] UI 响应流畅，无明显卡顿
- [ ] 拖拽时间轴时画面更新及时
- [ ] 内存占用稳定

### Native Module 测试（完整模式）
- [ ] Native Module 加载成功
- [ ] 状态栏显示 "Native: 已加载"（绿色）
- [ ] 无控制台错误
- [ ] FFmpeg 解码器初始化成功
- [ ] OpenGL 渲染器初始化成功

---

## 常见问题解决

### 问题 1: npm install 失败

**错误：** `MSBUILD : error MSB3428: Could not load the Visual C++ component "VCBuild.exe"`

**解决：**
```powershell
# 安装 Windows Build Tools
npm install --global --production windows-build-tools

# 或手动指定 VS 版本
npm config set msvs_version 2022
```

### 问题 2: Native Module 编译失败

**错误：** `fatal error LNK1181: cannot open input file 'avcodec.lib'`

**解决：**
```powershell
# 确认 FFmpeg 已安装
echo %FFMPEG_DIR%

# 检查 lib 文件是否存在
dir %FFMPEG_DIR%\lib\avcodec.lib

# 如果不存在，重新安装 FFmpeg
vcpkg install ffmpeg:x64-windows
```

### 问题 3: 应用启动后黑屏

**可能原因：** OpenGL 初始化失败

**解决：**
1. 更新显卡驱动
2. 检查 OpenGL 版本（需要 3.3+）
3. 使用 `npm start` 启动并查看控制台错误

### 问题 4: SharedArrayBuffer 不可用

**错误：** `SharedArrayBuffer is not defined`

**解决：**
确保 Electron 启动参数正确，检查 `main.js` 中是否有：
```javascript
app.commandLine.appendSwitch('enable-features', 'SharedArrayBuffer');
```

---

## 开发者工具

### 查看日志

应用启动后按 `F12` 打开开发者工具，查看控制台日志：

```
[Main] App ready
[Main] Native module loaded successfully
[Renderer] Initializing Video Editor...
[Renderer] Native module available, buffer size: 8294400
[Renderer] Initialization complete
```

### 性能分析

在开发者工具中：
1. 切换到 Performance 标签
2. 点击录制
3. 操作应用（播放、拖拽等）
4. 停止录制并分析

### 调试 Native Module

```powershell
# 启用详细日志
$env:NODE_ENV = "development"
npm start
```

---

## 测试结果反馈

完成测试后，请反馈以下信息：

```
测试环境:
- Windows 版本：Windows 11 64-bit
- Node.js 版本：v18.x.x
- npm 版本：9.x.x
- 显卡型号：NVIDIA/AMD/Intel
- 测试模式：Mock / 完整

测试结果:
- UI 测试：✅ / ❌
- 功能测试：✅ / ❌
- 性能测试：✅ / ❌
- Native Module: ✅ / ❌

问题描述:
[如有问题，详细描述]

截图/日志:
[如有问题，附上截图或日志]
```

---

## 联系

如有问题，请提交 Issue 或联系开发者。

**GitHub:** https://github.com/LionelHao/ElectronDemo

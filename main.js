/**
 * Electron Main Process
 * 负责：系统资源管理、文件监听、IPC 路由、窗口创建
 */

const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');

// 启用 SharedArrayBuffer 支持（需要 crossOriginIsolated）
app.commandLine.appendSwitch('enable-features', 'SharedArrayBuffer');

let mainWindow;
let nativeModule = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
      // 启用 SharedArrayBuffer 需要这些设置
      sandbox: false
    },
    backgroundColor: '#1e1e1e',
    show: false, // 先隐藏，加载完成后显示
    frame: true,
    titleBarStyle: 'default'
  });

  mainWindow.loadFile('index.html');

  // 页面加载完成后显示窗口
  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    console.log('[Main] Window ready');
  });

  // 开发工具（生产环境可禁用）
  if (process.env.NODE_ENV === 'development') {
    mainWindow.webContents.openDevTools();
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
    if (nativeModule) {
      nativeModule.cleanup();
      nativeModule = null;
    }
  });
}

// 初始化 Native Module
function initNativeModule() {
  try {
    // 加载 C++ Native Module
    nativeModule = require('./build/Release/video_engine.node');
    console.log('[Main] Native module loaded successfully');
    
    // 初始化模块
    if (nativeModule.init) {
      nativeModule.init();
    }
  } catch (err) {
    console.warn('[Main] Native module not available (development mode):', err.message);
    console.log('[Main] Running in mock mode');
    nativeModule = createMockModule();
  }
}

// 创建 Mock 模块用于测试
function createMockModule() {
  const sharedBuffer = new SharedArrayBuffer(1920 * 1080 * 4);
  const sharedArray = new Uint8ClampedArray(sharedBuffer);
  
  return {
    init: () => console.log('[Mock] Native module initialized'),
    cleanup: () => console.log('[Mock] Native module cleaned up'),
    requestFrame: (timestamp) => {
      // Mock: 生成测试帧数据（渐变蓝色）
      for (let i = 0; i < sharedArray.length; i += 4) {
        sharedArray[i] = (i / 4) % 256;     // R
        sharedArray[i + 1] = 100;            // G
        sharedArray[i + 2] = 200;            // B
        sharedArray[i + 3] = 255;            // A
      }
      return {
        width: 1920,
        height: 1080,
        timestamp: timestamp
      };
    },
    getSharedBuffer: () => sharedBuffer,
    setTimeline: (timeline) => console.log('[Mock] Timeline set:', timeline),
    loadMedia: (path) => console.log('[Mock] Media loaded:', path)
  };
}

// IPC 处理
function setupIPC() {
  // 请求渲染帧
  ipcMain.handle('request-frame', async (event, timestamp) => {
    if (!nativeModule) {
      return { error: 'Native module not initialized' };
    }
    
    const startTime = Date.now();
    const result = nativeModule.requestFrame(timestamp);
    const endTime = Date.now();
    
    console.log(`[Main] Frame rendered at ${timestamp}s, took ${endTime - startTime}ms`);
    return result;
  });

  // 设置时间轴
  ipcMain.handle('set-timeline', async (event, timeline) => {
    if (nativeModule && nativeModule.setTimeline) {
      nativeModule.setTimeline(timeline);
    }
    return { success: true };
  });

  // 加载媒体文件
  ipcMain.handle('load-media', async (event, filePath) => {
    if (nativeModule && nativeModule.loadMedia) {
      const result = nativeModule.loadMedia(filePath);
      return result;
    }
    return { success: false, error: 'Native module not available' };
  });

  // 获取共享内存信息
  ipcMain.handle('get-shared-buffer-info', async () => {
    if (nativeModule && nativeModule.getSharedBuffer) {
      const buffer = nativeModule.getSharedBuffer();
      return {
        byteLength: buffer.byteLength,
        available: true
      };
    }
    return { available: false };
  });

  // 播放控制
  ipcMain.handle('play', async () => {
    console.log('[Main] Play requested');
    return { success: true };
  });

  ipcMain.handle('pause', async () => {
    console.log('[Main] Pause requested');
    return { success: true };
  });

  ipcMain.handle('seek', async (event, timestamp) => {
    console.log(`[Main] Seek to ${timestamp}s`);
    return { success: true, timestamp };
  });
}

// 应用生命周期
app.whenReady().then(() => {
  console.log('[Main] App ready');
  initNativeModule();
  setupIPC();
  createWindow();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (mainWindow === null) {
    createWindow();
  }
});

// 安全退出
app.on('before-quit', () => {
  console.log('[Main] Quitting...');
  if (nativeModule && nativeModule.cleanup) {
    nativeModule.cleanup();
  }
});

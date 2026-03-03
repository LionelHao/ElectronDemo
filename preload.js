/**
 * Preload Script
 * 安全地暴露 IPC API 给渲染进程
 */

const { contextBridge, ipcRenderer } = require('electron');

// 暴露安全的 API 到渲染进程
contextBridge.exposeInMainWorld('videoEditor', {
  // 请求渲染帧
  requestFrame: (timestamp) => ipcRenderer.invoke('request-frame', timestamp),
  
  // 设置时间轴
  setTimeline: (timeline) => ipcRenderer.invoke('set-timeline', timeline),
  
  // 加载媒体文件
  loadMedia: (filePath) => ipcRenderer.invoke('load-media', filePath),
  
  // 获取共享内存信息
  getSharedBufferInfo: () => ipcRenderer.invoke('get-shared-buffer-info'),
  
  // 播放控制
  play: () => ipcRenderer.invoke('play'),
  pause: () => ipcRenderer.invoke('pause'),
  seek: (timestamp) => ipcRenderer.invoke('seek', timestamp),
  
  // 版本信息
  getVersion: () => '1.0.0'
});

// 日志
console.log('[Preload] VideoEditor API exposed');

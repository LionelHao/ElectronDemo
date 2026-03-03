/**
 * Renderer Process - Video Editor Demo
 * 负责：UI 交互、时间轴可视化、Canvas 渲染、播放控制
 */

class VideoEditor {
  constructor() {
    // 播放状态
    this.isPlaying = false;
    this.currentTime = 0; // 秒
    this.duration = 30; // 默认 30 秒
    this.fps = 60;
    
    // 时间轴
    this.zoom = 1;
    this.tracks = [];
    
    // Canvas
    this.canvas = document.getElementById('preview-canvas');
    this.ctx = this.canvas.getContext('2d');
    
    // 共享内存（如果可用）
    this.sharedBuffer = null;
    this.sharedArray = null;
    
    // 性能监控
    this.frameCount = 0;
    this.lastFpsUpdate = Date.now();
    this.currentFps = 0;
    
    // 动画帧
    this.animationId = null;
    this.lastFrameTime = 0;
    
    // 初始化
    this.init();
  }

  async init() {
    console.log('[Renderer] Initializing Video Editor...');
    
    // 绑定事件
    this.bindEvents();
    
    // 检查 Native Module 状态
    await this.checkNativeModule();
    
    // 初始化时间轴
    this.initTimeline();
    
    // 渲染初始帧
    this.renderFrame(0);
    
    // 启动性能监控
    this.startFpsMonitor();
    
    console.log('[Renderer] Initialization complete');
    this.updateStatus('就绪');
  }

  bindEvents() {
    // 播放控制
    document.getElementById('btn-play').addEventListener('click', () => this.togglePlay());
    document.getElementById('btn-first').addEventListener('click', () => this.seek(0));
    document.getElementById('btn-prev').addEventListener('click', () => this.seek(this.currentTime - 5));
    document.getElementById('btn-next').addEventListener('click', () => this.seek(this.currentTime + 5));
    document.getElementById('btn-last').addEventListener('click', () => this.seek(this.duration));
    
    // 时间轴缩放
    document.getElementById('btn-zoom-in').addEventListener('click', () => this.setZoom(this.zoom + 0.25));
    document.getElementById('btn-zoom-out').addEventListener('click', () => this.setZoom(Math.max(0.25, this.zoom - 0.25)));
    
    // 工具栏
    document.getElementById('btn-import').addEventListener('click', () => this.importMedia());
    document.getElementById('btn-new').addEventListener('click', () => this.newProject());
    document.getElementById('btn-open').addEventListener('click', () => this.openProject());
    document.getElementById('btn-save').addEventListener('click', () => this.saveProject());
    document.getElementById('btn-export').addEventListener('click', () => this.exportVideo());
    
    // 属性面板
    document.getElementById('prop-scale').addEventListener('input', (e) => {
      document.getElementById('scale-value').textContent = e.target.value + '%';
    });
    document.getElementById('prop-opacity').addEventListener('input', (e) => {
      document.getElementById('opacity-value').textContent = e.target.value + '%';
    });
    
    // 时间轴拖拽
    this.initTimelineDrag();
    
    // 键盘快捷键
    document.addEventListener('keydown', (e) => this.handleKeyboard(e));
    
    // 拖放支持
    this.initDragDrop();
  }

  async checkNativeModule() {
    try {
      const info = await window.videoEditor.getSharedBufferInfo();
      if (info.available) {
        console.log('[Renderer] Native module available, buffer size:', info.byteLength);
        document.getElementById('status-native').textContent = 'Native: 已加载';
        document.getElementById('status-native').style.color = '#50c878';
        
        // 初始化共享内存
        this.sharedBuffer = new SharedArrayBuffer(info.byteLength);
        this.sharedArray = new Uint8ClampedArray(this.sharedBuffer);
      } else {
        console.log('[Renderer] Running in mock mode');
        document.getElementById('status-native').textContent = 'Native: Mock 模式';
        document.getElementById('status-native').style.color = '#ffa500';
      }
    } catch (err) {
      console.warn('[Renderer] Native module check failed:', err);
      document.getElementById('status-native').textContent = 'Native: 未加载';
    }
  }

  initTimeline() {
    // 初始化时间轴数据
    this.tracks = [
      { id: 'video-1', type: 'video', name: '视频 1', clips: [] },
      { id: 'audio-1', type: 'audio', name: '音频 1', clips: [] },
      { id: 'text-1', type: 'text', name: '文字', clips: [] }
    ];
    
    // 更新时间显示
    this.updateTimeDisplay();
  }

  initTimelineDrag() {
    const scrubber = document.getElementById('timeline-scrubber');
    const timelineBody = document.querySelector('.timeline-body');
    let isDragging = false;

    scrubber.addEventListener('mousedown', (e) => {
      isDragging = true;
      scrubber.style.cursor = 'grabbing';
    });

    document.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      
      const rect = timelineBody.getBoundingClientRect();
      const x = e.clientX - rect.left - 120; // 减去轨道头宽度
      const percentage = Math.max(0, Math.min(1, x / (rect.width - 120)));
      
      this.seek(percentage * this.duration);
    });

    document.addEventListener('mouseup', () => {
      isDragging = false;
      scrubber.style.cursor = 'grab';
    });
  }

  initDragDrop() {
    const dropZones = document.querySelectorAll('.track-content, .media-list');
    
    dropZones.forEach(zone => {
      zone.addEventListener('dragover', (e) => {
        e.preventDefault();
        zone.classList.add('drag-over');
      });
      
      zone.addEventListener('dragleave', () => {
        zone.classList.remove('drag-over');
      });
      
      zone.addEventListener('drop', (e) => {
        e.preventDefault();
        zone.classList.remove('drag-over');
        
        const files = e.dataTransfer.files;
        if (files.length > 0) {
          this.handleFiles(files, zone);
        }
      });
    });
  }

  handleFiles(files, zone) {
    Array.from(files).forEach(file => {
      console.log('[Renderer] File dropped:', file.name, file.type);
      
      // 创建媒体项
      const mediaItem = {
        id: Date.now() + Math.random(),
        name: file.name,
        type: file.type.startsWith('video') ? 'video' : 
              file.type.startsWith('audio') ? 'audio' : 'image',
        file: file,
        duration: 10 // 默认 10 秒
      };
      
      // 添加到对应轨道
      this.addClipToTrack(mediaItem, zone);
    });
  }

  addClipToTrack(clip, zone) {
    const trackId = zone.closest('.track')?.id || 'track-video-1';
    const track = this.tracks.find(t => t.id === trackId.replace('track-', ''));
    
    if (track) {
      track.clips.push({
        ...clip,
        start: this.currentTime,
        end: this.currentTime + clip.duration
      });
      
      // 更新 UI
      this.renderTrackClips(track);
      this.updateStatus(`已添加：${clip.name}`);
    }
  }

  renderTrackClips(track) {
    const trackEl = document.getElementById(`track-${track.id}`);
    if (!trackEl) return;
    
    trackEl.innerHTML = '';
    
    track.clips.forEach(clip => {
      const clipEl = document.createElement('div');
      clipEl.className = 'track-clip';
      clipEl.style.cssText = `
        position: absolute;
        left: ${(clip.start / this.duration) * 100}%;
        width: ${((clip.end - clip.start) / this.duration) * 100}%;
        height: calc(100% - 8px);
        top: 4px;
        background: ${this.getTrackColor(track.type)};
        border-radius: 4px;
        padding: 4px 8px;
        font-size: 11px;
        overflow: hidden;
        white-space: nowrap;
        text-overflow: ellipsis;
        cursor: pointer;
      `;
      clipEl.textContent = clip.name;
      clipEl.addEventListener('click', () => this.selectClip(clip));
      trackEl.appendChild(clipEl);
    });
  }

  getTrackColor(type) {
    const colors = {
      video: 'rgba(74, 158, 255, 0.6)',
      audio: 'rgba(80, 200, 120, 0.6)',
      text: 'rgba(255, 165, 0, 0.6)'
    };
    return colors[type] || colors.video;
  }

  selectClip(clip) {
    document.getElementById('selected-info').textContent = clip.name;
    console.log('[Renderer] Selected clip:', clip);
  }

  togglePlay() {
    this.isPlaying = !this.isPlaying;
    const btn = document.getElementById('btn-play');
    btn.textContent = this.isPlaying ? '⏸' : '▶';
    
    if (this.isPlaying) {
      this.updateStatus('播放中');
      this.startPlayback();
    } else {
      this.updateStatus('已暂停');
      this.stopPlayback();
    }
  }

  startPlayback() {
    const frameInterval = 1000 / this.fps;
    
    const step = (timestamp) => {
      if (!this.isPlaying) return;
      
      const elapsed = timestamp - this.lastFrameTime;
      
      if (elapsed >= frameInterval) {
        this.currentTime += elapsed / 1000;
        
        if (this.currentTime >= this.duration) {
          this.currentTime = 0;
          this.togglePlay();
          return;
        }
        
        this.renderFrame(this.currentTime);
        this.updateTimeDisplay();
        this.lastFrameTime = timestamp;
      }
      
      this.animationId = requestAnimationFrame(step);
    };
    
    this.lastFrameTime = performance.now();
    this.animationId = requestAnimationFrame(step);
  }

  stopPlayback() {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
      this.animationId = null;
    }
  }

  seek(time) {
    this.currentTime = Math.max(0, Math.min(this.duration, time));
    this.renderFrame(this.currentTime);
    this.updateTimeDisplay();
  }

  setZoom(zoom) {
    this.zoom = zoom;
    document.getElementById('timeline-zoom').textContent = Math.round(zoom * 100) + '%';
  }

  async renderFrame(timestamp) {
    try {
      // 请求 C++ 层渲染帧
      const result = await window.videoEditor.requestFrame(timestamp);
      
      if (result && this.sharedArray) {
        // 从共享内存读取帧数据
        const imageData = new ImageData(this.sharedArray, 1920, 1080);
        this.ctx.putImageData(imageData, 0, 0);
      } else {
        // Mock 渲染：绘制测试图案
        this.renderMockFrame(timestamp);
      }
      
      this.frameCount++;
    } catch (err) {
      console.error('[Renderer] Frame render error:', err);
      this.renderMockFrame(timestamp);
    }
  }

  renderMockFrame(timestamp) {
    // 清空画布
    this.ctx.fillStyle = '#000';
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
    
    // 绘制渐变背景
    const gradient = this.ctx.createLinearGradient(0, 0, this.canvas.width, this.canvas.height);
    gradient.addColorStop(0, '#1a1a2e');
    gradient.addColorStop(1, '#16213e');
    this.ctx.fillStyle = gradient;
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
    
    // 绘制时间戳
    this.ctx.fillStyle = '#fff';
    this.ctx.font = '48px Arial';
    this.ctx.textAlign = 'center';
    this.ctx.fillText(`Frame: ${timestamp.toFixed(3)}s`, this.canvas.width / 2, this.canvas.height / 2);
    
    // 绘制播放状态
    this.ctx.font = '24px Arial';
    this.ctx.fillStyle = this.isPlaying ? '#50c878' : '#ffa500';
    this.ctx.fillText(this.isPlaying ? '▶ PLAYING' : '⏸ PAUSED', this.canvas.width / 2, this.canvas.height / 2 + 50);
    
    // 绘制网格
    this.ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    this.ctx.lineWidth = 1;
    const gridSize = 100;
    for (let x = 0; x < this.canvas.width; x += gridSize) {
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, this.canvas.height);
      this.ctx.stroke();
    }
    for (let y = 0; y < this.canvas.height; y += gridSize) {
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(this.canvas.width, y);
      this.ctx.stroke();
    }
  }

  updateTimeDisplay() {
    const formatTime = (seconds) => {
      const h = Math.floor(seconds / 3600);
      const m = Math.floor((seconds % 3600) / 60);
      const s = Math.floor(seconds % 60);
      const ms = Math.floor((seconds % 1) * 100);
      
      if (h > 0) {
        return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
      }
      return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}:${ms.toString().padStart(2, '0')}`;
    };
    
    const timeStr = formatTime(this.currentTime);
    document.getElementById('preview-time').textContent = timeStr;
    document.getElementById('timeline-current').textContent = timeStr;
    
    // 更新进度条位置
    const scrubber = document.getElementById('timeline-scrubber');
    const percentage = (this.currentTime / this.duration) * 100;
    scrubber.style.left = `calc(120px + ${percentage}%)`;
  }

  startFpsMonitor() {
    setInterval(() => {
      const now = Date.now();
      const elapsed = now - this.lastFpsUpdate;
      this.currentFps = Math.round((this.frameCount * 1000) / elapsed);
      
      document.getElementById('status-fps').textContent = `FPS: ${this.currentFps}`;
      
      // 颜色指示
      const fpsEl = document.getElementById('status-fps');
      if (this.currentFps >= 55) {
        fpsEl.style.color = '#50c878';
      } else if (this.currentFps >= 30) {
        fpsEl.style.color = '#ffa500';
      } else {
        fpsEl.style.color = '#ff4444';
      }
      
      this.frameCount = 0;
      this.lastFpsUpdate = now;
    }, 1000);
  }

  updateStatus(message) {
    document.getElementById('playback-status').textContent = message;
    console.log('[Renderer] Status:', message);
  }

  handleKeyboard(e) {
    switch(e.code) {
      case 'Space':
        e.preventDefault();
        this.togglePlay();
        break;
      case 'Home':
        this.seek(0);
        break;
      case 'End':
        this.seek(this.duration);
        break;
      case 'ArrowLeft':
        this.seek(this.currentTime - 1);
        break;
      case 'ArrowRight':
        this.seek(this.currentTime + 1);
        break;
    }
  }

  importMedia() {
    console.log('[Renderer] Import media clicked');
    this.updateStatus('请选择媒体文件');
  }

  newProject() {
    console.log('[Renderer] New project');
    this.currentTime = 0;
    this.tracks.forEach(t => t.clips = []);
    this.renderFrame(0);
    this.updateStatus('新建项目');
  }

  openProject() {
    console.log('[Renderer] Open project');
    this.updateStatus('打开项目（功能开发中）');
  }

  saveProject() {
    console.log('[Renderer] Save project');
    this.updateStatus('保存项目（功能开发中）');
  }

  exportVideo() {
    console.log('[Renderer] Export video');
    this.updateStatus('导出视频（功能开发中）');
  }
}

// 启动应用
const app = new VideoEditor();

// 暴露给全局用于调试
window.videoEditorApp = app;

#ifndef VIDEO_ENGINE_H
#define VIDEO_ENGINE_H

#include <napi.h>
#include <memory>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>

// 前向声明
class FFmpegDecoder;
class OpenGLRenderer;
class TimelineScheduler;

/**
 * VideoEngine - C++ Native Module 主类
 * 
 * 职责:
 * - 管理 FFmpeg 解码器池
 * - 管理 OpenGL 渲染上下文
 * - 时间轴调度
 * - 共享内存管理
 */
class VideoEngine : public Napi::ObjectWrap<VideoEngine> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  VideoEngine(const Napi::CallbackInfo& info);
  ~VideoEngine();

private:
  // N-API 方法
  Napi::Value Init(const Napi::CallbackInfo& info);
  Napi::Value Cleanup(const Napi::CallbackInfo& info);
  Napi::Value RequestFrame(const Napi::CallbackInfo& info);
  Napi::Value SetTimeline(const Napi::CallbackInfo& info);
  Napi::Value LoadMedia(const Napi::CallbackInfo& info);
  Napi::Value GetSharedBuffer(const Napi::CallbackInfo& info);

  // 内部方法
  bool InitializeDecoder();
  bool InitializeRenderer();
  uint8_t* AllocateSharedBuffer(size_t width, size_t height);
  void RenderFrame(double timestamp);

  // 成员变量
  Napi::ThreadSafeFunction m_notification;
  std::unique_ptr<FFmpegDecoder> m_decoder;
  std::unique_ptr<OpenGLRenderer> m_renderer;
  std::unique_ptr<TimelineScheduler> m_scheduler;
  
  // 共享内存
  uint8_t* m_sharedBuffer;
  size_t m_bufferSize;
  size_t m_width;
  size_t m_height;
  
  // 状态
  std::atomic<bool> m_initialized;
  std::mutex m_mutex;
  
  // 时间轴数据
  Napi::Reference<Napi::Object> m_timeline;
};

#endif // VIDEO_ENGINE_H

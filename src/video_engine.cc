/**
 * Video Engine - C++ Native Module
 * 
 * 基于 N-API 实现的视频处理引擎
 * 集成 FFmpeg 解码和 OpenGL 渲染
 */

#include "video_engine.h"
#include "ffmpeg_decoder.h"
#include "opengl_renderer.h"
#include "timeline_scheduler.h"

#include <napi.h>
#include <cstring>
#include <chrono>

Napi::FunctionReference VideoEngine::constructor;

Napi::Object VideoEngine::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "VideoEngine", {
    InstanceMethod("init", &VideoEngine::Init),
    InstanceMethod("cleanup", &VideoEngine::Cleanup),
    InstanceMethod("requestFrame", &VideoEngine::RequestFrame),
    InstanceMethod("setTimeline", &VideoEngine::SetTimeline),
    InstanceMethod("loadMedia", &VideoEngine::LoadMedia),
    InstanceMethod("getSharedBuffer", &VideoEngine::GetSharedBuffer)
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("VideoEngine", func);
  return exports;
}

VideoEngine::VideoEngine(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<VideoEngine>(info)
  , m_sharedBuffer(nullptr)
  , m_bufferSize(0)
  , m_width(1920)
  , m_height(1080)
  , m_initialized(false) {
  
  Napi::Env env = info.Env();
  // 构造函数可以接收配置参数
}

VideoEngine::~VideoEngine() {
  if (m_initialized) {
    Cleanup(Napi::Env());
  }
  
  if (m_sharedBuffer) {
    delete[] m_sharedBuffer;
    m_sharedBuffer = nullptr;
  }
}

Napi::Value VideoEngine::Init(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_initialized) {
    return Napi::Boolean::New(env, true);
  }
  
  try {
    // 初始化 FFmpeg 解码器
    if (!InitializeDecoder()) {
      Napi::Error::New(env, "Failed to initialize FFmpeg decoder").ThrowAsJavaScriptException();
      return Napi::Boolean::New(env, false);
    }
    
    // 初始化 OpenGL 渲染器
    if (!InitializeRenderer()) {
      Napi::Error::New(env, "Failed to initialize OpenGL renderer").ThrowAsJavaScriptException();
      return Napi::Boolean::New(env, false);
    }
    
    // 初始化时间轴调度器
    m_scheduler = std::make_unique<TimelineScheduler>();
    
    // 分配共享内存 (1920x1080 RGBA = ~8MB)
    m_sharedBuffer = AllocateSharedBuffer(m_width, m_height);
    if (!m_sharedBuffer) {
      Napi::Error::New(env, "Failed to allocate shared buffer").ThrowAsJavaScriptException();
      return Napi::Boolean::New(env, false);
    }
    
    m_initialized = true;
    
    printf("[VideoEngine] Initialized successfully\n");
    printf("[VideoEngine] Buffer size: %zu bytes (%zux%zu RGBA)\n", 
           m_bufferSize, m_width, m_height);
    
    return Napi::Boolean::New(env, true);
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Initialization error: ") + e.what())
      .ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
}

Napi::Value VideoEngine::Cleanup(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (!m_initialized) {
    return Napi::Boolean::New(env, true);
  }
  
  try {
    m_decoder.reset();
    m_renderer.reset();
    m_scheduler.reset();
    
    m_initialized = false;
    
    printf("[VideoEngine] Cleanup complete\n");
    
    return Napi::Boolean::New(env, true);
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Cleanup error: ") + e.what())
      .ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
}

Napi::Value VideoEngine::RequestFrame(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!m_initialized) {
    Napi::Error::New(env, "Engine not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Expected timestamp (number)").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  double timestamp = info[0].As<Napi::Number>().DoubleValue();
  
  try {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 渲染帧到共享内存
    RenderFrame(timestamp);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    printf("[VideoEngine] Frame rendered at %.3fs in %lldms\n", timestamp, duration.count());
    
    // 返回帧信息
    Napi::Object result = Napi::Object::New(env);
    result.Set("width", m_width);
    result.Set("height", m_height);
    result.Set("timestamp", timestamp);
    result.Set("renderTime", duration.count());
    
    return result;
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Render error: ") + e.what())
      .ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value VideoEngine::SetTimeline(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::TypeError::New(env, "Expected timeline object").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  
  try {
    m_timeline = Napi::Persistent(info[0].As<Napi::Object>());
    
    if (m_scheduler) {
      m_scheduler->SetTimeline(info[0].As<Napi::Object>());
    }
    
    printf("[VideoEngine] Timeline updated\n");
    
    return Napi::Boolean::New(env, true);
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("SetTimeline error: ") + e.what())
      .ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
}

Napi::Value VideoEngine::LoadMedia(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Expected file path (string)").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  
  try {
    std::string filePath = info[0].As<Napi::String>().Utf8Value();
    
    if (m_decoder) {
      bool success = m_decoder->LoadMedia(filePath);
      printf("[VideoEngine] Media loaded: %s (%s)\n", 
             filePath.c_str(), success ? "success" : "failed");
      return Napi::Boolean::New(env, success);
    }
    
    return Napi::Boolean::New(env, false);
    
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("LoadMedia error: ") + e.what())
      .ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
}

Napi::Value VideoEngine::GetSharedBuffer(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!m_sharedBuffer) {
    return env.Null();
  }
  
  // 创建 ArrayBuffer 包装共享内存
  // 注意：实际生产中需要使用 External 或 SharedArrayBuffer
  Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, m_bufferSize);
  memcpy(buffer.Data(), m_sharedBuffer, m_bufferSize);
  
  return buffer;
}

bool VideoEngine::InitializeDecoder() {
  try {
    m_decoder = std::make_unique<FFmpegDecoder>();
    return m_decoder->Initialize();
  } catch (...) {
    return false;
  }
}

bool VideoEngine::InitializeRenderer() {
  try {
    m_renderer = std::make_unique<OpenGLRenderer>(m_width, m_height);
    return m_renderer->Initialize();
  } catch (...) {
    return false;
  }
}

uint8_t* VideoEngine::AllocateSharedBuffer(size_t width, size_t height) {
  m_width = width;
  m_height = height;
  m_bufferSize = width * height * 4; // RGBA
  
  try {
    m_sharedBuffer = new uint8_t[m_bufferSize];
    std::memset(m_sharedBuffer, 0, m_bufferSize);
    return m_sharedBuffer;
  } catch (...) {
    return nullptr;
  }
}

void VideoEngine::RenderFrame(double timestamp) {
  if (!m_sharedBuffer || !m_renderer) {
    return;
  }
  
  // 1. 从调度器获取活跃图层
  auto activeLayers = m_scheduler ? m_scheduler->GetActiveLayers(timestamp) 
                                   : std::vector<LayerInfo>();
  
  // 2. 解码所需帧
  if (m_decoder && !activeLayers.empty()) {
    for (const auto& layer : activeLayers) {
      if (layer.type == LayerType::VIDEO) {
        auto frame = m_decoder->DecodeFrame(layer.mediaId, timestamp - layer.startTime);
        if (frame) {
          // 上传到 GPU 纹理
          m_renderer->UploadTexture(layer.id, frame);
        }
      }
    }
  }
  
  // 3. 合成到 FBO
  m_renderer->BeginFrame();
  
  for (const auto& layer : activeLayers) {
    m_renderer->DrawLayer(layer);
  }
  
  // 4. 读取到共享内存
  m_renderer->ReadPixels(m_sharedBuffer, m_width, m_height);
  m_renderer->EndFrame();
}

// 模块初始化
Napi::Object InitModule(Napi::Env env, Napi::Object exports) {
  VideoEngine::Init(env, exports);
  
  // 导出辅助函数
  exports.Set("getVersion", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), "1.0.0");
  }));
  
  return exports;
}

NODE_API_MODULE(video_engine, InitModule)

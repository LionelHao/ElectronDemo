#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

// OpenGL 头文件 (跨平台)
#ifdef _WIN32
  #include <windows.h>
  #include <GL/gl.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
  #include <GL/glx.h>
#endif

#include "timeline_scheduler.h"

/**
 * 纹理信息
 */
struct TextureInfo {
  GLuint textureId;
  int width;
  int height;
  bool valid;
  
  TextureInfo() : textureId(0), width(0), height(0), valid(false) {}
};

/**
 * OpenGLRenderer - OpenGL 渲染引擎
 * 
 * 功能:
 * - FBO (帧缓冲对象) 管理
 * - 图层合成
 * - Shader 特效处理
 * - 像素读取
 */
class OpenGLRenderer {
public:
  OpenGLRenderer(size_t width, size_t height);
  ~OpenGLRenderer();
  
  bool Initialize();
  void Cleanup();
  
  // 帧管理
  void BeginFrame();
  void EndFrame();
  
  // 纹理管理
  bool UploadTexture(int layerId, const std::shared_ptr<DecodedFrame>& frame);
  void DeleteTexture(int layerId);
  
  // 渲染
  void DrawLayer(const LayerInfo& layer);
  void ReadPixels(uint8_t* buffer, size_t width, size_t height);
  
  // 设置
  void SetViewport(size_t width, size_t height);
  void EnableBlend(bool enable);
  void SetBlendMode(int mode);
  
private:
  bool CreateFBO();
  bool CreateShaders();
  void BindFBO();
  void UnbindFBO();
  
  // 着色器编译
  GLuint CompileShader(GLenum type, const char* source);
  GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader);
  
  // 成员变量
  size_t m_width;
  size_t m_height;
  
  // FBO
  GLuint m_fbo;
  GLuint m_fboTexture;
  GLuint m_depthBuffer;
  
  // 着色器
  GLuint m_shaderProgram;
  GLuint m_vertexShader;
  GLuint m_fragmentShader;
  
  // 纹理缓存
  std::unordered_map<int, TextureInfo> m_textures;
  
  // 状态
  bool m_initialized;
  std::mutex m_mutex;
  
  // 混合模式
  bool m_blendEnabled;
  int m_blendMode;
};

#endif // OPENGL_RENDERER_H

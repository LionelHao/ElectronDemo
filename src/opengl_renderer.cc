/**
 * OpenGL Renderer Implementation
 */

#include "opengl_renderer.h"
#include <iostream>
#include <cstring>

OpenGLRenderer::OpenGLRenderer(size_t width, size_t height)
  : m_width(width)
  , m_height(height)
  , m_fbo(0)
  , m_fboTexture(0)
  , m_depthBuffer(0)
  , m_shaderProgram(0)
  , m_vertexShader(0)
  , m_fragmentShader(0)
  , m_initialized(false)
  , m_blendEnabled(true)
  , m_blendMode(0) {
}

OpenGLRenderer::~OpenGLRenderer() {
  Cleanup();
}

bool OpenGLRenderer::Initialize() {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_initialized) {
    return true;
  }
  
  std::cout << "[OpenGLRenderer] Initializing (" << m_width << "x" << m_height << ")" << std::endl;
  
  // 创建 FBO
  if (!CreateFBO()) {
    std::cerr << "[OpenGLRenderer] Failed to create FBO" << std::endl;
    return false;
  }
  
  // 创建着色器
  if (!CreateShaders()) {
    std::cerr << "[OpenGLRenderer] Failed to create shaders" << std::endl;
    return false;
  }
  
  m_initialized = true;
  std::cout << "[OpenGLRenderer] Initialized successfully" << std::endl;
  
  return true;
}

void OpenGLRenderer::Cleanup() {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (!m_initialized) {
    return;
  }
  
  // 删除纹理
  for (auto& [id, info] : m_textures) {
    if (info.valid && info.textureId != 0) {
      glDeleteTextures(1, &info.textureId);
    }
  }
  m_textures.clear();
  
  // 删除 FBO
  if (m_fbo != 0) {
    glDeleteFramebuffers(1, &m_fbo);
  }
  if (m_fboTexture != 0) {
    glDeleteTextures(1, &m_fboTexture);
  }
  if (m_depthBuffer != 0) {
    glDeleteRenderbuffers(1, &m_depthBuffer);
  }
  
  // 删除着色器
  if (m_shaderProgram != 0) {
    glDeleteProgram(m_shaderProgram);
  }
  if (m_vertexShader != 0) {
    glDeleteShader(m_vertexShader);
  }
  if (m_fragmentShader != 0) {
    glDeleteShader(m_fragmentShader);
  }
  
  m_initialized = false;
  std::cout << "[OpenGLRenderer] Cleanup complete" << std::endl;
}

bool OpenGLRenderer::CreateFBO() {
  // 创建帧缓冲
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  
  // 创建纹理附件
  glGenTextures(1, &m_fboTexture);
  glBindTexture(GL_TEXTURE_2D, m_fboTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);
  
  // 创建深度缓冲
  glGenRenderbuffers(1, &m_depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
  
  // 检查 FBO 完整性
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "[OpenGLRenderer] FBO incomplete: " << status << std::endl;
    return false;
  }
  
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return true;
}

bool OpenGLRenderer::CreateShaders() {
  // 顶点着色器
  const char* vertexSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord;
    
    uniform mat4 model;
    
    void main() {
      gl_Position = model * vec4(aPos, 0.0, 1.0);
      TexCoord = aTexCoord;
    }
  )";
  
  // 片段着色器
  const char* fragmentSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    
    uniform sampler2D texture1;
    uniform float opacity;
    
    void main() {
      vec4 texColor = texture(texture1, TexCoord);
      FragColor = vec4(texColor.rgb, texColor.a * opacity);
    }
  )";
  
  m_vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
  m_fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
  
  if (m_vertexShader == 0 || m_fragmentShader == 0) {
    return false;
  }
  
  m_shaderProgram = CreateProgram(m_vertexShader, m_fragmentShader);
  
  return m_shaderProgram != 0;
}

GLuint OpenGLRenderer::CompileShader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  
  // 检查编译错误
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLchar infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "[OpenGLRenderer] Shader compilation error: " << infoLog << std::endl;
    glDeleteShader(shader);
    return 0;
  }
  
  return shader;
}

GLuint OpenGLRenderer::CreateProgram(GLuint vertexShader, GLuint fragmentShader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
  
  // 检查链接错误
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLchar infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "[OpenGLRenderer] Shader program link error: " << infoLog << std::endl;
    glDeleteProgram(program);
    return 0;
  }
  
  return program;
}

void OpenGLRenderer::BeginFrame() {
  BindFBO();
  glViewport(0, 0, m_width, m_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  
  if (m_blendEnabled) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  
  glUseProgram(m_shaderProgram);
}

void OpenGLRenderer::EndFrame() {
  UnbindFBO();
}

void OpenGLRenderer::BindFBO() {
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void OpenGLRenderer::UnbindFBO() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool OpenGLRenderer::UploadTexture(int layerId, const std::shared_ptr<DecodedFrame>& frame) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (!frame || !frame->data[0]) {
    return false;
  }
  
  auto it = m_textures.find(layerId);
  GLuint textureId;
  
  if (it != m_textures.end() && it->second.valid) {
    // 更新现有纹理
    textureId = it->second.textureId;
  } else {
    // 创建新纹理
    glGenTextures(1, &textureId);
    m_textures[layerId] = TextureInfo{textureId, frame->width, frame->height, true};
  }
  
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame->width, frame->height, 0, 
               GL_RGBA, GL_UNSIGNED_BYTE, frame->data[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  return true;
}

void OpenGLRenderer::DeleteTexture(int layerId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  auto it = m_textures.find(layerId);
  if (it != m_textures.end() && it->second.valid) {
    glDeleteTextures(1, &it->second.textureId);
    m_textures.erase(it);
  }
}

void OpenGLRenderer::DrawLayer(const LayerInfo& layer) {
  auto it = m_textures.find(layer.id);
  if (it == m_textures.end() || !it->second.valid) {
    return;
  }
  
  glUseProgram(m_shaderProgram);
  
  // 设置 uniform
  GLint opacityLoc = glGetUniformLocation(m_shaderProgram, "opacity");
  glUniform1f(opacityLoc, layer.opacity);
  
  // 绑定纹理
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, it->second.textureId);
  
  // 这里应该绘制四边形，简化处理
  // 实际项目中需要 VAO/VBO
}

void OpenGLRenderer::ReadPixels(uint8_t* buffer, size_t width, size_t height) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  BindFBO();
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  UnbindFBO();
}

void OpenGLRenderer::SetViewport(size_t width, size_t height) {
  m_width = width;
  m_height = height;
  glViewport(0, 0, width, height);
}

void OpenGLRenderer::EnableBlend(bool enable) {
  m_blendEnabled = enable;
}

void OpenGLRenderer::SetBlendMode(int mode) {
  m_blendMode = mode;
}

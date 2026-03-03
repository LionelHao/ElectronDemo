/**
 * FFmpeg Decoder Implementation
 */

#include "ffmpeg_decoder.h"
#include <cstring>
#include <iostream>

FFmpegDecoder::FFmpegDecoder()
  : m_nextMediaId(0)
  , m_initialized(false) {
}

FFmpegDecoder::~FFmpegDecoder() {
  Cleanup();
}

bool FFmpegDecoder::Initialize() {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_initialized) {
    return true;
  }
  
  // 初始化 FFmpeg
  avformat_network_init();
  
  m_initialized = true;
  std::cout << "[FFmpegDecoder] Initialized" << std::endl;
  
  return true;
}

void FFmpegDecoder::Cleanup() {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  for (auto& [id, ctx] : m_mediaContexts) {
    if (ctx->swsCtx) {
      sws_freeContext(ctx->swsCtx);
    }
    if (ctx->codecCtx) {
      avcodec_free_context(&ctx->codecCtx);
    }
    if (ctx->formatCtx) {
      avformat_close_input(&ctx->formatCtx);
    }
  }
  
  m_mediaContexts.clear();
  m_initialized = false;
  
  std::cout << "[FFmpegDecoder] Cleanup complete" << std::endl;
}

bool FFmpegDecoder::LoadMedia(const std::string& filePath) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  // 打开输入文件
  AVFormatContext* formatCtx = nullptr;
  if (avformat_open_input(&formatCtx, filePath.c_str(), nullptr, nullptr) != 0) {
    std::cerr << "[FFmpegDecoder] Failed to open file: " << filePath << std::endl;
    return false;
  }
  
  // 读取流信息
  if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
    std::cerr << "[FFmpegDecoder] Failed to find stream info" << std::endl;
    avformat_close_input(&formatCtx);
    return false;
  }
  
  // 查找视频流
  int videoStreamIndex = -1;
  for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
    if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      break;
    }
  }
  
  if (videoStreamIndex == -1) {
    std::cerr << "[FFmpegDecoder] No video stream found" << std::endl;
    avformat_close_input(&formatCtx);
    return false;
  }
  
  // 打开编解码器
  auto ctx = std::make_unique<MediaContext>();
  ctx->formatCtx = formatCtx;
  ctx->videoStreamIndex = videoStreamIndex;
  ctx->filePath = filePath;
  
  if (!OpenCodec(formatCtx, videoStreamIndex)) {
    avformat_close_input(&formatCtx);
    return false;
  }
  
  ctx->codecCtx = avcodec_alloc_context3(nullptr);
  // 实际项目中需要正确初始化解码器
  
  // 创建色彩空间转换上下文
  ctx->swsCtx = sws_getContext(
    ctx->codecCtx->width, ctx->codecCtx->height, ctx->codecCtx->pix_fmt,
    ctx->codecCtx->width, ctx->codecCtx->height, AV_PIX_FMT_RGBA,
    SWS_BILINEAR, nullptr, nullptr, nullptr
  );
  
  // 保存上下文
  int mediaId = m_nextMediaId++;
  m_mediaContexts[mediaId] = std::move(ctx);
  
  std::cout << "[FFmpegDecoder] Media loaded: " << filePath 
            << " (ID: " << mediaId << ")" << std::endl;
  
  return true;
}

std::shared_ptr<DecodedFrame> FFmpegDecoder::DecodeFrame(int mediaId, double timestamp) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  auto it = m_mediaContexts.find(mediaId);
  if (it == m_mediaContexts.end()) {
    return nullptr;
  }
  
  auto& ctx = it->second;
  
  // Seek 到目标位置
  Seek(mediaId, timestamp);
  
  // 解码帧
  AVPacket* packet = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  AVFrame* rgbFrame = av_frame_alloc();
  
  std::shared_ptr<DecodedFrame> result = std::make_shared<DecodedFrame>();
  
  // 读取并解码帧
  while (av_read_frame(ctx->formatCtx, packet) >= 0) {
    if (packet->stream_index == ctx->videoStreamIndex) {
      // 发送包到解码器
      avcodec_send_packet(ctx->codecCtx, packet);
      
      // 接收解码帧
      if (avcodec_receive_frame(ctx->codecCtx, frame) == 0) {
        // 转换像素格式到 RGBA
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, 
                                                frame->width, frame->height, 1);
        uint8_t* buffer = (uint8_t*)av_malloc(numBytes);
        
        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer,
                            AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
        
        sws_scale(ctx->swsCtx, frame->data, frame->linesize, 0,
                 frame->height, rgbFrame->data, rgbFrame->linesize);
        
        // 填充结果
        for (int i = 0; i < 4; i++) {
          result->data[i] = rgbFrame->data[i];
          result->linesize[i] = rgbFrame->linesize[i];
        }
        result->width = frame->width;
        result->height = frame->height;
        result->pts = frame->pts * av_q2d(ctx->formatCtx->streams[ctx->videoStreamIndex]->time_base);
        result->format = AV_PIX_FMT_RGBA;
        
        av_packet_unref(packet);
        break;
      }
    }
    av_packet_unref(packet);
  }
  
  av_frame_free(&frame);
  av_frame_free(&rgbFrame);
  av_packet_free(&packet);
  
  return result;
}

bool FFmpegDecoder::Seek(int mediaId, double timestamp) {
  auto it = m_mediaContexts.find(mediaId);
  if (it == m_mediaContexts.end()) {
    return false;
  }
  
  auto& ctx = it->second;
  
  // 计算时间戳
  int64_t ts = timestamp / av_q2d(ctx->formatCtx->streams[ctx->videoStreamIndex]->time_base);
  
  // Seek 到最近的关键帧
  av_seek_frame(ctx->formatCtx, ctx->videoStreamIndex, ts, AVSEEK_FLAG_BACKWARD);
  avcodec_flush_buffers(ctx->codecCtx);
  
  return true;
}

bool FFmpegDecoder::OpenCodec(AVFormatContext* formatCtx, int streamIndex) {
  AVCodecParameters* codecParams = formatCtx->streams[streamIndex]->codecpar;
  
  // 查找解码器
  const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
  if (!codec) {
    std::cerr << "[FFmpegDecoder] Codec not found" << std::endl;
    return false;
  }
  
  // 尝试启用硬件加速
  // 实际项目中需要实现 EnableHardwareAcceleration
  
  return true;
}

bool FFmpegDecoder::EnableHardwareAcceleration(AVCodecContext* codecCtx) {
  // 硬件加速实现
  // Windows: NVDEC, D3D11VA
  // macOS: VideoToolbox
  // Linux: VAAPI, VDPAU
  
  #ifdef _WIN32
    // Windows 硬件加速
    codecCtx->get_format = [](AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
      // 选择硬件像素格式
      return pix_fmts[0];
    };
  #endif
  
  return true;
}

int FFmpegDecoder::GetWidth(int mediaId) const {
  auto it = m_mediaContexts.find(mediaId);
  if (it != m_mediaContexts.end() && it->second->codecCtx) {
    return it->second->codecCtx->width;
  }
  return 0;
}

int FFmpegDecoder::GetHeight(int mediaId) const {
  auto it = m_mediaContexts.find(mediaId);
  if (it != m_mediaContexts.end() && it->second->codecCtx) {
    return it->second->codecCtx->height;
  }
  return 0;
}

double FFmpegDecoder::GetDuration(int mediaId) const {
  auto it = m_mediaContexts.find(mediaId);
  if (it != m_mediaContexts.end() && it->second->formatCtx) {
    return it->second->formatCtx->duration / (double)AV_TIME_BASE;
  }
  return 0;
}

#ifndef FFmpeg_DECODER_H
#define FFmpeg_DECODER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

// FFmpeg 前向声明
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/**
 * 解码帧数据结构
 */
struct DecodedFrame {
  uint8_t* data[4];      // 平面数据
  int linesize[4];        // 行大小
  int width;
  int height;
  double pts;             // 时间戳
  int format;             // 像素格式
  
  DecodedFrame() {
    std::memset(data, 0, sizeof(data));
    std::memset(linesize, 0, sizeof(linesize));
    width = height = 0;
    pts = 0;
    format = AV_PIX_FMT_NONE;
  }
};

/**
 * 媒体上下文
 */
struct MediaContext {
  AVFormatContext* formatCtx;
  AVCodecContext* codecCtx;
  int videoStreamIndex;
  SwsContext* swsCtx;
  std::string filePath;
  
  MediaContext() 
    : formatCtx(nullptr)
    , codecCtx(nullptr)
    , videoStreamIndex(-1)
    , swsCtx(nullptr) {}
};

/**
 * FFmpegDecoder - FFmpeg 解码器封装
 * 
 * 功能:
 * - 媒体文件加载
 * - 视频解码
 * - 硬件加速支持 (NVDEC, VideoToolbox, VAAPI)
 * - 智能 Seek 策略
 */
class FFmpegDecoder {
public:
  FFmpegDecoder();
  ~FFmpegDecoder();
  
  bool Initialize();
  void Cleanup();
  
  bool LoadMedia(const std::string& filePath);
  std::shared_ptr<DecodedFrame> DecodeFrame(int mediaId, double timestamp);
  bool Seek(int mediaId, double timestamp);
  
  // 获取媒体信息
  int GetWidth(int mediaId) const;
  int GetHeight(int mediaId) const;
  double GetDuration(int mediaId) const;
  
private:
  bool OpenCodec(AVFormatContext* formatCtx, int streamIndex);
  bool EnableHardwareAcceleration(AVCodecContext* codecCtx);
  AVFrame* DecodePacket(AVCodecContext* codecCtx, AVPacket* packet);
  
  std::unordered_map<int, std::unique_ptr<MediaContext>> m_mediaContexts;
  int m_nextMediaId;
  std::mutex m_mutex;
  bool m_initialized;
};

#endif // FFmpeg_DECODER_H

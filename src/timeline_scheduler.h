#ifndef TIMELINE_SCHEDULER_H
#define TIMELINE_SCHEDULER_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <napi.h>

/**
 * 图层类型
 */
enum class LayerType {
  VIDEO,
  AUDIO,
  IMAGE,
  TEXT
};

/**
 * 图层信息
 */
struct LayerInfo {
  int id;
  LayerType type;
  std::string name;
  int mediaId;
  double startTime;     // 在时间轴上的开始时间
  double endTime;       // 在时间轴上的结束时间
  double offset;        // 媒体内部偏移
  double positionX;     // X 位置
  double positionY;     // Y 位置
  double scale;         // 缩放
  double rotation;      // 旋转
  double opacity;       // 不透明度
  bool visible;
  
  LayerInfo()
    : id(0)
    , type(LayerType::VIDEO)
    , mediaId(-1)
    , startTime(0)
    , endTime(0)
    , offset(0)
    , positionX(0)
    , positionY(0)
    , scale(1.0)
    , rotation(0)
    , opacity(1.0)
    , visible(true) {}
};

/**
 * TimelineScheduler - 时间轴调度器
 * 
 * 功能:
 * - 解析时间轴数据结构
 * - 计算活跃图层
 * - 图层 Z 顺序管理
 */
class TimelineScheduler {
public:
  TimelineScheduler();
  ~TimelineScheduler();
  
  // 设置时间轴
  void SetTimeline(const Napi::Object& timeline);
  
  // 获取指定时间的活跃图层
  std::vector<LayerInfo> GetActiveLayers(double timestamp);
  
  // 获取所有图层
  const std::vector<LayerInfo>& GetAllLayers() const { return m_layers; }
  
  // 添加/删除图层
  void AddLayer(const LayerInfo& layer);
  void RemoveLayer(int layerId);
  void UpdateLayer(int layerId, const LayerInfo& layer);
  
  // 获取时间轴信息
  double GetDuration() const { return m_duration; }
  size_t GetLayerCount() const { return m_layers.size(); }
  
private:
  LayerType ParseLayerType(const std::string& type);
  LayerInfo ParseLayer(const Napi::Object& layerObj);
  
  std::vector<LayerInfo> m_layers;
  double m_duration;
  std::mutex m_mutex;
  bool m_dirty;
};

#endif // TIMELINE_SCHEDULER_H

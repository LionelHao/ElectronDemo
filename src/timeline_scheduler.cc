/**
 * Timeline Scheduler Implementation
 */

#include "timeline_scheduler.h"
#include <algorithm>
#include <iostream>

TimelineScheduler::TimelineScheduler()
  : m_duration(30.0)
  , m_dirty(true) {
}

TimelineScheduler::~TimelineScheduler() {
}

void TimelineScheduler::SetTimeline(const Napi::Object& timeline) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  Napi::Env env = timeline.Env();
  
  try {
    // 解析时间轴数据
    if (timeline.Has("duration")) {
      m_duration = timeline.Get("duration").As<Napi::Number>().DoubleValue();
    }
    
    m_layers.clear();
    
    if (timeline.Has("tracks")) {
      Napi::Array tracks = timeline.Get("tracks").As<Napi::Array>();
      
      for (uint32_t i = 0; i < tracks.Length(); i++) {
        Napi::Value trackValue = tracks.Get(i);
        if (trackValue.IsObject()) {
          Napi::Object track = trackValue.As<Napi::Object>();
          
          if (track.Has("clips")) {
            Napi::Array clips = track.Get("clips").As<Napi::Array>();
            
            for (uint32_t j = 0; j < clips.Length(); j++) {
              Napi::Value clipValue = clips.Get(j);
              if (clipValue.IsObject()) {
                LayerInfo layer = ParseLayer(clipValue.As<Napi::Object>());
                m_layers.push_back(layer);
              }
            }
          }
        }
      }
    }
    
    m_dirty = true;
    
    std::cout << "[TimelineScheduler] Timeline set: " << m_layers.size() 
              << " layers, duration: " << m_duration << "s" << std::endl;
    
  } catch (const std::exception& e) {
    std::cerr << "[TimelineScheduler] Error parsing timeline: " << e.what() << std::endl;
  }
}

std::vector<LayerInfo> TimelineScheduler::GetActiveLayers(double timestamp) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  std::vector<LayerInfo> active;
  
  for (const auto& layer : m_layers) {
    if (layer.visible && 
        timestamp >= layer.startTime && 
        timestamp <= layer.endTime) {
      active.push_back(layer);
    }
  }
  
  // 按 Z 顺序排序（这里简化为数组顺序）
  std::sort(active.begin(), active.end(), 
            [](const LayerInfo& a, const LayerInfo& b) {
              return a.id < b.id;
            });
  
  return active;
}

void TimelineScheduler::AddLayer(const LayerInfo& layer) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_layers.push_back(layer);
  m_dirty = true;
}

void TimelineScheduler::RemoveLayer(int layerId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  m_layers.erase(
    std::remove_if(m_layers.begin(), m_layers.end(),
                   [layerId](const LayerInfo& layer) {
                     return layer.id == layerId;
                   }),
    m_layers.end()
  );
  m_dirty = true;
}

void TimelineScheduler::UpdateLayer(int layerId, const LayerInfo& layer) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  for (auto& l : m_layers) {
    if (l.id == layerId) {
      l = layer;
      m_dirty = true;
      break;
    }
  }
}

LayerType TimelineScheduler::ParseLayerType(const std::string& type) {
  if (type == "video") return LayerType::VIDEO;
  if (type == "audio") return LayerType::AUDIO;
  if (type == "image") return LayerType::IMAGE;
  if (type == "text") return LayerType::TEXT;
  return LayerType::VIDEO;
}

LayerInfo TimelineScheduler::ParseLayer(const Napi::Object& layerObj) {
  LayerInfo layer;
  
  if (layerObj.Has("id")) {
    layer.id = layerObj.Get("id").As<Napi::Number>().Int32Value();
  }
  if (layerObj.Has("type")) {
    std::string type = layerObj.Get("type").As<Napi::String>().Utf8Value();
    layer.type = ParseLayerType(type);
  }
  if (layerObj.Has("name")) {
    layer.name = layerObj.Get("name").As<Napi::String>().Utf8Value();
  }
  if (layerObj.Has("start")) {
    layer.startTime = layerObj.Get("start").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("end")) {
    layer.endTime = layerObj.Get("end").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("offset")) {
    layer.offset = layerObj.Get("offset").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("positionX")) {
    layer.positionX = layerObj.Get("positionX").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("positionY")) {
    layer.positionY = layerObj.Get("positionY").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("scale")) {
    layer.scale = layerObj.Get("scale").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("rotation")) {
    layer.rotation = layerObj.Get("rotation").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("opacity")) {
    layer.opacity = layerObj.Get("opacity").As<Napi::Number>().DoubleValue();
  }
  if (layerObj.Has("visible")) {
    layer.visible = layerObj.Get("visible").As<Napi::Boolean>().Value();
  }
  
  return layer;
}

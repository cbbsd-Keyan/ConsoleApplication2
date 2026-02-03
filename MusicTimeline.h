#pragma once
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <iostream>
#include <SFML/Graphics.hpp>

// 事件类型枚举
enum class VisualEventType {
    CURTAIN_RISE,           // 幕布升起
    PARTICLES_APPEAR,       // 粒子出现
    FREQUENCY_PEAKS,        // 频率峰
    EPIC_EXPLOSION,         // 史诗爆发
    SCREEN_SHAKE,           // 屏幕震动
    VIOLIN_SOLO,           // 小提琴独奏
    LAYER_TRANSITION,      // 图层过渡
    COLOR_CHANGE,          // 颜色改变
    BACKGROUND_CHANGE,     // 背景变化
    MELODY_HIGHLIGHT       // 旋律高亮
};

// 事件结构体
struct TimelineEvent {
    float time;                    // 触发时间（秒）
    VisualEventType type;          // 事件类型
    std::string description;       // 事件描述
    std::map<std::string, float> parameters;  // 参数
    bool triggered;                // 是否已触发

    TimelineEvent(float t, VisualEventType ty, const std::string& desc = "")
        : time(t), type(ty), description(desc), triggered(false) {
    }

    // 添加参数
    void addParam(const std::string& key, float value) {
        parameters[key] = value;
    }

    float getParam(const std::string& key, float defaultValue = 0.0f) const {
        auto it = parameters.find(key);
        if (it != parameters.end()) return it->second;
        return defaultValue;
    }
};

// 时间轴类
class MusicTimeline {
private:
    std::vector<TimelineEvent> events;
    float currentTime;
    bool isPlaying;

    // 回调函数系统
    std::map<VisualEventType, std::function<void(const TimelineEvent&)>> callbacks;

public:
    MusicTimeline() : currentTime(0.0f), isPlaying(false) {
        // 初始化默认回调（空函数）
        for (int i = 0; i < static_cast<int>(VisualEventType::MELODY_HIGHLIGHT) + 1; i++) {
            callbacks[static_cast<VisualEventType>(i)] = [](const TimelineEvent&) {};
        }
    }

    // 添加事件
    void addEvent(const TimelineEvent& event) {
        events.push_back(event);
        // 按时间排序
        std::sort(events.begin(), events.end(),
            [](const TimelineEvent& a, const TimelineEvent& b) {
                return a.time < b.time;
            });
    }

    // 设置回调函数
    void setCallback(VisualEventType type, std::function<void(const TimelineEvent&)> callback) {
        callbacks[type] = callback;
    }

    // 更新时间轴
    void update(float audioTime) {
        if (!isPlaying) return;

        currentTime = audioTime;

        // 检查并触发事件
        for (auto& event : events) {
            if (!event.triggered && currentTime >= event.time) {
                triggerEvent(event);
                event.triggered = true;
            }
        }
    }

    // 触发事件
    void triggerEvent(TimelineEvent& event) {
        std::cout << "[时间轴] " << event.time << "s: " << event.description << std::endl;

        // 调用对应的回调函数
        if (callbacks.find(event.type) != callbacks.end()) {
            callbacks[event.type](event);
        }
    }

    // 重置时间轴
    void reset() {
        currentTime = 0.0f;
        for (auto& event : events) {
            event.triggered = false;
        }
    }

    // 播放控制
    void play() { isPlaying = true; }
    void pause() { isPlaying = false; }
    void stop() {
        isPlaying = false;
        reset();
    }

    // 获取当前时间
    float getCurrentTime() const { return currentTime; }

    // 获取下一个事件的时间
    float getNextEventTime() const {
        for (const auto& event : events) {
            if (!event.triggered) return event.time;
        }
        return -1.0f; // 没有更多事件
    }

    // 手动跳转到某个时间点
    void seek(float time) {
        currentTime = time;
        // 重置所有在time之后的事件
        for (auto& event : events) {
            event.triggered = (event.time <= time);
        }
    }

    // 创建《自新大陆》专用时间轴
    static MusicTimeline createDvorakTimeline() {
        MusicTimeline timeline;

        // 0:00 - 0:17 暗流涌动
        timeline.addEvent(TimelineEvent(0.0f, VisualEventType::CURTAIN_RISE,
            "幕布升起，深色背景出现"));

        timeline.addEvent(TimelineEvent(0.5f, VisualEventType::PARTICLES_APPEAR,
            "金色粒子群开始出现"));

        timeline.addEvent(TimelineEvent(8.0f, VisualEventType::FREQUENCY_PEAKS,
            "第二遍弦乐开始，频率峰增强"));

        // 0:17 史诗爆发
        TimelineEvent epicEvent(17.0f, VisualEventType::EPIC_EXPLOSION,
            "主旋律爆发，史诗感");
        epicEvent.addParam("intensity", 1.0f);
        epicEvent.addParam("duration", 2.0f);
        timeline.addEvent(epicEvent);

        timeline.addEvent(TimelineEvent(17.1f, VisualEventType::SCREEN_SHAKE,
            "屏幕震动效果"));

        // 0:44 小提琴独奏
        timeline.addEvent(TimelineEvent(44.0f, VisualEventType::VIOLIN_SOLO,
            "小提琴独奏开始"));

        // 0:56 再次激昂
        timeline.addEvent(TimelineEvent(56.0f, VisualEventType::COLOR_CHANGE,
            "色调向深金色渐变"));

        // 1:13 欢快段落
        timeline.addEvent(TimelineEvent(73.0f, VisualEventType::BACKGROUND_CHANGE,
            "百叶窗背景变为活泼金色"));

        // 检测特定旋律：mi-fa-so-fa-mi
        timeline.addEvent(TimelineEvent(17.5f, VisualEventType::MELODY_HIGHLIGHT,
            "主旋律高亮"));

        return timeline;
    }
};
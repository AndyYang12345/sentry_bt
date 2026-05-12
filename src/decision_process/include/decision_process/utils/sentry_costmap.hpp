#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <string>

// ============================================================
// SentryCostmap — 用于决策计算的代价地图
// ============================================================
//
//   设计思路:
//     1. 手动注册障碍物（仿真过渡期），后续可从 RViz / Gazebo / 雷达获取
//     2. 对任意坐标查询高度、是否可通过、威胁代价等
//     3. 作为 CalculateSafePosition 等节点的辅助工具类
//
//   使用示例:
//     SentryCostmap costmap(28.0, 15.0);
//     costmap.registerWall("wall_a", 3.0, 7.0, 4.0, 3.0, 1.0);
//     double h = costmap.heightAt(3.5, 5.0);  // → 1.0
//

class SentryCostmap {
public:
    // ============================================================
    // 数据结构
    // ============================================================

    // --- 长方体墙面 ---
    struct Wall {
        std::string id;
        double x1, y1;   // 底面矩形一角 (m)
        double x2, y2;   // 底面矩形对角 (m)
        double height;   // 高度 (m)

        double x_low()  const { return std::min(x1, x2); }
        double x_high() const { return std::max(x1, x2); }
        double y_low()  const { return std::min(y1, y2); }
        double y_high() const { return std::max(y1, y2); }

        bool containsXY(double x, double y) const {
            return x >= x_low() && x <= x_high() &&
                   y >= y_low() && y <= y_high();
        }
    };

    // --- 威胁源 (敌方单位) ---
    struct Threat {
        int    id;
        double x, y;
        double weight;
    };

    // --- 查询结果 ---
    struct CellInfo {
        double height;
        bool   blocked;
        const Wall* blocking_wall;
    };

    // --- 安全点 ---
    struct SafePoint {
        double x, y, cost;
    };

    // ============================================================
    // 构造
    // ============================================================
    SentryCostmap() = default;
    SentryCostmap(double map_width, double map_height)
        : map_width_(map_width), map_height_(map_height) {}

    void setMapSize(double w, double h) { map_width_ = w; map_height_ = h; }
    double mapWidth()  const { return map_width_; }
    double mapHeight() const { return map_height_; }

    // ============================================================
    // 墙面注册
    // ============================================================
    void registerWall(const std::string& id,
                      double x1, double y1, double x2, double y2,
                      double height = 1.0)
    {
        Wall w{id, x1, y1, x2, y2, height};
        walls_.push_back(w);
        wall_map_[id] = walls_.size() - 1;
    }

    size_t wallCount() const { return walls_.size(); }
    const Wall* getWall(size_t i) const {
        return (i < walls_.size()) ? &walls_[i] : nullptr;
    }
    const Wall* getWallById(const std::string& id) const {
        auto it = wall_map_.find(id);
        return (it != wall_map_.end()) ? &walls_[it->second] : nullptr;
    }

    // ============================================================
    // 地图查询
    // ============================================================
    double heightAt(double x, double y) const {
        for (const auto& w : walls_) {
            if (w.containsXY(x, y)) return w.height;
        }
        return 0.0;
    }

    bool isPassable(double x, double y) const {
        return heightAt(x, y) == 0.0;
    }

    CellInfo cellInfo(double x, double y) const {
        CellInfo info{0.0, false, nullptr};
        for (auto& w : walls_) {
            if (w.containsXY(x, y)) {
                info.height = w.height;
                info.blocked = true;
                info.blocking_wall = &w;
                return info;
            }
        }
        return info;
    }

    bool lineOfSight(double x1, double y1, double x2, double y2,
                     double step = 0.5) const
    {
        double dx = x2 - x1, dy = y2 - y1;
        double dist = std::hypot(dx, dy);
        if (dist < 1e-6) return isPassable(x1, y1);
        int samples = static_cast<int>(dist / step);
        for (int i = 0; i <= samples; ++i) {
            double t = (samples > 0) ? static_cast<double>(i) / samples : 0.0;
            if (!isPassable(x1 + dx * t, y1 + dy * t))
                return false;
        }
        return true;
    }

    // ============================================================
    // 威胁计算
    // ============================================================
    void setThreat(int id, double x, double y, double weight = 1.0) {
        for (auto& t : threats_) {
            if (t.id == id) { t.x = x; t.y = y; t.weight = weight; return; }
        }
        threats_.push_back({id, x, y, weight});
    }

    void clearThreats() { threats_.clear(); }
    const std::vector<Threat>& threats() const { return threats_; }

    double threatCost(double x, double y, double blocked_penalty = 1000.0) const
    {
        if (!isPassable(x, y)) return blocked_penalty;
        double cost = 0.0;
        const double eps = 1e-6;
        for (const auto& t : threats_) {
            cost += t.weight / (std::hypot(x - t.x, y - t.y) + eps);
        }
        return cost;
    }

    SafePoint findSafePoint(double cx, double cy,
                            double search_radius = 5.0,
                            double grid_step = 0.5,
                            double blocked_penalty = 1000.0) const
    {
        SafePoint best{cx, cy, std::numeric_limits<double>::max()};
        double x_min = std::max(0.0, cx - search_radius);
        double x_max = std::min(map_width_, cx + search_radius);
        double y_min = std::max(0.0, cy - search_radius);
        double y_max = std::min(map_height_, cy + search_radius);
        for (double y = y_min; y <= y_max; y += grid_step) {
            for (double x = x_min; x <= x_max; x += grid_step) {
                if (!isPassable(x, y)) continue;
                double cost = threatCost(x, y, blocked_penalty);
                if (cost < best.cost) best = {x, y, cost};
            }
        }
        // 兜底：若无可用点，保持当前位置，代价设为超大值
        if (best.cost >= std::numeric_limits<double>::max())
            best = {cx, cy, best.cost};
        return best;
    }

private:
    double map_width_  = 28.0;
    double map_height_ = 15.0;
    std::vector<Wall> walls_;
    std::unordered_map<std::string, size_t> wall_map_;
    std::vector<Threat> threats_;
};

// ============================================================
// 临时测试地图 — 仿真过渡期
// ============================================================
inline SentryCostmap makeTempMap() {
    SentryCostmap cm(28.0, 15.0);
    cm.registerWall("wall_demo",      3.0, 7.0,  4.0, 3.0,  1.0);
    cm.registerWall("left_barrier",   0.0, 0.0,  1.0, 15.0, 2.0);
    cm.registerWall("right_barrier", 27.0, 0.0, 28.0, 15.0, 2.0);
    cm.registerWall("top_barrier",    0.0, 14.0, 28.0, 15.0, 2.0);
    cm.registerWall("bottom_barrier", 0.0, 0.0,  28.0, 1.0,  2.0);
    return cm;
}

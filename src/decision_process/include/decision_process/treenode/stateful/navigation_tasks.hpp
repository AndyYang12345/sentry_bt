#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <random>

// ============================================================
// 移动任务节点 — StatefulActionNode（跨 tick 持续）
// ============================================================

// ---------- ① GoToPoint: 基础导航 ----------

/*
    @brief 导航到目标点，到达后返回 SUCCESS
    黑板读取: nav_target_x, nav_target_y, current_x, current_y (由 odom 话题更新)
    黑板写入：nav_cancel,被halt启动
    @return RUNNING 移动中; SUCCESS 到达; FAILURE 超时、被打断或输入错误
*/
class GoToPoint : public BT::StatefulActionNode {
public:
    GoToPoint(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("current_x",    "当前X坐标"),
            BT::InputPort<double>("current_y",    "当前Y坐标"),
            BT::InputPort<double>("XY_TOLERANCE", "到达容差(m)"),
            BT::InputPort<double>("nav_target_x", "导航目标X"),
            BT::InputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        start_time_ = std::chrono::steady_clock::now();
        if (!getInput("nav_target_x", nav_target_x_) || !getInput("nav_target_y", nav_target_y_))
            return BT::NodeStatus::FAILURE;
        double tol = 0.2;
        getInput("XY_TOLERANCE", tol);
        tolerance_ = tol;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double current_x = 0, current_y = 0;
        if(!getInput("current_x", current_x) || !getInput("current_y", current_y)){
            return BT::NodeStatus::FAILURE;
        }

        if (std::hypot(current_x - nav_target_x_, current_y - nav_target_y_) < tolerance_)
            return BT::NodeStatus::SUCCESS;

        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }

protected:
    double nav_target_x_ = 0, nav_target_y_ = 0, tolerance_ = 0.2, timeout_ = 30.0;
    std::chrono::steady_clock::time_point start_time_;

private:
    double elapsed() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time_).count();
    }
};

// GoHome
class GoHome : public GoToPoint {
public:
    GoHome(const std::string& name, const BT::NodeConfig& config)
        : GoToPoint(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("current_x",    "当前X坐标"),
            BT::InputPort<double>("current_y",    "当前Y坐标"),
            BT::InputPort<double>("HOME_X", "预设回家点X坐标"),
            BT::InputPort<double>("HOME_Y", "预设回家点Y坐标"),
            BT::InputPort<double>("XY_TOLERANCE", "到达容差(m)"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        double hx = 0.0, hy = 0.0;
        if (!getInput("HOME_X", hx) || !getInput("HOME_Y", hy)) {
            return BT::NodeStatus::FAILURE;
        }
        nav_target_x_ = hx;
        nav_target_y_ = hy;

        double tol = 0.2;
        getInput("XY_TOLERANCE", tol);
        tolerance_ = tol;

        start_time_ = std::chrono::steady_clock::now();
        return BT::NodeStatus::RUNNING;
    }
};

// ---------- ② Patrol: 巡逻（遍历点位列表）----------

/*
    @brief 在给定的点位列表中循环巡逻
    黑板读取: patrol_points_x, patrol_points_y (vector<double>, 在xml或黑板初始化)
    每到达一个点后切到下一个，到末尾回绕
    @return RUNNING 永久循环（被高优先级打断才退出）
*/
class Patrol : public BT::StatefulActionNode {
public:
    Patrol(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<std::vector<double>>("patrol_points_x", "巡逻点X坐标列表"),
            BT::InputPort<std::vector<double>>("patrol_points_y", "巡逻点Y坐标列表"),
            BT::InputPort<double>("current_x", "当前X坐标"),
            BT::InputPort<double>("current_y", "当前Y坐标"),
            BT::InputPort<double>("XY_TOLERANCE", "到达容差(m), 默认0.2"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        std::vector<double> px, py;
        if (!getInput("patrol_points_x", px) || !getInput("patrol_points_y", py))
            return BT::NodeStatus::FAILURE;
        if (px.empty() || px.size() != py.size())
            return BT::NodeStatus::FAILURE;
        points_x_ = px;
        points_y_ = py;
        current_idx_ = 0;
        publishTarget();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double cx = 0, cy = 0;
        if (!getInput("current_x", cx) || !getInput("current_y", cy))
            return BT::NodeStatus::FAILURE;

        double tx = points_x_[current_idx_];
        double ty = points_y_[current_idx_];

        getInput("XY_TOLERANCE", xy_tolerance_);

        if (std::hypot(cx - tx, cy - ty) < xy_tolerance_) {
            current_idx_ = (current_idx_ + 1) % points_x_.size();
            publishTarget();
        }
        // 巡逻永不自行结束，等待高优先级打断
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }

private:
    void publishTarget() {
        setOutput("nav_target_x", points_x_[current_idx_]);
        setOutput("nav_target_y", points_y_[current_idx_]);
    }
    std::vector<double> points_x_, points_y_;
    size_t current_idx_ = 0;
    float xy_tolerance_ = 0.2;
};

// ---------- ③ Dodge: 躲避（预设安全点 / 动态安全点，通过 flag 切换）----------

/*
    @brief 受伤且无敌方视野时，循环移动至安全点以躲避攻击
    黑板读取: safe_points_x, safe_points_y (vector<double>) — 安全点位坐标列表
    黑板读取: use_dynamic_dodge (bool, 默认false) — 若为 true，安全点由 CalculateSafePosition 动态生成
    每到达一个点后切到下一个，到末尾回绕
    @return RUNNING 永久循环（被高优先级打断才退出）
*/
class Dodge : public BT::StatefulActionNode {
public:
    Dodge(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<std::vector<double>>("safe_points_x", "躲避安全点X坐标列表"),
            BT::InputPort<std::vector<double>>("safe_points_y", "躲避安全点Y坐标列表"),
            BT::InputPort<bool>("use_dynamic_dodge", "是否启用动态安全点计算, 默认false"),
            BT::InputPort<double>("current_x", "当前X坐标"),
            BT::InputPort<double>("current_y", "当前Y坐标"),
            BT::InputPort<double>("XY_TOLERANCE", "到达容差(m), 默认0.2"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        std::vector<double> sx, sy;
        if (!getInput("safe_points_x", sx) || !getInput("safe_points_y", sy))
            return BT::NodeStatus::FAILURE;

        // re-read every tick if dynamic mode is on
        getInput("use_dynamic_dodge", use_dynamic_);

        if (sx.empty() || sx.size() != sy.size())
            return BT::NodeStatus::FAILURE;
        safe_x_ = sx;
        safe_y_ = sy;
        current_idx_ = 0;
        publishTarget();
        getInput("XY_TOLERANCE", xy_tolerance_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double cx = 0, cy = 0;
        if (!getInput("current_x", cx) || !getInput("current_y", cy))
            return BT::NodeStatus::FAILURE;

        // 动态模式: 每 tick 重新读取安全点（由 CalculateSafePosition 实时更新）
        if (use_dynamic_) {
            std::vector<double> sx, sy;
            if (!getInput("safe_points_x", sx) || !getInput("safe_points_y", sy))
                return BT::NodeStatus::FAILURE;
            if (!sx.empty() && sx.size() == sy.size()) {
                safe_x_ = sx;
                safe_y_ = sy;
                current_idx_ = 0;  // 重新开始遍历
            }
        }

        double tx = safe_x_[current_idx_];
        double ty = safe_y_[current_idx_];

        if (std::hypot(cx - tx, cy - ty) < xy_tolerance_) {
            current_idx_ = (current_idx_ + 1) % safe_x_.size();
            publishTarget();
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }

private:
    void publishTarget() {
        setOutput("nav_target_x", safe_x_[current_idx_]);
        setOutput("nav_target_y", safe_y_[current_idx_]);
    }
    std::vector<double> safe_x_, safe_y_;
    size_t current_idx_ = 0;
    float xy_tolerance_ = 0.2;
    bool use_dynamic_ = false;
};

// ---------- ④ ChaseEnemy: 追击 + 自适应距离 ----------
/*
    @brief 追击敌人并自适应保持最佳射击距离
    距离 > 4m  → 靠近到 2.8m   (追击)
    距离 1.2~4 → 保持在 2.8m   (微调)
    距离 < 1.2 → 后退到 1.8m   (防贴脸)
    参数均通过 XML 端口配置
    @return RUNNING 持续维持; FAILURE 雷达丢失/超时
*/
class ChaseEnemy : public BT::StatefulActionNode {
public:
    ChaseEnemy(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<uint8_t>("target_id",        "追击目标编号1-5"),
            BT::InputPort<std::vector<float>>("radar_data", "雷达坐标数组"),
            BT::InputPort<double>("current_x",         "当前X坐标"),
            BT::InputPort<double>("current_y",         "当前Y坐标"),
            BT::InputPort<double>("min_dist",           "后退阈值(m), 默认1.2"),
            BT::InputPort<double>("max_dist",           "追击阈值(m), 默认4.0"),
            BT::InputPort<double>("close_to",           "靠近到(m), 默认2.8"),
            BT::InputPort<double>("back_to",            "后退到(m), 默认1.8"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel",     "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        getInput("min_dist", min_dist_);   getInput("max_dist", max_dist_);
        getInput("close_to", close_to_);   getInput("back_to", back_to_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        uint8_t tid = 0;
        if (!getInput("target_id", tid) || tid == 0)
            return BT::NodeStatus::FAILURE;

        std::vector<float> radar;
        if (!getInput("radar_data", radar) || radar.size() < 12)
            return BT::NodeStatus::RUNNING;

        size_t base = static_cast<size_t>(2 * (tid - 1));
        double ex = radar[base], ey = radar[base + 1];
        if (ex == 0.0 && ey == 0.0) return BT::NodeStatus::FAILURE;

        double cx = 0, cy = 0;
        if (!getInput("current_x", cx) || !getInput("current_y", cy))
            return BT::NodeStatus::FAILURE;

        double dist = std::hypot(cx - ex, cy - ey);
        double dx = ex - cx, dy = ey - cy;

        if (dist < 0.01) return BT::NodeStatus::SUCCESS;

        if (dist > max_dist_) {
            // 太远 → 追击到 close_to_
            double tx = cx + (dx / dist) * (dist - close_to_);
            double ty = cy + (dy / dist) * (dist - close_to_);
            setOutput("nav_target_x", tx);
            setOutput("nav_target_y", ty);
            return BT::NodeStatus::RUNNING;  // 移动中，阻塞射击
        } else if (dist < min_dist_) {
            // 太近 → 后退到 back_to_
            double tx = cx - (dx / dist) * (back_to_ - dist);
            double ty = cy - (dy / dist) * (back_to_ - dist);
            setOutput("nav_target_x", tx);
            setOutput("nav_target_y", ty);
            return BT::NodeStatus::RUNNING;  // 移动中，阻塞射击
        }
        // 在 [min_dist, max_dist] 舒适区内 → 不移动，放行射击
        return BT::NodeStatus::SUCCESS;
    }

    void onHalted() override { setOutput("nav_cancel", true); }

private:
    double min_dist_ = 1.2, max_dist_ = 4.0, close_to_ = 2.8, back_to_ = 1.8;
};
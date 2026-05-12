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
    黑板写入: nav_target_x, nav_target_y (供导航模块读取)
    黑板读取: current_x, current_y (由 odom 话题更新)
    @return RUNNING 移动中; SUCCESS 到达; FAILURE 超时
*/
class GoToPoint : public BT::StatefulActionNode {
public:
    GoToPoint(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("target_x",    "目标X坐标"),
            BT::InputPort<double>("target_y",    "目标Y坐标"),
            BT::InputPort<double>("xy_tolerance", "到达容差(m)"),
            BT::InputPort<double>("timeout",      "超时时间(s)"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        start_time_ = std::chrono::steady_clock::now();
        if (!getInput("target_x", target_x_) || !getInput("target_y", target_y_))
            return BT::NodeStatus::FAILURE;
        double tol = 0.2, tmo = 30.0;
        getInput("xy_tolerance", tol);
        getInput("timeout", tmo);
        tolerance_ = tol;
        timeout_   = tmo;

        setOutput("nav_target_x", target_x_);
        setOutput("nav_target_y", target_y_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double cx = 0, cy = 0;
        (void)config().blackboard->get("current_x", cx);
        (void)config().blackboard->get("current_y", cy);

        if (std::hypot(cx - target_x_, cy - target_y_) < tolerance_)
            return BT::NodeStatus::SUCCESS;

        if (elapsed() > timeout_)
            return BT::NodeStatus::FAILURE;

        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }

private:
    double elapsed() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time_).count();
    }
    double target_x_ = 0, target_y_ = 0, tolerance_ = 0.2, timeout_ = 30.0;
    std::chrono::steady_clock::time_point start_time_;
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
        (void)config().blackboard->get("current_x", cx);
        (void)config().blackboard->get("current_y", cy);

        double tx = points_x_[current_idx_];
        double ty = points_y_[current_idx_];

        if (std::hypot(cx - tx, cy - ty) < 0.2) {
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
};

// ---------- ③ Dodge: 躲避（受伤害时随机移动）----------

/*
    @brief 受伤且无敌方视野时，随机移动以躲避攻击
    生成随机目标点（在当前点附近或预设安全点），持续移动
    每次到达后再随机新点，直到伤害解除（由外部条件打断）
    @return RUNNING 永久循环
*/
class Dodge : public BT::StatefulActionNode {
public:
    Dodge(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config), gen_(rd_()) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("dodge_radius", "随机躲避半径(m), 默认2.0"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        double radius = 2.0;
        getInput("dodge_radius", radius);
        radius_ = radius;
        pickNewTarget();
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        double cx = 0, cy = 0;
        (void)config().blackboard->get("current_x", cx);
        (void)config().blackboard->get("current_y", cy);

        if (std::hypot(cx - dodge_x_, cy - dodge_y_) < 0.3) {
            pickNewTarget();  // 到了，随机下一个点
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }

private:
    void pickNewTarget() {
        double cx = 0, cy = 0;
        (void)config().blackboard->get("current_x", cx);
        (void)config().blackboard->get("current_y", cy);

        std::uniform_real_distribution<> dist(-radius_, radius_);
        dodge_x_ = cx + dist(gen_);
        dodge_y_ = cy + dist(gen_);

        setOutput("nav_target_x", dodge_x_);
        setOutput("nav_target_y", dodge_y_);
    }

    double dodge_x_ = 0, dodge_y_ = 0, radius_ = 2.0;
    std::random_device rd_;
    std::mt19937 gen_;
};

// ---------- ④ ChaseEnemy: 追击（持续更新目标坐标）----------

/*
    @brief 追击指定敌方单位，持续更新目标位置
    黑板读取: target_id, enemy_x, enemy_y (由雷达/自瞄更新)
    每个 tick 都刷新导航目标为该敌人的最新位置
    @return RUNNING 追击中; SUCCESS 目标消失/死亡; FAILURE 超时
*/
class ChaseEnemy : public BT::StatefulActionNode {
public:
    ChaseEnemy(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<uint8_t>("target_id",       "追击目标编号1-5"),
            BT::InputPort<double>("chase_distance",    "追击停止距离(m), 默认2.8"),
            BT::InputPort<double>("timeout",            "超时(s), 默认20"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override {
        start_time_ = std::chrono::steady_clock::now();
        double cd = 2.8, tmo = 20.0;
        getInput("chase_distance", cd);
        getInput("timeout", tmo);
        chase_dist_ = cd;
        timeout_    = tmo;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override {
        uint8_t tid = 0;
        (void)config().blackboard->get("target_id", tid);
        if (tid == 0) return BT::NodeStatus::FAILURE;

        // 从雷达坐标中取目标位置 (radar_data[2*(tid-1)], radar_data[2*(tid-1)+1])
        std::vector<float> radar;
        (void)config().blackboard->get("radar_data", radar);
        if (radar.size() < 12) return BT::NodeStatus::RUNNING;

        size_t base = static_cast<size_t>(2 * (tid - 1));
        double ex = radar[base];
        double ey = radar[base + 1];
        if (ex == 0.0 && ey == 0.0) return BT::NodeStatus::FAILURE; // 雷达丢失

        setOutput("nav_target_x", ex);
        setOutput("nav_target_y", ey);

        double cx = 0, cy = 0;
        (void)config().blackboard->get("current_x", cx);
        (void)config().blackboard->get("current_y", cy);

        if (std::hypot(cx - ex, cy - ey) < chase_dist_)
            return BT::NodeStatus::SUCCESS;

        if (elapsed() > timeout_)
            return BT::NodeStatus::FAILURE;

        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }

private:
    double elapsed() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time_).count();
    }
    double chase_dist_ = 2.8, timeout_ = 20.0;
    std::chrono::steady_clock::time_point start_time_;
};

// ---------- ⑤ KeepDistance: 自适应距离 ----------

/*
    @brief 与目标保持安全距离
    太近(<1.2m)后退，太远(>4m)靠近，在1.8~2.8m之间停止
    黑板读取: target_id, radar_data
    @return RUNNING 调整中; FAILURE 目标丢失
*/
class KeepDistance : public BT::StatefulActionNode {
public:
    KeepDistance(const std::string& name, const BT::NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<uint8_t>("target_id", "目标编号"),
            BT::InputPort<double>("min_dist", "太近阈值(m)"),
            BT::InputPort<double>("max_dist", "太远阈值(m)"),
            BT::InputPort<double>("close_to", "靠近到(m)"),
            BT::InputPort<double>("back_to", "后退到(m)"),
            BT::OutputPort<double>("nav_target_x", "导航目标X"),
            BT::OutputPort<double>("nav_target_y", "导航目标Y"),
            BT::OutputPort<bool>("nav_cancel", "取消导航"),
        };
    }

    BT::NodeStatus onStart() override { return BT::NodeStatus::RUNNING; }

    BT::NodeStatus onRunning() override {
        uint8_t tid = 0;
        (void)config().blackboard->get("target_id", tid);
        if (tid == 0) return BT::NodeStatus::FAILURE;

        std::vector<float> radar;
        (void)config().blackboard->get("radar_data", radar);
        if (radar.size() < 12) return BT::NodeStatus::RUNNING;

        size_t base = static_cast<size_t>(2 * (tid - 1));
        double ex = radar[base];
        double ey = radar[base + 1];
        if (ex == 0.0 && ey == 0.0) return BT::NodeStatus::FAILURE;

        double cx = 0, cy = 0;
        (void)config().blackboard->get("current_x", cx);
        (void)config().blackboard->get("current_y", cy);

        double dist = std::hypot(cx - ex, cy - ey);
        double dx   = ex - cx;
        double dy   = ey - cy;

        if (dist < 0.01) return BT::NodeStatus::SUCCESS; // 重合，不动

        if (dist > 4.0) {
            // 太远 → 靠近到2.8m
            double target_x = cx + (dx / dist) * (dist - 2.8);
            double target_y = cy + (dy / dist) * (dist - 2.8);
            setOutput("nav_target_x", target_x);
            setOutput("nav_target_y", target_y);
        } else if (dist < 1.2) {
            // 太近 → 后退到1.8m
            double target_x = cx - (dx / dist) * (1.8 - dist);
            double target_y = cy - (dy / dist) * (1.8 - dist);
            setOutput("nav_target_x", target_x);
            setOutput("nav_target_y", target_y);
        }
        // 在1.2~4.0m之间，不动，等自然偏移
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        setOutput("nav_cancel", true);
    }
};
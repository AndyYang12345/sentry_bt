#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cmath>
#include <cstdint>

// ============================================================
// 移动条件节点 — 纯判断
// ============================================================

/*
    @brief 检查是否已到达目标点
    黑板读取: target_x, target_y, current_x, current_y, xy_tolerance
    所有值均为 double，容差默认通过 XML 端口传入
    @return SUCCESS 已到达; FAILURE 未到达
*/
class CheckArrived : public BT::ConditionNode {
public:
    CheckArrived(const std::string& name, const BT::NodeConfig& config)
        : ConditionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("nav_target_x",    "目标X坐标"),
            BT::InputPort<double>("nav_target_y",    "目标Y坐标"),
            BT::InputPort<double>("current_x",   "当前X坐标"),
            BT::InputPort<double>("current_y",   "当前Y坐标"),
            BT::InputPort<double>("XY_TOLERANCE", "到达容差(m), 默认0.2"),
        };
    }

    BT::NodeStatus tick() override {
        double tx, ty, cx, cy, tol;
        if (!getInput("nav_target_x", tx) || !getInput("nav_target_y", ty))
            return BT::NodeStatus::FAILURE;
        if (!getInput("current_x", cx) || !getInput("current_y", cy))
            return BT::NodeStatus::FAILURE;
        // 容差有默认值
        if (!getInput("XY_TOLERANCE", tol))
            tol = 0.2;

        double dist = std::hypot(cx - tx, cy - ty);
        return (dist < tol) ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
};

/*
    @brief 检测受伤 + 无视野 → 开启躲避模式
    黑板读取:
      hp_sentry    (uint16_t) — 当前血量
      target       (uint8_t[9]) — 自瞄可见性数组, target[1-5] 为地面单位
    逻辑:
      - 若当前已在躲避窗口内(计时未到) → SUCCESS (保持躲避)
      - 若 hp < 上一帧 hp 且 无可识别地面单位 → 进入躲避窗口 → SUCCESS
      - 否则 → FAILURE
    参数:
      dodge_duration (double, 默认3.0s) — 单次躲避持续时长, 通过 XML 端口或黑板传入
    @return SUCCESS 应当躲避; FAILURE 无需躲避
*/
class CheckDamage : public BT::ConditionNode {
public:
    CheckDamage(const std::string& name, const BT::NodeConfig& config)
        : ConditionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<uint16_t>("hp_sentry",       "当前血量"),
            BT::InputPort<std::vector<uint8_t>>("target", "自瞄目标可见性数组"),
            BT::InputPort<double>("dodge_duration",    "躲避持续时长(s), 默认3.0"),
        };
    }

    BT::NodeStatus tick() override {
        // ---- 1. 如果还在躲避窗口内 → 继续躲 ----
        if (dodging_) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - dodge_start_).count();
            if (elapsed < dodge_duration_) {
                return BT::NodeStatus::SUCCESS;
            }
            // 窗口到期, 退出躲避
            dodging_ = false;
        }

        // ---- 2. 读取当前数据 ----
        uint16_t hp = 0;
        if (!getInput("hp_sentry", hp))
            return BT::NodeStatus::FAILURE;

        std::vector<uint8_t> target;
        if (!getInput("target", target) || target.size() < 9)
            return BT::NodeStatus::FAILURE;

        double dur = 3.0;
        getInput("dodge_duration", dur);
        dodge_duration_ = dur;

        // ---- 3. 检测受伤 ----
        bool damaged = (hp < last_hp_) && (last_hp_ > 0);

        // ---- 4. 检测是否有可识别地面单位 ----
        bool enemy_visible = false;
        for (size_t i = 1; i <= 5 && i < target.size(); ++i) {
            if (target[i] == 1) { enemy_visible = true; break; }
        }

        // ---- 5. 更新血量记录 ----
        last_hp_ = hp;

        // ---- 6. 触发躲避: 受伤 + 无视野 ----
        if (damaged && !enemy_visible) {
            dodging_   = true;
            dodge_start_ = std::chrono::steady_clock::now();
            return BT::NodeStatus::SUCCESS;
        }

        return BT::NodeStatus::FAILURE;
    }

private:
    uint16_t last_hp_ = 0;
    bool     dodging_ = false;
    double   dodge_duration_ = 3.0;
    std::chrono::steady_clock::time_point dodge_start_;
};
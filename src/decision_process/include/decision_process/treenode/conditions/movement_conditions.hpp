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
#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cstdint>
#include <vector>

using namespace BT;

// ============================================================
// 战斗条件节点 — 纯判断，只读黑板，不修改任何数据
// ============================================================

/*
    @brief 检查弹药是否充足
    @params 读取 ammo (uint16_t) — 当前弹药量
    @return SUCCESS 弹药 > 0; FAILURE 无弹药
*/
class CheckAmmo : public ConditionNode {
public:
    CheckAmmo(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config) {}

    static PortsList providedPorts() {
        return { InputPort<uint16_t>("ammo", "当前弹药数量") };
    }

    NodeStatus tick() override {
        uint16_t ammo = 0;
        if (!getInput("ammo", ammo)) return NodeStatus::FAILURE;
        return (ammo > 0) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
    }
};

/*
    @brief 检查自瞄视野中是否存在可识别目标 (1-8号单位)
    @params 读取 target (uint8_t[9]) — target[0]=模糊, target[1-8]=敌方1-8号
    @return SUCCESS 至少有一个可见单位; FAILURE 无可识别单位
*/
class CheckEnemyVisible : public ConditionNode {
public:
    CheckEnemyVisible(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config) {}

    static PortsList providedPorts() {
        return { InputPort<std::vector<uint8_t>>("target", "自瞄目标可见性数组") };
    }

    NodeStatus tick() override {
        std::vector<uint8_t> target;
        if (!getInput("target", target) || target.size() < 9)
            return NodeStatus::FAILURE;
        for (size_t i = 1; i <= 8; ++i) {
            if (target[i] == 1) return NodeStatus::SUCCESS;
        }
        return NodeStatus::FAILURE;
    }
};
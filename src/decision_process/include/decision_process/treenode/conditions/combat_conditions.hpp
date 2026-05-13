#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cstdint>
#include <vector>

using namespace BT;

// ============================================================
// 战斗条件节点 — 纯判断，只读黑板，不修改任何数据
// ============================================================

/*
    @brief 检查自瞄视野中是否存在可识别的地面单位(1-5号机器人)
    @params 读取 target (uint8_t[9]) — AutoaimToDecision 目标可见性数组
      target[0] 模糊装甲板, target[1-5] 敌方1-5号地面机器人
    @return SUCCESS 至少有一个可见地面单位; FAILURE 无可识别地面单位
*/
class CheckEnemyVisible : public ConditionNode {
public:
    CheckEnemyVisible(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config) {}

    static PortsList providedPorts() {
        return { 
            InputPort<std::vector<uint8_t>>("target", "自瞄目标可见性数组") 
        };
    }

    NodeStatus tick() override {
        std::vector<uint8_t> target;
        if (!getInput("target", target) || target.size() < 9)
            return NodeStatus::FAILURE;
        // 仅检查 1-5 号地面机器人
        for (size_t i = 1; i <= 5; ++i) {
            if (target[i] == 1) return NodeStatus::SUCCESS;
        }
        return NodeStatus::FAILURE;
    }
};
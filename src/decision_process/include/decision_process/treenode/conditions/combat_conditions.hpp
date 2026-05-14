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

/*
    @brief 检查哨兵是否处于死亡状态
    读取 game_period (uint8_t): 0=未开始, 1=死亡/准备, 2=进行中
    @return SUCCESS game_period == 1 (需要复活); FAILURE 不需要复活
*/
class CheckDead : public ConditionNode {
public:
    CheckDead(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config) {}

    static PortsList providedPorts() {
        return {
            InputPort<uint8_t>("game_period", "比赛阶段: 0未开始/1死亡准备/2进行中")
        };
    }

    NodeStatus tick() override {
        uint8_t game_period = 0;
        if (!getInput("game_period", game_period))
            return NodeStatus::FAILURE;
        return (game_period == 1) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
    }
};
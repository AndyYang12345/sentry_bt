#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "interfaces/interfaces.hpp"

using namespace BT;

/*
    @brief 检测自己是否处于低血量状态，如果处于低血量状态返回seccess，否则返回failure
*/
class CheckSelfLowHP : public ConditionNode {
public:
    CheckSelfLowHP(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config) {}

    static PortsList providedPorts() {
        return {
            InputPort<uint16_t>("hp_sentry", "当前血量"),
            InputPort<uint16_t>("HP_RETURN_THRESHOLD", "残血阈值, 默认100")
        };
    }

    NodeStatus tick() override {
        uint16_t hp = 0, threshold = 100;
        if (!getInput("hp_sentry", hp))
            return NodeStatus::FAILURE;
        getInput("HP_RETURN_THRESHOLD", threshold);
        return (hp < threshold) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
    }
};

/*
    @brief 检测自身防御增益是否充足，如果存在60%及以上防御增益时，血量低于HP_RETURN_UNDER_DEFENSE阈值则返回success，否则返回failure
*/
class CheckUnderDefenseLowHP : public ConditionNode {
public:
    CheckUnderDefenseLowHP(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config) {} 
    
    static PortsList providedPorts() {
        return {
            InputPort<uint16_t>("hp_sentry", "当前血量"),
            InputPort<uint16_t>("HP_RETURN_UNDER_DEFENSE", "残血阈值, 默认120"),
            InputPort<uint8_t>("defense_buff", "当前防御增益百分比, 0-100")
        };
    }

    NodeStatus tick() override {
        uint16_t hp = 0, threshold = 120;
        uint8_t defense_buff = 0;
        if (!getInput("hp_sentry", hp))
            return NodeStatus::FAILURE;
        getInput("HP_RETURN_UNDER_DEFENSE", threshold);
        getInput("defense_buff", defense_buff);
        if (defense_buff >= 60) {
            return (hp < threshold) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
        }
        return NodeStatus::FAILURE;
    }
};
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
        return { 
            InputPort<uint16_t>("ammo", "当前弹药数量") 
        };
    }

    NodeStatus tick() override {
        uint16_t ammo = 0;
        if (!getInput("ammo", ammo)) return NodeStatus::FAILURE;
        return (ammo > 0) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
    }
};
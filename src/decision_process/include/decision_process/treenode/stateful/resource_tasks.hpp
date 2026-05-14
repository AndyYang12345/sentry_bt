#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "interfaces/interfaces.hpp"

using namespace BT;


/*
    @brief 重生后检查血量是否恢复，恢复后消息位置0,返回SUCCESS
    黑板读取: hp_sentry (uint16_t) — 当前血量
    黑板写入: confirm_respawn (int) — 清除重生标志 (0)
    @return RUNNING 血量未恢复; SUCCESS 血量已恢复; FAILURE 输入错误
*/
class CheckRespawned : public StatefulActionNode {
public:
    CheckRespawned(const std::string& name, const NodeConfig& config)
        : StatefulActionNode(name, config) {}

    static PortsList providedPorts() {
        return {
            InputPort<uint16_t>("hp_sentry", "当前血量"),
            InputPort<uint16_t>("SENTRY_MAX_HP", "最大血量"),
            OutputPort<uint8_t>("confirm_respawn", "清除重生标志")
        };
    }

    NodeStatus onStart() override {
        return BT::NodeStatus::RUNNING;
    }

    NodeStatus onRunning() override {
        uint16_t hp = 0;
        uint16_t sentry_max_hp = 0;
        if (!getInput("hp_sentry", hp))
            return BT::NodeStatus::FAILURE;

        if (!getInput("SENTRY_MAX_HP", sentry_max_hp))
            return BT::NodeStatus::FAILURE;

        if (hp == sentry_max_hp) {
            setOutput("confirm_respawn", static_cast<uint8_t>(0));  // 清除重生标志
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {
        // 等待回血过程中被打断 (例如进入躲避或攻击)
        // 无需特殊清理, 下一个 tick 会重新从 CheckRespawned 开始等待
    }
};
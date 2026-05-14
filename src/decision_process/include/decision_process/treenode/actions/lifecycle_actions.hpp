#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "interfaces/interfaces.hpp"

using namespace BT;

class Respawn : public SyncActionNode {
public:
    Respawn(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config) {} 

    static PortsList providedPorts() {
        return {
            OutputPort<uint8_t>("confirm_respawn", "触发重生")
        };
    }

    NodeStatus tick() override {
        setOutput("confirm_respawn", static_cast<uint8_t>(1));
        return NodeStatus::SUCCESS;
    }
};
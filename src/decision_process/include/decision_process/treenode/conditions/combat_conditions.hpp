#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "decision_process/include/interfaces/interfaces.hpp"

using namespace BT;

class CheckAmmo: public ConditionNode{
public:
    CheckAmmo(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<uint16_t>("ammo", "当前弹药数量")
        };
    }

    NodeStatus tick() override{
        uint16_t ammo;
        if (ammo > 0){
            return NodeStatus::SUCCESS;
        } else {
            return NodeStatus::FAILURE;
        }
    }
};
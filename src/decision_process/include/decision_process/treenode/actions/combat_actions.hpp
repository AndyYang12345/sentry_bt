#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <string>
#include "decision_process/include/interfaces/interfaces.hpp"

/*
    @brief 战斗，爽！
    @param 无
    @return 成功：直接开火！！
*/
using namespace BT;

class AttackEnemy: public SyncActionNode{
public:
    AttackEnemy(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config){}
    static PortsList providedPorts(){
        return {
            OutputPort<uint8_t>("shoot_mode", "设置开火状态")
        };
    }

    NodeStatus tick() override{
        setOutput("shoot_mode", 1);
        return NodeStatus::SUCCESS;
    }
};
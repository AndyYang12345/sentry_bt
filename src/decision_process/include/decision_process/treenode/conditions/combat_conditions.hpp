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

class CheckEnemyVisible: public ConditionNode{
public:
    CheckEnemyVisible(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<std::vector<bool>>("enemy_visible", "敌人是否可见"),
            OutputPort<bool>("blur_target", "是否存在模糊目标")
        };
    }

    NodeStatus tick() override{
        std::vector<bool> enemy_visible;
        if (getInput("enemy_visible", enemy_visible)){
            if(enemy_visible.empty()){
                setOutput("blur_target", false);
                return NodeStatus::FAILURE;
            }
            if(enemy_visible[0]){
                setOutput("blur_target", true);
                return NodeStatus::SUCCESS;
            }else{
                setOutput("blur_target", false);
            }
            for (size_t i = 1; i < enemy_visible.size(); ++i){
                bool visible = enemy_visible[i];
                return visible ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
            }
        }
        return NodeStatus::FAILURE;
    }
};
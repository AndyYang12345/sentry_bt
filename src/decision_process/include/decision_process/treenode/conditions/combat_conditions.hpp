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

class CheckNotInvincible: public ConditionNode{
public:
    CheckNotInvincible(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<std::vector<uint16_t>>("hp_enemy", "敌人血量"),
            OutputPort<std::vector<uint16_t>>("invincible_enemy", "无敌敌人列表")
        };
    }

    NodeStatus tick() override{
        std::vector<uint16_t> hp_enemy;
        if (getInput("hp_enemy", hp_enemy)){
            if(hp_enemy.empty()){
                return NodeStatus::FAILURE;
            }
            std::vector<uint16_t> invincible_enemy;
            for (size_t i = 0; i < 5; ++i){
                if(hp_enemy[i] == 1001){
                    invincible_enemy.push_back(hp_enemy[i]);
                }
            }
            setOutput("invincible_enemy", invincible_enemy);
            return NodeStatus::SUCCESS;
        }
        return NodeStatus::FAILURE;
    }
};
#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "decision_process/include/interfaces/interfaces.hpp"

using namespace BT;

/*
    @brief 检查弹药是否充足
    @param 当前弹药数量
    @return 成功：弹药充足；失败：弹药不足
*/
class CheckAmmo: public ConditionNode{
public:
    CheckAmmo(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<uint16_t>("ammo", "当前弹药数量"),
            InputPort<uint16_t>("RETURN_THRESHOLD", "返回阈值")
        };
    }

    NodeStatus tick() override{
        uint16_t ammo;
        uint16_t RETURN_THRESHOLD;
        if (getInput("ammo", ammo) && getInput("RETURN_THRESHOLD", RETURN_THRESHOLD)){
            if (ammo > RETURN_THRESHOLD){
                return NodeStatus::SUCCESS;
            } else {
                return NodeStatus::FAILURE;
            }
        }
        return NodeStatus::FAILURE;
    }
};


/*
    @brief 检查敌人是否可见
    @param 敌人可见度列表
    @return 成功：敌人可见；失败：敌人不可见，或者是否存在模糊单位
*/

class CheckEnemyVisible: public ConditionNode{
public:
    CheckEnemyVisible(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<std::vector<bool>>("target", "敌人是否可见"),
            OutputPort<bool>("blur_target", "是否存在模糊目标")
        };
    }

    NodeStatus tick() override{
        std::vector<bool> target;
        if (getInput("target", target)){
            if(target.empty()){
                setOutput("blur_target", false);
                return NodeStatus::FAILURE;
            }
            // 模糊目标可见
            if(target[0]){
                setOutput("blur_target", true);
                return NodeStatus::SUCCESS;
            }else{
                setOutput("blur_target", false);
            }
            // 模糊目标不可见，继续检查具体敌人
            for (size_t i = 1; i < target.size(); ++i){
                if(target[i]){
                    // 存在可见敌人，返回成功
                    return NodeStatus::SUCCESS;
                }
            }
            return NodeStatus::FAILURE;
        }
        return NodeStatus::FAILURE;
    }
};

/*
    @brief 检查敌人是否无敌
    @param 敌人血量列表
    @return 返回无敌敌人列表（bool），成功：检查完成；失败：检查失败
*/
class CheckNotInvincible: public ConditionNode{
public:
    CheckNotInvincible(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<std::vector<uint16_t>>("hp_enemy", "敌人血量"),
            OutputPort<std::vector<bool>>("invincible_enemy", "无敌敌人列表")
        };
    }

    NodeStatus tick() override{
        std::vector<uint16_t> hp_enemy;
        if (getInput("hp_enemy", hp_enemy)){
            if(hp_enemy.empty()){
                return NodeStatus::FAILURE;
            }
            std::vector<bool> invincible_enemy;
            invincible_enemy.push_back(false); 
            // 模糊目标不加入无敌列表,强行对齐目标列表
            // 血量列表从第一个开始就是具体敌人，向后数5个是地面单位
            for (size_t i = 0; i < 5; ++i){
                if(hp_enemy[i] == 1001){
                    invincible_enemy.push_back(true);
                }else{
                    invincible_enemy.push_back(false);
                }
            }
            setOutput("invincible_enemy", invincible_enemy);
            return NodeStatus::SUCCESS;
        }
        return NodeStatus::FAILURE;
    }
};
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

/*
    @brief 选择目标
    @param 模糊目标可见度，敌人可见度列表，无敌敌人列表
    @return 根据是否无敌和是否可见二次筛选目标列表，成功：返回可选目标列表；失败：不存在可选目标
*/
class SelectTarget: public SyncActionNode{
public:
    SelectTarget(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<bool>("blur_target", "是否存在模糊目标"),
            InputPort<std::vector<bool>>("target_list", "敌人是否可见"),
            InputPort<std::vector<bool>>("invincible_enemy_list", "无敌敌人列表"),
            OutputPort<std::vector<bool>>("selected_targets_list", "设置可选中目标列表")
        };
    }

    NodeStatus tick() override{
        bool blur_target;
        std::vector<bool> target_list;
        std::vector<bool> invincible_enemy_list;
        std::vector<bool> selected_targets_list;
        selected_targets_list.push_back(false); // 模糊目标不加入选定列表,强行对齐目标列表
        if (getInput("blur_target", blur_target) && getInput("target_list", target_list) && getInput("invincible_enemy", invincible_enemy)){
            if (blur_target){
                // 模糊目标开火逻辑待定
                return NodeStatus::SUCCESS;
            }
            for (size_t i = 1; i <= invincible_enemy_list.size(); ++i){
                // target_list第一项是模糊目标，后续项对应具体敌人
                if (target_list[i] && !invincible_enemy_list[i]){
                    selected_targets_list.push_back(true);
                    return NodeStatus::SUCCESS;
                }else{
                    selected_targets_list.push_back(false);
                }
            }
            return NodeStatus::FAILURE;
        }
        setOutput("selected_targets_list", std::vector<bool>(target_list.size(), false));
        return NodeStatus::FAILURE;
    }
};
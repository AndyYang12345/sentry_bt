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
            InputPort<std::vector<bool>>("target_list", "敌人是否可见"),
            OutputPort<bool>("blur_target", "是否存在模糊目标")
        };
    }

    NodeStatus tick() override{
        std::vector<bool> target_list;
        if (getInput("target_list", target_list)){
            if(target_list.empty()){
                setOutput("blur_target", false);
                return NodeStatus::FAILURE;
            }
            // 模糊目标可见
            if(target_list[0]){
                setOutput("blur_target", true);
                return NodeStatus::SUCCESS;
            }else{
                setOutput("blur_target", false);
            }
            // 模糊目标不可见，继续检查具体敌人
            for (size_t i = 1; i < target_list.size(); ++i){
                if(target_list[i]){
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
            InputPort<std::vector<uint16_t>>("hp_enemy_list", "敌人血量"),
            OutputPort<std::vector<bool>>("invincible_enemy_list", "无敌敌人列表")
        };
    }

    NodeStatus tick() override{
        std::vector<uint16_t> hp_enemy_list;
        if (getInput("hp_enemy_list", hp_enemy_list)){
            if(hp_enemy_list.empty()){
                return NodeStatus::FAILURE;
            }
            std::vector<bool> invincible_enemy_list;
            invincible_enemy_list.push_back(false); 
            // 模糊目标不加入无敌列表,强行对齐目标列表
            // 血量列表从第一个开始就是具体敌人，向后数5个是地面单位
            for (size_t i = 0; i < 5; ++i){
                if(hp_enemy_list[i] == 1001){
                    invincible_enemy_list.push_back(true);
                }else{
                    invincible_enemy_list.push_back(false);
                }
            }
            setOutput("invincible_enemy_list", invincible_enemy_list);
            return NodeStatus::SUCCESS;
        }
        return NodeStatus::FAILURE;
    }
};

class CheckDistance: public ConditionNode{
public:
    CheckDistance(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
    static PortsList providedPorts(){
        return {
            InputPort<std::vector<bool>>("selected_targets_list", "可选中目标列表"),
            InputPort<std::vector<float>>("target_distance_list", "目标距离列表"),
            InputPort<std::vector<float>>("target_polar_r_list", "目标极坐标半径列表"),
            InputPort<std::vector<float>>("target_polar_theta_list", "目标极坐标角度列表"),
            OutputPort<int>("target", "选定目标索引"),
            OutputPort<float>("target_polar_r", "选定目标极坐标半径"),
            OutputPort<float>("target_polar_theta", "选定目标极坐标角度")
        };
    }

    NodeStatus tick() override{
        std::vector<bool> selected_targets_list;
        std::vector<float> target_distance_list;
        std::vector<float> target_polar_r_list;
        std::vector<float> target_polar_theta_list;
        if (getInput("selected_targets_list", selected_targets_list) && getInput("target_distance_list", target_distance_list) &&
            getInput("target_polar_r_list", target_polar_r_list) && getInput("target_polar_theta_list", target_polar_theta_list)){
            if (selected_targets_list.empty() || target_distance_list.empty() || target_polar_r_list.empty() || target_polar_theta_list.empty()){
                return NodeStatus::FAILURE;
            }
            int selected_target_index = -1;
            float min_distance = std::numeric_limits<float>::max();
            for (size_t i = 1; i <= selected_targets_list.size(); ++i){
                if (selected_targets_list[i]){
                    // 选定目标中距离最近的一个
                    if (target_distance_list[i] < min_distance){
                        min_distance = target_distance_list[i];
                        selected_target_index = static_cast<int>(i);
                    }
                }
            }
            if (selected_target_index != -1){
                setOutput("target", selected_target_index);
                setOutput("target_polar_r", target_polar_r_list[selected_target_index]);
                setOutput("target_polar_theta", target_polar_theta_list[selected_target_index]);
                return NodeStatus::SUCCESS;
            } else {
                return NodeStatus::FAILURE;
            }
        }
        return NodeStatus::FAILURE;
    }
};
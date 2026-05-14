#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cstdint>
#include <vector>
#include <limits>

using namespace BT;

// ============================================================
// 战斗动作节点
// ============================================================

/*
    @brief 综合筛选 + 选最优目标（1-8号单位均考虑）
    @params 黑板读取 (均来自订阅回调更新):
      target          (uint8_t[9])  — 自瞄可见性, target[0]=模糊, target[1-8]=敌方1-8号
      hp_enemy        (uint16_t[8]) — 敌方血量, [0-4]=地面1-5号, [5]=前哨站, [6]=基地, [7]=8号
      target_distance (double[9])   — 目标距离
      target_polar_angle (double[9])— 目标极角
    筛选条件: 可见(target[i]==1) 且 (非无敌(hp!=1001) 或 建筑未摧毁(hp!=0))
    选择策略: 距离最近(>0)
    黑板写入:
      target_id  (uint8_t) — 选定目标编号(1-8)
      decide_yaw (float)   — 选定目标的极角值, 用于自瞄
    @return SUCCESS 选中目标; FAILURE 无有效目标
*/
class SelectTarget : public SyncActionNode {
public:
    SelectTarget(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config) {}

    static PortsList providedPorts() {
        return {
            InputPort<std::vector<uint8_t>>("target", "自瞄目标可见性数组"),
            InputPort<std::vector<uint16_t>>("hp_enemy", "敌方血量数组"),
            InputPort<std::vector<double>>("target_distance", "目标距离数组"),
            InputPort<std::vector<double>>("target_polar_angle", "目标极角数组"),
            OutputPort<uint8_t>("target_id", "选定的目标编号(1-8)"),
            OutputPort<double>("decide_yaw", "选定目标的极角, 传给自瞄模块")
        };
    }

    NodeStatus tick() override {
        std::vector<uint8_t>  target;
        std::vector<uint16_t> hp_enemy;
        std::vector<double>   target_distance;
        std::vector<double>   target_polar_angle;

        if (!getInput("target", target) || target.size() < 9) return NodeStatus::FAILURE;
        if (!getInput("hp_enemy", hp_enemy) || hp_enemy.size() < 8) return NodeStatus::FAILURE;
        if (!getInput("target_distance", target_distance) || target_distance.size() < 9) return NodeStatus::FAILURE;
        if (!getInput("target_polar_angle", target_polar_angle) || target_polar_angle.size() < 9) return NodeStatus::FAILURE;

        int    best_id   = -1;
        double best_dist = std::numeric_limits<double>::max();

        for (size_t i = 1; i <= 8; ++i) {
            // 1) 自瞄可见
            if (target[i] != 1) continue;

            // 2) hp_enemy[i-1] 对应的血量
            uint16_t hp = hp_enemy[i - 1];

            // 前哨站(6号,idx=5) / 基地(7号,idx=6) 只要 hp>0 即可打
            if (i == 6 || i == 7) {
                if (hp == 0) continue;                // 建筑已摧毁
            } else {
                if (hp == 1001) continue;              // 地面单位无敌
                if (hp == 0)    continue;              // 已摧毁
            }

            // 3) 距离有效且最近
            if (target_distance[i] > 0 && target_distance[i] < best_dist) {
                best_dist = target_distance[i];
                best_id   = static_cast<int>(i);
            }
        }

        if (best_id > 0) {
            setOutput("target_id", static_cast<uint8_t>(best_id));
            setOutput("decide_yaw", static_cast<double>(target_polar_angle[best_id]));
            return NodeStatus::SUCCESS;
        }
        return NodeStatus::FAILURE;
    }
};

/*
    @brief 发送开火指令
    黑板写入: shoot_mode = 1
    @return SUCCESS
*/
class AttackEnemy : public SyncActionNode {
public:
    AttackEnemy(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config) {}

    static PortsList providedPorts() {
        return { OutputPort<uint8_t>("shoot_mode", "开火状态 0停火/1开火") };
    }

    NodeStatus tick() override {
        setOutput("shoot_mode", static_cast<uint8_t>(1));
        return NodeStatus::SUCCESS;
    }
};
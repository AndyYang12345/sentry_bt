#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "interfaces/interfaces.hpp"
#include <cstdint>

using namespace BT;

// ============================================================
// 弹药管理节点 — 比赛每分钟 +100 待领取弹药, 到补给点领取
// ============================================================

/*
    @brief 每分钟自动积累待领取弹药
    比赛开始后(比赛进行阶段 game_period==2), 每经过完整的一分钟,
    待领取弹药量 +100 (最多 7 分钟, 累计 700)

    时间基准: ReceiveData.time 是比赛阶段剩余时间 (420→0 倒计时)
             已流逝秒数 = 420 - time
             当前分钟数 = 已流逝秒数 / 60

    内部状态:
      last_elapsed_minute_ — 追踪上一次处理的分钟编号, 防止重复累加
      accumulated_ammo_    — 累计待领取弹药总量

    黑板读取:
      time         (uint16_t) — 比赛阶段剩余时间 (游戏阶段: 420→0)
      game_period  (uint8_t)  — 0=未开始, 1=准备, 2=进行中
    黑板写入:
      ammo_to_collect (uint16_t) — 当前累计待领取弹药量

    @return SUCCESS (始终, 仅更新黑板)
*/
class CalculateAmmoToCollect : public SyncActionNode {
public:
    CalculateAmmoToCollect(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config),
          last_elapsed_minute_(-1),
          accumulated_ammo_(0) {}

    static PortsList providedPorts() {
        return {
            InputPort<uint16_t>("time",        "比赛阶段剩余时间(游戏阶段420→0倒计时)"),
            InputPort<uint8_t>("game_period",  "比赛阶段: 0未开始/1准备/2进行中"),
            OutputPort<uint16_t>("ammo_to_collect", "累计待领取弹药量")
        };
    }

    NodeStatus tick() override {
        uint16_t time        = 0;
        uint8_t  game_period = 0;

        if (!getInput("time", time))
            return NodeStatus::FAILURE;
        getInput("game_period", game_period);

        // ---- 非比赛进行阶段: 不累加, 但保留已积累的值 ----
        if (game_period != 2) {
            setOutput("ammo_to_collect", accumulated_ammo_);
            return NodeStatus::SUCCESS;
        }

        // ---- 比赛进行阶段: 按分钟累加 ----
        // time 从 420 倒计时到 0, 已流逝 = 420 - time
        uint16_t elapsed_seconds = 420 - time;
        int      current_minute  = elapsed_seconds / 60;   // 整数除法, 0~7

        // 首次进入: 初始化追踪值 (不触发累加)
        if (last_elapsed_minute_ < 0) {
            last_elapsed_minute_ = current_minute;
            setOutput("ammo_to_collect", accumulated_ammo_);
            return NodeStatus::SUCCESS;
        }

        // 检测分钟边界: 每跨过一个完整分钟, +100/分钟
        if (current_minute > last_elapsed_minute_) {
            int minutes_passed = current_minute - last_elapsed_minute_;
            accumulated_ammo_  = static_cast<uint16_t>(
                accumulated_ammo_ + minutes_passed * 100);
            last_elapsed_minute_ = current_minute;
        }

        // 处理倒计时归零的特殊情况 (time==0 时 elapsed=420, minute=7)
        // 此时已进入第 7 分钟, 正常累加即可

        setOutput("ammo_to_collect", accumulated_ammo_);
        return NodeStatus::SUCCESS;
    }

private:
    int      last_elapsed_minute_ = -1;   // 上次处理的分钟编号
    uint16_t accumulated_ammo_    = 0;    // 累计待领取弹药
};


/*
    @brief 到达补给点后领取弹药
    将待领取弹药量发放至哨兵剩余子弹量, 同时清空待领取弹药量

    注意: 在真实比赛环境中, ammo 的实际值由裁判系统通过
          /serial_receive_data 话题下发。本节点写入的 ammo 值
          可能被下一次订阅回调覆盖。若需离线测试, 可直接使用
          本节点的输出; 若对接裁判系统, 游戏服务器会在哨兵
          到达补给点时自动增加 ammo, 本节点仅负责清空
          ammo_to_collect 以防止重复领取。

    黑板读取:
      ammo            (uint16_t) — 当前剩余子弹量
      ammo_to_collect (uint16_t) — 待领取弹药量
    黑板写入:
      ammo            (uint16_t) — 领取后子弹量 (ammo + ammo_to_collect)
      ammo_to_collect (uint16_t) — 清空为 0

    @return SUCCESS 领取成功; FAILURE 无可领取弹药
*/
class CollectAmmo : public SyncActionNode {
public:
    CollectAmmo(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config) {}

    static PortsList providedPorts() {
        return {
            InputPort<uint16_t>("ammo",            "当前剩余子弹量"),
            InputPort<uint16_t>("ammo_to_collect", "待领取弹药量"),
            OutputPort<uint16_t>("ammo",           "领取后子弹量"),
            OutputPort<uint16_t>("ammo_to_collect", "清空为0")
        };
    }

    NodeStatus tick() override {
        uint16_t ammo            = 0;
        uint16_t ammo_to_collect = 0;

        if (!getInput("ammo", ammo))
            return NodeStatus::FAILURE;
        if (!getInput("ammo_to_collect", ammo_to_collect))
            return NodeStatus::FAILURE;

        // 无可领取弹药
        if (ammo_to_collect == 0)
            return NodeStatus::FAILURE;

        // 发放待领取弹药 → 剩余子弹量
        uint16_t new_ammo = ammo + ammo_to_collect;

        setOutput("ammo",            new_ammo);
        setOutput("ammo_to_collect", static_cast<uint16_t>(0));

        return NodeStatus::SUCCESS;
    }
};
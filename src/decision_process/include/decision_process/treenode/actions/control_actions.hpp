#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "interfaces/interfaces.hpp"

using namespace BT;

/*
    @brief 基类：设置底盘旋转模式（持续，等待外部打断）
    spin_mode: 0=停止, 1=低速, 2=中速, 3=高速
    子类只需构造时传对应值即可自动设置
*/
class Spin : public BT::SyncActionNode {
public:
    Spin(const std::string& name, const NodeConfig& config, int mode = 0)
        : SyncActionNode(name, config), spin_mode_(mode) {}

    static PortsList providedPorts() {
        return {
            OutputPort<uint8_t>("spin_mode", "0=停止, 1=低速, 2=中速, 3=高速")
        };
    }

    NodeStatus tick() override {
        setOutput("spin_mode", static_cast<uint8_t>(spin_mode_));
        return NodeStatus::RUNNING;
    }

protected:
    int spin_mode_ = 0;
};

// ---------- 子类：构造函数中硬编码速度，不需要额外传参 ----------

class SpinHalt : public Spin {
public:
    SpinHalt(const std::string& name, const NodeConfig& config)
        : Spin(name, config, 0) {}
};

class SpinLow : public Spin {
public:
    SpinLow(const std::string& name, const NodeConfig& config)
        : Spin(name, config, 1) {}
};

class SpinMid : public Spin {
public:
    SpinMid(const std::string& name, const NodeConfig& config)
        : Spin(name, config, 2) {}
};

class SpinHigh : public Spin {
public:
    SpinHigh(const std::string& name, const NodeConfig& config)
        : Spin(name, config, 3) {}
};


/*
    @brief 旋转云台
*/
class SetTripod : public BT::SyncActionNode {
public:
    SetTripod(const std::string& name, const NodeConfig& config, int mode = 0)
        : SyncActionNode(name, config), tripod_mode_(mode) {}

    static PortsList providedPorts() {
        return {
            OutputPort<uint8_t>("tripod_mode", "0=关闭, 1=开启")
        };
    }

    NodeStatus tick() override {
        setOutput("tripod_mode", static_cast<uint8_t>(tripod_mode_));
        return NodeStatus::SUCCESS;
    }
protected:
    int tripod_mode_ = 0;
};

// ---------- 子类：构造函数中硬编码模式 ----------
class SetTripodOff : public SetTripod {
public:
    SetTripodOff(const std::string& name, const NodeConfig& config)
        : SetTripod(name, config, 0) {}
};

class SetTripodOn : public SetTripod {
public:
    SetTripodOn(const std::string& name, const NodeConfig& config)
        : SetTripod(name, config, 1) {}
};

/*
    @brief 设置姿态
*/
class SetPosture : public BT::SyncActionNode {
public:
    SetPosture(const std::string& name, const NodeConfig& config, int mode = 0)
        : SyncActionNode(name, config), posture_(mode) {}

    static PortsList providedPorts() {
        return {
            OutputPort<uint8_t>("posture", "姿态控制,1进攻姿态 2防御姿态 3移动姿态")
        };
    }

    NodeStatus tick() override {
        setOutput("posture", static_cast<uint8_t>(posture_));
        return NodeStatus::SUCCESS;
    }
protected:
    int posture_ = 3; // 默认移动姿态
};

// ---------- 子类：构造函数中硬编码姿态模式 ----------
class SetPostureAttack : public SetPosture {
public:
    SetPostureAttack(const std::string& name, const NodeConfig& config)
        : SetPosture(name, config, 1) {}
};

class SetPostureDefense : public SetPosture {
public:
    SetPostureDefense(const std::string& name, const NodeConfig& config)
        : SetPosture(name, config, 2) {}
};

class SetPostureMove : public SetPosture {
public:
    SetPostureMove(const std::string& name, const NodeConfig& config)
        : SetPosture(name, config, 3) {}
};
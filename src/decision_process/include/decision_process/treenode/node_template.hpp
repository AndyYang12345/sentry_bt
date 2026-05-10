#pragma once

#include <behaviortree_cpp/bt_factory.h>
#include <string>
#include <chrono>
#include <thread>

using namespace BT;

class SayMsg: public SyncActionNode{
public:
    SayMsg(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config){}
        
    static PortsList providedPorts(){
        return {
            InputPort<std::string>("message", "msg")
        };
    }

    NodeStatus tick() override{
        std::string msg;
        if (!getInput("message", msg)){
            std::cout << "[SayMsg] Missing input: message" << std::endl;
            return NodeStatus::FAILURE;
        }
        std::cout << "[SayMsg] " << msg << std::endl;
        return NodeStatus::SUCCESS;
    }
};

class CalculateSum: public SyncActionNode{
public:
    CalculateSum(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config){}
        
    static PortsList providedPorts(){
        return {
            InputPort<int>("a", "first number"),
            InputPort<int>("b", "second number"),
            OutputPort<int>("sum", "result of a + b")
        };
    }

    NodeStatus tick() override{
        int a, b;
        if (!getInput("a", a) || !getInput("b", b)){
            std::cout << "[CalculateSum] Missing input: a or b" << std::endl;
            return NodeStatus::FAILURE;
        }
        int sum = a + b;
        setOutput("sum", sum);
        std::cout << "[CalculateSum] " << a << " + " << b << " = " << sum << std::endl;
        return NodeStatus::SUCCESS;
    }
};

class IsGreaterThan: public ConditionNode{
public:
    IsGreaterThan(const std::string& name, const NodeConfig& config)
        : ConditionNode(name, config){}
        
    static PortsList providedPorts(){
        return {
            InputPort<int>("value", "需要比较的值"),
            InputPort<int>("threshold", "比较的阈值")
        };
    }

    NodeStatus tick() override{
        int value, threshold;
        if (!getInput("value", value) || !getInput("threshold", threshold)){
            std::cout << "[IsGreaterThan] Missing input: value or threshold" << std::endl;
            return NodeStatus::FAILURE;
        }
        std::cout << "[IsGreaterThan] " << value << " > " << threshold << " ? " << (value > threshold) << std::endl;
        return (value > threshold) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
    }
};

#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include "decision_process/include/interfaces/interfaces.hpp"

using namespace BT;

class GoToPoint : public SyncActionNode {
public:
    GoToPoint(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config) {}
        
    static PortsList providedPorts() {
        return {
            InputPort<double>("target_x", "目标点的X坐标"),
            InputPort<double>("target_y", "目标点的Y坐标"),
            InputPort<double>("target_yaw", "目标点的朝向"),
            InputPort<double>("current_x", "当前X坐标"),
            InputPort<double>("current_y", "当前Y坐标"),
            InputPort<double>("current_yaw", "当前朝向"),
            InputPort<double>("x_tolerance", "X坐标容差"),
            InputPort<double>("y_tolerance", "Y坐标容差"),
            InputPort<double>("yaw_tolerance", "朝向容差"),
            OutputPort<bool>("arrived", "是否到达目标点")
        };
    }

    NodeStatus tick() override {
        double target_x, target_y, target_yaw;
        if (!getInput("target_x", target_x) || !getInput("target_y", target_y) || !getInput("target_yaw", target_yaw)) {
            return NodeStatus::FAILURE;
        }
        
        bool arrived = checkArrival(target_x, target_y, target_yaw);
        setOutput("arrived", arrived);

        return arrived ? NodeStatus::SUCCESS : NodeStatus::RUNNING;
    }
private:
    bool checkArrival(double target_x, double target_y, double target_yaw) {
        double current_x, current_y, current_yaw;
        double x_tolerance, y_tolerance, yaw_tolerance;
        if (!getInput("current_x", current_x) || !getInput("current_y", current_y) || !getInput("current_yaw", current_yaw) || !getInput("x_tolerance", x_tolerance) || !getInput("y_tolerance", y_tolerance) || !getInput("yaw_tolerance", yaw_tolerance)) {
            return false;
        }
        if(fabs(current_x - target_x) < x_tolerance && fabs(current_y - target_y) < y_tolerance && fabs(current_yaw - target_yaw) < yaw_tolerance) {
            return true;
        }
        return false;
    }
};
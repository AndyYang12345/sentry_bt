#pragma once

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <std_msgs/msg/string.hpp>
#include <mutex>

using namespace BT;

class RosSubscriberNode : public SyncActionNode {
public:
    RosSubscriberNode(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config), has_new_data_(false) {
            node_ = rclcpp::Node::make_shared("subscriber_node");
            subscriber_ = node_->create_subscription<std_msgs::msg::String>(
                "chatter", 10, [this](const std_msgs::msg::String::SharedPtr msg) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    latest_data_ = msg->data;
                    has_new_data_ = true;
                });
        }

    static PortsList providedPorts() {
        return {
            OutputPort<std::string>("msg", "latest message from ROS topic"),
            OutputPort<bool>("has_new_data", "flag indicating if new data has been received")
        };
    }

    NodeStatus tick() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (has_new_data_) {
            setOutput("msg", latest_data_);
            setOutput("has_new_data", true);
            has_new_data_ = false;
            return NodeStatus::SUCCESS;
        } else {
            setOutput("has_new_data", false);
            return NodeStatus::FAILURE;
        }
    }

private:
    std::mutex mutex_;
    bool has_new_data_;
    std::string latest_data_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscriber_;
    rclcpp::Node::SharedPtr node_;
};
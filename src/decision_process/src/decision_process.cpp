#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/loggers/bt_cout_logger.h>
#include <behaviortree_cpp/loggers/bt_minitrace_logger.h>

#include "decision_process/treenode/node_template.hpp"

#include <filesystem>
#include <memory>
#include <string>

void node_init(const rclcpp::Node::SharedPtr& node)
{
    node->declare_parameter("map_width", 10.0);
    node->declare_parameter("map_length", 10.0);
    node->declare_parameter("max_rotate_speed", 1.0);
    node->declare_parameter("defence_hp_threshold", 30);
    node->declare_parameter("spin_low_speed", 0.5);
    node->declare_parameter("spin_mid_speed", 1.0);
    node->declare_parameter("spin_high_speed", 1.5);
}

void blackboard_init(BT::Blackboard::Ptr blackboard, const rclcpp::Node::SharedPtr& node)
{
    blackboard->set("node", node);
    blackboard->set("map_width",   node->get_parameter("map_width").as_double());
    blackboard->set("map_length",  node->get_parameter("map_length").as_double());
    blackboard->set("max_rotate_speed", node->get_parameter("max_rotate_speed").as_double());
    blackboard->set("defence_hp_threshold", node->get_parameter("defence_hp_threshold").as_int());
    blackboard->set("spin_low_speed",  node->get_parameter("spin_low_speed").as_double());
    blackboard->set("spin_mid_speed",  node->get_parameter("spin_mid_speed").as_double());
    blackboard->set("spin_high_speed", node->get_parameter("spin_high_speed").as_double());
}

void run_basic_mode(const rclcpp::Node::SharedPtr& node)
{
    // 1. 创建行为树工厂
    BT::BehaviorTreeFactory factory;
    
    // 2. 注册节点类型
    // factory.registerNodeType<SayMsg>("SayMsg");
    // factory.registerNodeType<CalculateSum>("CalculateSum");
    // factory.registerNodeType<IsGreaterThan>("IsGreaterThan");
    
    
    // 3. 创建黑板并设置共享数据
    node_init(node);
    auto blackboard = BT::Blackboard::create();
    blackboard_init(blackboard, node);

    
    // 4. 从 XML 加载树
    std::string xml_path = "src/decision_process/xml/task_simple.xml";
    auto tree = factory.createTreeFromFile(xml_path, blackboard);
    
    // 5. 添加日志（可选）
    // BT::StdOutLogger logger(&tree);
    
    // 6. 主循环
    rclcpp::Rate rate(10);  // 10Hz
    while (rclcpp::ok()) {
        tree.tickOnce();
        rclcpp::spin_some(node);
        rate.sleep();
    }
}

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    
    auto node = rclcpp::Node::make_shared("decision_process_node");

    run_basic_mode(node);
    
    rclcpp::shutdown();
    return 0;
}
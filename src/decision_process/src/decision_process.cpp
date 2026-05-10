#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/loggers/bt_cout_logger.h>
#include <behaviortree_cpp/loggers/bt_minitrace_logger.h>

#include "decision_process/treenode/node_template.hpp"

#include <filesystem>
#include <memory>
#include <string>

void run_basic_mode(const rclcpp::Node::SharedPtr& node)
{
    // 1. 创建行为树工厂
    BT::BehaviorTreeFactory factory;
    
    // 2. 注册节点类型
    // factory.registerNodeType<SayMsg>("SayMsg");
    // factory.registerNodeType<CalculateSum>("CalculateSum");
    // factory.registerNodeType<IsGreaterThan>("IsGreaterThan");
    
    
    // 3. 创建黑板并设置共享数据
    auto blackboard = BT::Blackboard::create();
    blackboard->set("node", node);
    blackboard->set("map_width",   node->get_parameter("map_width").as_double());
    blackboard->set("map_length",  node->get_parameter("map_length").as_double());
    blackboard->set("max_rotate_speed", node->get_parameter("max_rotate_speed").as_double());
    blackboard->set("defence_hp_threshold", node->get_parameter("defence_hp_threshold").as_int());
    blackboard->set("spin_low_speed",  node->get_parameter("spin_low_speed").as_double());
    blackboard->set("spin_mid_speed",  node->get_parameter("spin_mid_speed").as_double());
    blackboard->set("spin_high_speed", node->get_parameter("spin_high_speed").as_double());

    
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
    
    // 参数声明
    node->declare_parameter<float>("map_width", 15.0);
    node->declare_parameter<float>("map_length", 28.0);
    node->declare_parameter<float>("max_rotate_speed", 3.14);
    node->declare_parameter<int>("defence_buff_threshold", 50);
    node->declare_parameter<int>("enemy_hp_threshold", 50);
    node->declare_parameter<float>("spin_low_speed", 1.0);
    node->declare_parameter<float>("spin_mid_speed", 2.0);
    node->declare_parameter<float>("spin_high_speed", 3.0);

    run_basic_mode(node);
    
    rclcpp::shutdown();
    return 0;
}
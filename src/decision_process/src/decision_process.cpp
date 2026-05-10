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
    factory.registerNodeType<SayMsg>("SayMsg");
    factory.registerNodeType<CalculateSum>("CalculateSum");
    factory.registerNodeType<IsGreaterThan>("IsGreaterThan");
    
    // 注册标准 Script 节点
    // factory.registerNodeType<BT::ScriptNode>("Script");
    
    // 3. 创建黑板并设置共享数据
    auto blackboard = BT::Blackboard::create();
    blackboard->set("node", node);
    blackboard->set<std::string>("greeting_message", "Hello from Blackboard!");
    blackboard->set<int>("counter", 0);
    
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
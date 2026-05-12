#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/loggers/bt_cout_logger.h>

#include "decision_process/treenode/node_template.hpp"
#include "decision_process/treenode/conditions/combat_conditions.hpp"
#include "decision_process/treenode/conditions/movement_conditions.hpp"
#include "decision_process/treenode/actions/combat_actions.hpp"
#include "decision_process/treenode/stateful/navigation_tasks.hpp"
#include "interfaces/interfaces.hpp"

#include <memory>
#include <string>
#include <vector>

// ============================================================
// ROS 2 参数声明
// ============================================================
void node_init(const rclcpp::Node::SharedPtr& node)
{
    // 地图
    node->declare_parameter("map_width", 10.0);
    node->declare_parameter("map_height", 8.0);
    // 运动控制
    node->declare_parameter("max_rotate_speed", 1.0);
    node->declare_parameter("spin_low_speed",  0.5);
    node->declare_parameter("spin_mid_speed",  1.0);
    node->declare_parameter("spin_high_speed", 1.5);
    // 战斗
    node->declare_parameter("defence_hp_threshold", 160);
    node->declare_parameter("ammo_threshold", 0);
}
// ...existing code...
void blackboard_init(BT::Blackboard::Ptr blackboard, const rclcpp::Node::SharedPtr& node)
{
    blackboard->set("node", node);
    blackboard->set("map_width",          node->get_parameter("map_width").as_double());
    blackboard->set("map_height",         node->get_parameter("map_height").as_double());
    blackboard->set("max_rotate_speed",   node->get_parameter("max_rotate_speed").as_double());
    blackboard->set("spin_low_speed",     node->get_parameter("spin_low_speed").as_double());
    blackboard->set("spin_mid_speed",     node->get_parameter("spin_mid_speed").as_double());
    blackboard->set("spin_high_speed",    node->get_parameter("spin_high_speed").as_double());
    blackboard->set("defence_hp_threshold", node->get_parameter("defence_hp_threshold").as_int());
    blackboard->set("ammo_threshold",     node->get_parameter("ammo_threshold").as_int());

    // ============================================================
    // 话题数据初始值 (后续由订阅回调实时更新)
    // ============================================================
    // --- /serial_receive_data (ReceiveData) ---
    blackboard->set("game_period",    static_cast<uint8_t>(0));
    blackboard->set("time",           static_cast<uint16_t>(0));
    blackboard->set("hp_sentry",      static_cast<uint16_t>(400));
    blackboard->set("defence_buff",   static_cast<uint8_t>(0));
    blackboard->set("color",          static_cast<uint8_t>(0));
    blackboard->set("ammo",           static_cast<uint16_t>(0));
    blackboard->set("hp_outpost",     static_cast<uint16_t>(0));
    blackboard->set("hp_base",        static_cast<uint16_t>(0));
    blackboard->set("hp_enemy",       std::vector<uint16_t>(8, 0));
    blackboard->set("radar_data",     std::vector<float>(12, 0.0f));

    // --- /autoaim_to_decision (AutoaimToDecision) ---
    blackboard->set("target",            std::vector<uint8_t>(9, 0));
    blackboard->set("target_distance",   std::vector<double>(9, 0.0));
    blackboard->set("target_polar_r",    std::vector<double>(9, 0.0));
    blackboard->set("target_polar_angle",std::vector<double>(9, 0.0));

    // ============================================================
    // BT 决策产出 (默认值)
    // ============================================================
    // --- /serial_send_data ---
    blackboard->set("posture",         static_cast<uint8_t>(3));  // 默认移动姿态
    blackboard->set("confirm_respawn", static_cast<uint8_t>(0));
    blackboard->set("tripod_mode",     static_cast<uint8_t>(0));
    blackboard->set("spin_mode",       static_cast<uint8_t>(0));
    blackboard->set("shoot_mode",      static_cast<uint8_t>(0));
    // --- /decision_to_autoaim ---
    blackboard->set("target_id",       static_cast<uint8_t>(0));
    blackboard->set("decide_yaw",      0.0f);

    // --- 导航状态 ---
    blackboard->set("nav_target_x", 0.0);
    blackboard->set("nav_target_y", 0.0);
    blackboard->set("nav_cancel",   false);
}

// ============================================================
// 主运行模式
// ============================================================
void run_basic_mode(const rclcpp::Node::SharedPtr& node)
{
    using namespace sentry_interfaces::msg;

    // 1. 创建行为树工厂
    BT::BehaviorTreeFactory factory;

    // 2. 注册节点类型
    factory.registerNodeType<CheckAmmo>("CheckAmmo");
    factory.registerNodeType<CheckEnemyVisible>("CheckEnemyVisible");
    factory.registerNodeType<SelectTarget>("SelectTarget");
    factory.registerNodeType<AttackEnemy>("AttackEnemy");
    factory.registerNodeType<CheckArrived>("CheckArrived");
    factory.registerNodeType<GoToPoint>("GoToPoint");
    factory.registerNodeType<Patrol>("Patrol");
    factory.registerNodeType<Dodge>("Dodge");
    factory.registerNodeType<ChaseEnemy>("ChaseEnemy");
    factory.registerNodeType<KeepDistance>("KeepDistance");

    // 3. 创建黑板并初始化
    node_init(node);
    auto blackboard = BT::Blackboard::create();
    blackboard_init(blackboard, node);

    // 4. 设置订阅 (数据写入黑板)
    auto sub_receive = node->create_subscription<ReceiveData>(
        "/serial_receive_data", 10,
        [blackboard](ReceiveData::SharedPtr msg) {
            blackboard->set("game_period",  msg->game_period);
            blackboard->set("time",         msg->time);
            blackboard->set("hp_sentry",    msg->hp_sentry);
            blackboard->set("defence_buff", msg->defence_buff);
            blackboard->set("color",        msg->color);
            blackboard->set("ammo",         msg->ammo);
            blackboard->set("hp_outpost",   msg->hp_outpost);
            blackboard->set("hp_base",      msg->hp_base);
            blackboard->set("hp_enemy",     msg->hp_enemy);
            blackboard->set("radar_data",   msg->radar_data);
        });

    auto sub_autoaim = node->create_subscription<AutoaimToDecision>(
        "/autoaim_to_decision", 10,
        [blackboard](AutoaimToDecision::SharedPtr msg) {
            blackboard->set("target",            msg->target);
            blackboard->set("target_distance",   msg->target_distance);
            blackboard->set("target_polar_r",    msg->target_polar_r);
            blackboard->set("target_polar_angle", msg->target_polar_angle);
        });

    // 5. 设置发布 (从黑板读取)
    auto pub_send    = node->create_publisher<DecisionSendData>("/serial_send_data", 10);
    auto pub_autoaim = node->create_publisher<DecisionToAutoaim>("/decision_to_autoaim", 10);

    // 6. 加载行为树
    std::string xml_path = "src/decision_process/xml/task_simple.xml";
    auto tree = factory.createTreeFromFile(xml_path, blackboard);

    // 7. 主循环
    rclcpp::Rate rate(100);  // 100Hz
    while (rclcpp::ok()) {
        tree.tickOnce();
        rclcpp::spin_some(node);

        // --- 发布到 /serial_send_data ---
        {
            DecisionSendData msg;
            (void)blackboard->get("posture",         msg.posture);
            (void)blackboard->get("confirm_respawn", msg.confirm_respawn);
            (void)blackboard->get("tripod_mode",     msg.tripod_mode);
            (void)blackboard->get("spin_mode",       msg.spin_mode);
            (void)blackboard->get("shoot_mode",      msg.shoot_mode);
            pub_send->publish(msg);
        }

        // --- 发布到 /decision_to_autoaim ---
        {
            DecisionToAutoaim msg;
            uint8_t target_id  = 0;
            float   decide_yaw = 0.0f;
            uint8_t color      = 0;
            (void)blackboard->get("target_id",  target_id);
            (void)blackboard->get("decide_yaw", decide_yaw);
            (void)blackboard->get("color",      color);

            msg.target    = target_id;
            msg.decide_yaw = decide_yaw;
            // ReceiveData.color: 0红1蓝 → DecisionToAutoaim.color: 0红2蓝
            msg.color     = (color == 0) ? static_cast<uint8_t>(0) : static_cast<uint8_t>(2);

            pub_autoaim->publish(msg);
        }

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
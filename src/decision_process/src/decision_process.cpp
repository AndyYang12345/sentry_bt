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
// ROS 2 参数声明 (对应 config/ 下各 yaml 文件)
// ============================================================
void node_init(const rclcpp::Node::SharedPtr& node)
{
    // --- map_params.yaml ---
    node->declare_parameter("map_width",      15.0);
    node->declare_parameter("map_height",      8.0);
    node->declare_parameter("xy_tolerance",    0.2);

    // --- movement_params.yaml ---
    node->declare_parameter("max_rotate_speed", 3.14);
    node->declare_parameter("spin_low_speed",   1.0);
    node->declare_parameter("spin_mid_speed",   2.0);
    node->declare_parameter("spin_high_speed",  3.0);

    // --- combat_params.yaml ---
    node->declare_parameter("ammo_threshold",       0);
    node->declare_parameter("defence_hp_threshold", 160);
    node->declare_parameter("dodge_radius",          2.0);
    node->declare_parameter("min_dist",              1.2);
    node->declare_parameter("max_dist",              4.0);
    node->declare_parameter("close_to",              2.8);
    node->declare_parameter("back_to",               1.8);

    // --- decision_params.yaml ---
    node->declare_parameter("default_posture",         3);
    node->declare_parameter("default_confirm_respawn", 0);
    node->declare_parameter("default_tripod_mode",     0);
    node->declare_parameter("default_spin_mode",       0);
    node->declare_parameter("default_shoot_mode",      0);
    node->declare_parameter("default_target_id",       0);
    node->declare_parameter("default_decide_yaw",      0.0);
}
// ...existing code...
void blackboard_init(BT::Blackboard::Ptr blackboard, const rclcpp::Node::SharedPtr& node)
{
    blackboard->set("node", node);

    // ============================================================
    // 地图 & 导航参数 (map_params.yaml)
    // ============================================================
    blackboard->set("map_width",      node->get_parameter("map_width").as_double());
    blackboard->set("map_height",     node->get_parameter("map_height").as_double());
    blackboard->set("XY_TOLERANCE",   node->get_parameter("xy_tolerance").as_double());
    blackboard->set("xy_tolerance",   node->get_parameter("xy_tolerance").as_double());

    // ============================================================
    // 运动控制参数 (movement_params.yaml)
    // ============================================================
    blackboard->set("max_rotate_speed", node->get_parameter("max_rotate_speed").as_double());
    blackboard->set("spin_low_speed",   node->get_parameter("spin_low_speed").as_double());
    blackboard->set("spin_mid_speed",   node->get_parameter("spin_mid_speed").as_double());
    blackboard->set("spin_high_speed",  node->get_parameter("spin_high_speed").as_double());

    // ============================================================
    // 战斗参数 (combat_params.yaml)
    // ============================================================
    blackboard->set("ammo_threshold",        node->get_parameter("ammo_threshold").as_int());
    blackboard->set("defence_hp_threshold",  node->get_parameter("defence_hp_threshold").as_int());
    blackboard->set("dodge_radius",          node->get_parameter("dodge_radius").as_double());
    blackboard->set("min_dist",              node->get_parameter("min_dist").as_double());
    blackboard->set("max_dist",              node->get_parameter("max_dist").as_double());
    blackboard->set("close_to",              node->get_parameter("close_to").as_double());
    blackboard->set("back_to",               node->get_parameter("back_to").as_double());

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
    // BT 决策产出 (默认值, 来自 decision_params.yaml)
    // ============================================================
    // --- /serial_send_data ---
    blackboard->set("posture",         static_cast<uint8_t>(node->get_parameter("default_posture").as_int()));
    blackboard->set("confirm_respawn", static_cast<uint8_t>(node->get_parameter("default_confirm_respawn").as_int()));
    blackboard->set("tripod_mode",     static_cast<uint8_t>(node->get_parameter("default_tripod_mode").as_int()));
    blackboard->set("spin_mode",       static_cast<uint8_t>(node->get_parameter("default_spin_mode").as_int()));
    blackboard->set("shoot_mode",      static_cast<uint8_t>(node->get_parameter("default_shoot_mode").as_int()));
    // --- /decision_to_autoaim ---
    blackboard->set("target_id",       static_cast<uint8_t>(node->get_parameter("default_target_id").as_int()));
    blackboard->set("decide_yaw",      node->get_parameter("default_decide_yaw").as_double());

    // ============================================================
    // 导航状态
    // ============================================================
    blackboard->set("nav_target_x", 0.0);
    blackboard->set("nav_target_y", 0.0);
    blackboard->set("nav_cancel",   false);

    // --- 巡逻点位 (空列表占位, 实际值建议在 XML 的 TreeNodesModel 中设定, 或由外部订阅写入) ---
    blackboard->set("patrol_points_x", std::vector<double>{});
    blackboard->set("patrol_points_y", std::vector<double>{});
}

// 运行节点
void run_mode(const rclcpp::Node::SharedPtr& node)
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

    auto sub_nav = node->create_subscription<nav_msgs::msg::Odometry>(
        "/state_estimation", 10,
        [blackboard](nav_msgs::msg::Odometry::SharedPtr msg) {
            blackboard->set("current_x", msg->pose.pose.position.x);
            blackboard->set("current_y", msg->pose.pose.position.y);
            // 这里假定无roll、pitch轴旋转，将w直接当作yaw的cos值，z当作sin值计算yaw角度
            blackboard->set("current_yaw", std::atan2(
                msg->pose.pose.orientation.z, msg->pose.pose.orientation.w) * 2.0);
        });
    // 5. 设置发布 (从黑板读取)
    auto pub_send    = node->create_publisher<DecisionSendData>("/serial_send_data", 10);
    auto pub_autoaim = node->create_publisher<DecisionToAutoaim>("/decision_to_autoaim", 10);
    auto pub_nav     = node->create_publisher<geometry_msgs::msg::PoseStamped>("/goal_pose", 10);

    // 6. 加载行为树
    std::string xml_path = "src/decision_process/xml/main.xml";
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

        {
            geometry_msgs::msg::PoseStamped msg;
            double nav_target_x = 0.0;
            double nav_target_y = 0.0;
            bool nav_cancel = false;
            (void)blackboard->get("nav_target_x", nav_target_x);
            (void)blackboard->get("nav_target_y", nav_target_y);
            (void)blackboard->get("nav_cancel",   nav_cancel);

            if (!nav_cancel) {
                msg.header.stamp = node->now();
                msg.header.frame_id = "map";
                msg.pose.position.x = nav_target_x;
                msg.pose.position.y = nav_target_y;
                msg.pose.position.z = 1.0;

                pub_nav->publish(msg);
            }
        }

        rate.sleep();
    }
}

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("decision_process_node");
    run_mode(node);
    rclcpp::shutdown();
    return 0;
}
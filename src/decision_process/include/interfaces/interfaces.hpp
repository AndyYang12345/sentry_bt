#pragma once

// 统一引入所有需要的接口文件，方便在其他地方使用
#include "sentry_interfaces/msg/receive_data.hpp"
#include "sentry_interfaces/msg/decision_send_data.hpp"
#include "sentry_interfaces/msg/autoaim_to_decision.hpp"
#include "sentry_interfaces/msg/decision_to_autoaim.hpp"
// 导航相关（需要时取消注释并添加 nav_msgs 依赖到 CMakeLists.txt）
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
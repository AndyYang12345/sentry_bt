cmake_minimum_required(VERSION 3.8)
project(decision_process)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sentry_interfaces REQUIRED)

# 直接创建可执行文件
add_executable(decision_process_main
  src/decision_process.cpp
)

# 添加头文件路径
target_include_directories(decision_process_main PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 添加依赖
ament_target_dependencies(decision_process_main
  rclcpp
  behaviortree_cpp
  geometry_msgs
  nav_msgs
  std_msgs
  sentry_interfaces
)

# 安装目标
install(TARGETS
  decision_process_main
  DESTINATION lib/${PROJECT_NAME}
)

# 安装 XML 文件
install(DIRECTORY xml/
  DESTINATION share/${PROJECT_NAME}/xml
)

# 安装 YAML 配置文件
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

# ============================================================
# 调试工具 — SentryCostmap 可视化测试 (需要 Qt5)
# ============================================================
option(BUILD_COSTMAP_TEST "Build costmap visual test tool" OFF)

if(BUILD_COSTMAP_TEST)
  find_package(Qt5 REQUIRED COMPONENTS Widgets)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)

  add_executable(test_costmap
    src/debug/test_custom_costmap.cpp
  )

  target_include_directories(test_costmap PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

  target_link_libraries(test_costmap
    Qt5::Widgets
  )

  # Qt app 独立安装 (非 ROS 节点)
  install(TARGETS test_costmap
    DESTINATION lib/${PROJECT_NAME}/debug
  )
endif()

ament_package()
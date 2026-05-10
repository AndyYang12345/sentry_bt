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
find_package(std_msgs REQUIRED)

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
  std_msgs
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

ament_package()
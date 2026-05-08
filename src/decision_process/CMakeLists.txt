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

add_library(decision_process_nodes SHARED
  include/decision_process/treenode/
)

target_include_directories(decision_process_nodes PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(decision_process_nodes
  rclcpp::rclcpp
  behaviortree_cpp::behaviortree_cpp
  geometry_msgs::geometry_msgs
  std_msgs::std_msgs
)

add_executable(decision_process_main
  src/decision_process_node.cpp
)

target_link_libraries(decision_process_main
  decision_process_nodes
  rclcpp::rclcpp
  behaviortree_cpp::behaviortree_cpp
)

install(TARGETS
  decision_process_nodes
  decision_process_main
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY xml/
  DESTINATION share/${PROJECT_NAME}/xml
)

ament_package()

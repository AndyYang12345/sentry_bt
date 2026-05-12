"""
================================================================
Sentry 决策系统 Launch 文件
================================================================
加载所有分类 yaml 参数并启动 decision_process 主节点。

yaml 文件来源:
  - config/map_params.yaml       地图与导航
  - config/movement_params.yaml  运动控制
  - config/combat_params.yaml    战斗相关
  - config/decision_params.yaml  决策默认值

使用方式:
  ros2 launch sentry_bringup decision_process.launch.py
================================================================
"""

import os
from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():

    # ---- 定位 decision_process 包的 share 目录 ----
    pkg_share = get_package_share_directory("decision_process")

    # ---- 各个 yaml 参数文件路径 ----
    map_params      = os.path.join(pkg_share, "config", "map_params.yaml")
    movement_params = os.path.join(pkg_share, "config", "movement_params.yaml")
    combat_params   = os.path.join(pkg_share, "config", "combat_params.yaml")
    decision_params = os.path.join(pkg_share, "config", "decision_params.yaml")

    # ---- 创建 decision_process 主节点 ----
    decision_node = Node(
        package="decision_process",
        executable="decision_process_main",
        name="decision_process_node",
        output="screen",
        parameters=[
            map_params,
            movement_params,
            combat_params,
            decision_params,
        ],
    )

    return LaunchDescription([
        decision_node,
    ])

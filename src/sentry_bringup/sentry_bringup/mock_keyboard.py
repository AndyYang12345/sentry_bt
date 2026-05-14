#!/usr/bin/env python3
"""
============================================================================
键盘模拟节点 — 用于 behavior tree 仿真测试 (第一级)
============================================================================

通过键盘按键模拟裁判系统、自瞄、里程计话题，驱动行为树运行。

用法:
    # 终端1: 启动行为树
    ros2 launch sentry_bringup decision_process.launch.py

    # 终端2: 启动模拟器
    ros2 run sentry_bringup mock_keyboard.py

按键说明:
    1   — 在 (3, 3) 处生成 1 号可见敌人 (距离3m, 极角0.5rad)
    2   — 哨兵受伤: hp_sentry -= 100
    3   — 弹药耗尽: ammo = 0
    4   — 进入死亡状态: game_period = 1
    5   — 复活: game_period = 2, hp_sentry = 400
    0   — 清除敌人 (target 全部置0)

    W/A/S/D — 移动哨兵 (每步 0.5m)
    R       — 重置所有状态到初始值

    Q 或 Ctrl+C — 退出

原理:
    每 0.1 秒 (10Hz) 发布一次裁判系统、自瞄、里程计数据。
    行为树的 StdCoutLogger 会在终端1中打印节点状态切换日志。
    观察不同按键下行为树分支的变化。
============================================================================
"""

import rclpy
from rclpy.node import Node
from sentry_interfaces.msg import ReceiveData, AutoaimToDecision, DecisionSendData
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped
import sys
import tty
import termios
import select
import threading


class KeyboardMock(Node):
    def __init__(self):
        super().__init__('mock_keyboard')

        # ---- 发布者 ----
        self.pub_receive = self.create_publisher(ReceiveData, '/serial_receive_data', 10)
        self.pub_autoaim  = self.create_publisher(AutoaimToDecision, '/autoaim_to_decision', 10)
        self.pub_odom     = self.create_publisher(Odometry, '/state_estimation', 10)

        # ---- 订阅者 (回显行为树输出) ----
        self.sub_send = self.create_subscription(
            DecisionSendData, '/serial_send_data',
            lambda msg: self.get_logger().info(
                f'BT→串口: posture={msg.posture} respawn={msg.confirm_respawn} '
                f'tripod={msg.tripod_mode} spin={msg.spin_mode} shoot={msg.shoot_mode}'),
            10)
        self.sub_goal = self.create_subscription(
            PoseStamped, '/goal_pose',
            self._on_goal_pose, 10)

        # ---- 哨兵状态 ----
        self.hp_sentry = 400
        self.ammo = 100
        self.game_period = 2   # 2=进行中
        self.x = 5.0
        self.y = 3.0
        self.yaw = 0.0

        # ---- 导航模拟 ----
        self.nav_target_x = None  # None=没有目标
        self.nav_target_y = None
        self.nav_cancel = False
        self.nav_speed = 1.0       # 模拟移动速度 (m/s)

        # ---- 敌方状态 ----
        self.enemy_visible = False
        self.enemy_x = 8.0
        self.enemy_y = 4.0

        # ---- 键盘线程 ----
        self.running = True
        self.keyboard_thread = threading.Thread(target=self._keyboard_loop, daemon=True)
        self.keyboard_thread.start()

        # ---- 定时器 10Hz ----
        self.timer = self.create_timer(0.1, self._publish_all)

        self._print_help()

    def _print_help(self):
        print("""
╔══════════════════════════════════════════════════════════════╗
║         哨兵行为树仿真键盘控制面板                            ║
╠══════════════════════════════════════════════════════════════╣
║  1 = 生成敌人    2 = 受伤(-100hp)   3 = 弹药耗尽(ammo=0)    ║
║  4 = 死亡        5 = 复活           0 = 清除敌人             ║
║  W/A/S/D = 移动   R = 重置全部状态   Q = 退出               ║
╠══════════════════════════════════════════════════════════════╣
║  观察终端1中的 StdCoutLogger 输出看行为树分支变化            ║
╚══════════════════════════════════════════════════════════════╝
        """)

    def _keyboard_loop(self):
        """非阻塞键盘读取 (Linux 终端)"""
        old = termios.tcgetattr(sys.stdin)
        tty.setcbreak(sys.stdin.fileno())
        try:
            while self.running:
                if select.select([sys.stdin], [], [], 0.05)[0]:
                    ch = sys.stdin.read(1).lower()
                    if ch == 'q':
                        self.running = False
                        break
                    self._handle_key(ch)
        finally:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old)

    def _handle_key(self, key):
        """处理按键"""
        if key == '1':
            self.enemy_visible = True
            self.get_logger().info('🎯 生成敌人! (1号, 距离3m)')
        elif key == '2':
            self.hp_sentry = max(0, self.hp_sentry - 100)
            self.get_logger().info(f'💔 受伤! hp={self.hp_sentry}')
        elif key == '3':
            self.ammo = 0
            self.get_logger().info('🔫 弹药耗尽!')
        elif key == '4':
            self.game_period = 1
            self.hp_sentry = 0
            self.get_logger().info('💀 死亡! game_period=1')
        elif key == '5':
            self.game_period = 2
            self.hp_sentry = 400
            self.get_logger().info('✨ 复活! hp=400')
        elif key == '0':
            self.enemy_visible = False
            self.get_logger().info('👻 清除敌人')
        elif key == 'w':
            self.y += 0.5
        elif key == 's':
            self.y -= 0.5
        elif key == 'a':
            self.x -= 0.5
        elif key == 'd':
            self.x += 0.5
        elif key == 'r':
            self.hp_sentry = 400
            self.ammo = 100
            self.game_period = 2
            self.enemy_visible = False
            self.x = 5.0
            self.y = 3.0
            self.get_logger().info('🔄 全部状态已重置')
            self.nav_target_x = None
            self.nav_target_y = None

    def _on_goal_pose(self, msg):
        """行为树发布了新的导航目标 → 开始自动向目标移动"""
        tx = msg.pose.position.x
        ty = msg.pose.position.y
        self.nav_target_x = tx
        self.nav_target_y = ty
        self.nav_cancel = False
        self.get_logger().info(f'🎯 收到导航目标: ({tx:.1f}, {ty:.1f})')

    def _update_navigation(self, dt):
        """每 tick 向导航目标移动一步，模拟真实机器人运动"""
        if self.nav_target_x is None or self.nav_cancel:
            return  # 没有目标或导航已取消

        import math
        dx = self.nav_target_x - self.x
        dy = self.nav_target_y - self.y
        dist = math.hypot(dx, dy)

        if dist < 0.15:  # 到达容差 (与 XY_TOLERANCE=0.2 对齐)
            self.nav_target_x = None
            self.nav_target_y = None
            self.get_logger().info(f'✅ 已到达目标 ({self.x:.1f}, {self.y:.1f})')
            return

        step = self.nav_speed * dt
        if step > dist:
            step = dist
        self.x += (dx / dist) * step
        self.y += (dy / dist) * step

    def _publish_all(self):
        """10Hz 定时发布所有话题"""
        dt = 0.1  # 定时器周期

        # ---- 导航模拟 ----
        self._update_navigation(dt)

        # ---- 裁判系统数据 ====
        rd = ReceiveData()
        rd.game_period  = self.game_period
        rd.time         = 300          # 剩余比赛时间
        rd.hp_sentry    = self.hp_sentry
        rd.defence_buff = 0
        rd.color        = 0            # 红方
        rd.ammo         = self.ammo
        rd.hp_enemy     = [0, 400, 0, 0, 0, 0, 0, 0]  # 1号敌人满血
        self.pub_receive.publish(rd)

        # ---- 自瞄数据 ====
        aa = AutoaimToDecision()
        if self.enemy_visible:
            aa.target            = [0, 1, 0, 0, 0, 0, 0, 0, 0]
            aa.target_distance   = [0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
            aa.target_polar_r    = [0.0, 2.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
            aa.target_polar_angle = [0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        else:
            aa.target            = [0, 0, 0, 0, 0, 0, 0, 0, 0]
            aa.target_distance   = [0.0] * 9
            aa.target_polar_r    = [0.0] * 9
            aa.target_polar_angle = [0.0] * 9
        self.pub_autoaim.publish(aa)

        # ---- 里程计 ===
        odom = Odometry()
        odom.header.stamp = self.get_clock().now().to_msg()
        odom.header.frame_id = 'odom'
        odom.child_frame_id = 'base_link'
        odom.pose.pose.position.x = self.x
        odom.pose.pose.position.y = self.y
        odom.pose.pose.position.z = 0.0
        # yaw → quaternion (绕 Z 轴)
        import math
        odom.pose.pose.orientation.z = math.sin(self.yaw / 2.0)
        odom.pose.pose.orientation.w = math.cos(self.yaw / 2.0)
        self.pub_odom.publish(odom)


def main(args=None):
    rclpy.init(args=args)
    node = KeyboardMock()
    try:
        while rclpy.ok() and node.running:
            rclpy.spin_once(node, timeout_sec=0.1)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()
        print("\n👋 模拟器已退出")


if __name__ == '__main__':
    main()

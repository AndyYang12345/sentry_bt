# 一些学习过程的小想法

- 对行为树的一点认识：行为树构建了一个独立于ros2的内部决策环境，通过主节点对xml文件的解析，组织起来一整套决策逻辑。行为树内部共享一块叫黑板的内存空间，用于分享数据和高效的沟通。
- 我需要做的就是利用行为树CPP给出的四种node基类进行覆写，定义节点类型、黑板沟通接口和tick任务，tick就是驱动行为树运转的动力源，当tick通过主节点和构建的xml树结构传到当前节点时，当前节点的tick函数就会被执行，机器人的运转逻辑就在这其中编写。
- 一点关于黑板和话题信息的小想法：黑板独立于外部话题信息，只能在内部使用，可以通过节点内创建订阅方来获取话题信息，订阅方由于回调函数所以总是会更新到最新状态，但是不一定总是被写入黑板。所以信息架构搭建中最重要的一环应该就是确保合适的信息总是在合适的时候被更新，需要使用信息的节点总是能够获取最新的信息。为了实现这个，我或许应该分类创建一些订阅节点，根据信息需要的频繁程度在对应时候调用，这样能够保证后续节点总是能获得最新的话题信息。
- 关于异步动作节点的小知识：从4.x以后的版本，AsyncActionNode已经改名叫ThreadedAction了，但是这个类的功能依旧非常底层，需要手动覆写很多功能。于是我们可以用ROSActionNode这个类来创建异步动作节点，这个类已经帮我们封装好了很多功能，我们只需要覆写tick函数就可以了，ROSActionNode会自动帮我们处理好线程和回调函数的关系，非常方便。
  - 经过进一步学习后，我发现其实异步动作节点一般用statefulActionNode就可以了，这个类也帮我们封装好了很多功能，能够满足大部分异步动作节点的需求。ROSActionNode虽然也可以用来创建异步动作节点，但是它更适合用来创建一些需要和ROS2 Action Server进行交互的节点，比如说一些需要发送目标或者获取反馈的节点。总之，根据实际需求选择合适的基类来创建异步动作节点是非常重要的。
- 我大致理出来了这个项目执行的顺序：先使用基本ros2节点构建信息解析和发布功能，然后构建一些基本的动作节点和判断节点，最后根据实际需求搭建行为树。
- 在行为树搭建过程中，和ros2话题的通信是一个重点，我必须确保话题信息必须即时被更新到黑板上，节点在做出决策之后，需要及时发布。我需要利用各种回调函数，将信息类型做出区分，全局信息在主节点中更新，局部信息在对应节点中更新，这样能够保证信息的及时性和准确性。
- 上面的理解是错的，我只需要在主节点中spin就能正常处理黑板信息到ros2话题的更新，不需要担心信息更新的问题。树的运行速度很快，不用也不能够进行和外界信息的互换，所以需要共享内存的黑板来进行信息的更新和共享，主节点负责处理黑板和ros2话题的交互，节点只需要专注于自己的功能实现就好了。
- 感觉现在有点懂了行为树的操作逻辑，本质上就是根据自己定义和重载的一些类实现一些功能，然后通过xml定义的行为树来按照顺序实例化这些类，最后通过主节点的tick函数来驱动整个树的运转，树的运转过程中会根据定义好的逻辑来调用对应节点的tick函数，节点的tick函数中会执行一些功能，并且可能会更新黑板上的信息，主节点会根据黑板上的信息来决定下一步应该tick哪个节点，这样就形成了一个完整的决策流程。

## TODO

- [x] 按照要求创建动作节点和基本判断节点，并且搭建一个简单的行为树来测试功能。
- [x] 构建行为树逻辑，确保树的结构合理，能够满足预期的决策流程。
- [ ] 实现仿真

- [x] Day 1 ─── 🚗 基础移动                          [估计 8h]
├── GoToPoint     StatefulAction  发布目标点, 等到达 → SUCCESS
├── SetPosture    SyncAction      姿态切换 1进攻/2防御/3移动
└── GoHome        StatefulAction  继承 GoToPoint, 目标=起始点

- [x] Day 2 ─── 🔁 巡逻 + 打前哨                        [估计 8h]
├── Patrol        StatefulAction  遍历点位列表, 逐个 GoToPoint
├── AttackOutpost StatefulAction  去(16,11)等前哨站hp=0
└── Patrol xml    XML             点位集在xml中可配置

- [x] Day 3 ─── 💚 资源管理                          [估计 8h]
├── CheckLowHP            ConditionNode  hp<阈值 → 回血          ✅ resource_conditions.hpp
├── CheckDefenceBuff      ConditionNode  防御增益特化(buff≥60%)   ✅ resource_conditions.hpp
├── CheckAmmoZero         ConditionNode  无弹触发                  ✅ resource_conditions.hpp
├── GoHeal                StatefulAction 去补给点等回血            ✅ GoToSupplyZone (navigation_tasks.hpp)
└── GoGetAmmo             StatefulAction 去补给点领弹              ✅ GoToSupplyZone (navigation_tasks.hpp)

- [x] Day 4 ─── 🎮 底盘+云台控制 + 小陀螺               [估计 6h]
├── SetSpinMode       SyncAction      前哨站存活→关, 摧毁→开    ✅ SpinHalt/Low/Mid/High (control_actions.hpp)
├── SetTripodMode     SyncAction      有目标→停转, 无目标→360°   ✅ SetTripodOn/Off (control_actions.hpp)
├── ConfirmRespawn    SyncAction      自主复活确认                ✅ Respawn (lifecycle_actions.hpp)
└── 组装主树XML       XML             ReactiveSequence 串联全部子树 ✅ main.xml (7分支优先级选择器)

- [x] Day 5 ─── 🏃 躲避 + 追击                        [估计 8h]
├── DodgeTrigger      ConditionNode  无敌方视野 + 受伤害          ✅ CheckDamage (movement_conditions.hpp)
├── Dodge             StatefulAction 随机/预设移动路径             ✅ Dodge (navigation_tasks.hpp)
├── ChaseEnemy        StatefulAction 追击, 持续更新目标坐标        ✅ ChaseEnemy (navigation_tasks.hpp)
├── KeepDistance      StatefulAction 自适应距离 2.8m~1.8m         ✅ 并入ChaseEnemy (close_to/back_to参数)
└── RadarChase        StatefulAction 自瞄无目标→雷达坐标追击       🟡 雷达数据已接入, 追击逻辑待实现

- [ ] Day 6 ─── 🎯 高级战斗 + 坐标解算                 [估计 8h]
├── LowHPTargetSelect ConditionNode  残血<100 + 距离<4m → 优先    ❌ SelectTarget目前仅按距离选, 需增加残血优先逻辑
├── CoordinateConvert UpdateNode     极坐标→RM地图坐标             ❌ 需实现: x_w = x_self + d*cos(yaw+θ), y_w = y_self + d*sin(yaw+θ)
├── 全体联调         集成测试         整树跑通, 修复bug            ❌ 待行为树XML完善后进行
└── 参数调优         yaml            实战参数整定                  ❌ 需实际比赛环境调试

- [ ] Day 7 ─── 🖥️ Rviz2 / Gazebo 仿真               [估计 8h]
├── 哨兵URDF/SDF     模型            简单底盘+云台模型
├── Rviz2可视化       话题            显示坐标、目标、血量
├── Gazebo世界        场景            地图+障碍+补给点
└── 仿真联调          跑通            哨兵在仿真中完成全流程

- [ ] Day 8 ─── 🎛️ QT 交互界面                       [估计 8h]
├── rqt / 自定义QT    Dashboard      实时显示黑板关键值
├── 参数面板          Dynamic Reconf  滑动条改阈值
├── 话题注入          Topic Pub       手动发送坐标/血量
├── 树状态监控        Groot2连接      实时看BT流转
└── 快捷指令          Service         一键归位/暂停/切换姿态

- [ ] Day 9 ─── 🧪 测试 + 缓冲                        [估计 8h]
├── 单元测试          关键Condition  边界条件验证
├── 场景测试          裁判系统联调    全流程跑通
├── 性能压测          100Hz稳定      CPU/内存检查
└── 缓冲时间          修bug          前两天遗留问题

攻击逻辑：

- 根据可见敌人列表和敌人距离选中目标
- 设置开火指令

追击逻辑：

- 检查当前位置、敌方位置
- 选取追击目标
- tick时先检查距离，如果距离小于停止距离则认为追击成功，返回SUCCESS，直接进入攻击
- 设置最近阈值和最远阈值，如果距离小于最近阈值就停止追击，如果距离大于最远阈值就放弃追击，否则继续追击
- 追击过程中持续更新目标位置，确保追击的实时性和准确性
- 追上后启用攻击逻辑：
  - 关闭云台，开始攻击
  - 如果目标消失或者被击败，认为追击成功，返回SUCCESS

躲避逻辑：

- 触发条件：无敌方视野 + 受伤害
- 行动：在当前位置之外的可移动半径内随即生成目标点并发布
- 伤害解除后一段时间回归正常行为，前一个判定节点负责计时，符合条件才激活该节点，整体组合完成躲避子树

Goto逻辑：

- 输入：目标坐标、停止距离
- 输出：导航目标坐标、导航取消信号
- 过程：直接设定目标距离和坐标，发布导航目标，持续检查当前坐标和目标坐标的距离，如果小于停止距离则认为到达成功

巡逻逻辑：

- 输入：点位列表
- 过程：依次取出点位列表中的坐标，发布导航目标，
- 持续检查当前坐标和目标坐标的距离，如果小于停止距离则认为到达成功，继续下一个点位，直到列表遍历完成

一次追击流程（✅ 已全部在 main.xml 中实现）：

- check_enemy_visible    ✅ CheckEnemyVisible
- select_target          ✅ SelectTarget (最近距离优先, 过滤无敌/已摧毁)
- chase_enemy            ✅ ChaseEnemy (自适应距离 2.8m~1.8m)
- attack_enemy           ✅ AttackEnemy (shoot_mode=1)


## 📋 详细需求完成状态 (2026-05-14 更新)

### ✅ 已完成
- [x]  姿态切换：比赛过程中是需要根据不同的场景切换不同的姿态，默认是移动姿态
          → SetPostureAttack/Defense/Move (control_actions.hpp)
- [x]  躲避行为：无敌方单位但受到伤害→动起来躲避; 伤害解除后回归正常
          → CheckDamage (3秒躲避窗口计时) + CalculateSafePosition + Dodge
- [x]  巡逻行为：在xml可配置的点集中来回巡逻
          → Patrol (navigation_tasks.hpp) + main.xml patrol_points_x/y
- [x]  小陀螺控制：己方前哨被摧毁前关闭小陀螺，被摧毁后打开
          → SpinHalt/Low/Mid/High (control_actions.hpp)
- [x]  云台控制：有射击目标→停转, 无目标→360°观察
          → SetTripodOn/Off (control_actions.hpp)
- [x]  回血阈值：低于200血量时前往补给点回血
          → CheckSelfLowHP (resource_conditions.hpp) + GoHome (navigation_tasks.hpp)
- [x]  运行时参数：血量阈值在xml中指定
          → 所有阈值通过 {变量名} 端口从 Blackboard 传入
- [x]  防御增益特化：buff≥60% + hp<160 才回血
          → CheckUnderDefenseLowHP (resource_conditions.hpp)
- [x]  击打目标：自瞄有非无敌目标→指定目标编号+发送射击指令
          → SelectTarget + AttackEnemy (combat_actions.hpp)
- [x]  不击打无敌目标：hp==1001 跳过
          → SelectTarget 中 if(hp==1001) continue
- [x]  不击打无效单位：建筑 hp==0 跳过
          → SelectTarget 中 if(hp==0 && 建筑) continue
- [x]  距离目标选择：多目标时优先选距离最近的
          → SelectTarget 按 target_distance 取最小
- [x]  追击：通过发布导航目标点引导哨兵移动追击
          → ChaseEnemy (navigation_tasks.hpp)
- [x]  自主确认复活：死亡后自动发送复活确认
          → Respawn (lifecycle_actions.hpp)
- [x]  不抖动的追击行为/自适应距离调节：>4m追到2.8m; <1.2m退到1.8m
          → ChaseEnemy 内 close_to=2.8 / back_to=1.8 参数
- [x]  运行时参数：追击参数在xml中指定
          → min_dist/max_dist/close_to/back_to 全部通过端口传入
- [x]  行为树XML总框架：7分支优先级选择器
          → main.xml (ReactiveFallback: 复活>躲避>回城>防御战>补给>进攻>巡逻)

### 🟡 部分完成
- [ ]  无弹量行为：无弹时若有待领弹量→领弹, 无→站桩等待
          🟡 GoToSupplyZone 已实现导航到补给区, 但缺少"待领弹量检查"和"站桩点等待"逻辑

### ❌ 待实现
- [ ]  归位：比赛结束时自动返回哨兵启动点
          → 需检查 game_period 判断比赛结束阶段, 触发 GoHome
- [ ]  打前哨：比赛开始先攻击敌方前哨站(16,11), 摧毁后才进行其他行为
          → 需 CheckOutpostAlive 条件节点 + XML 最顶层打前哨分支
- [ ]  残血目标选择：hp<100且距离<4m的地面单位→优先击打其中血量最低的
          → SelectTarget 目前仅按距离选, 需增加"残血优先"的二次排序逻辑
- [ ]  自瞄视野内单位相对坐标解算到RM地图坐标系
          💡 理解方向: 自瞄给出的是相对于哨兵的极坐标(distance, polar_angle)
             解算公式: x_w = x_self + d * cos(yaw + θ_polar)
                      y_w = y_self + d * sin(yaw + θ_polar)
             需要 current_x, current_y, current_yaw (已在黑板中)
- [ ]  雷达联动追击：自瞄无目标时→向雷达侦查到的非无敌目标追击
          💡 理解方向: radar_data(float[12]) = 6个敌方单位世界坐标(x0,y0,x1,y1...x5,y5)
             当SelectTarget返回FAILURE时, 遍历radar_data找最近非零坐标作为追击目标
- [ ]  雷达联动拉远距离
          💡 理解方向: 雷达精度低于自瞄, 依赖雷达时应拉远安全距离
             (如 max_dist 从4m调到6m), 避免因精度问题靠太近
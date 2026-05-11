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

- [ ] 按照要求创建动作节点和基本判断节点，并且搭建一个简单的行为树来测试功能。
- [ ] 构建行为树逻辑，确保树的结构合理，能够满足预期的决策流程。
- [ ] 实现仿真

```xml
Day 1 ─── 🚗 基础移动                          [估计 8h]
├── GoToPoint     StatefulAction  发布目标点, 等到达 → SUCCESS
├── SetPosture    SyncAction      姿态切换 1进攻/2防御/3移动
└── GoHome        StatefulAction  继承 GoToPoint, 目标=起始点

Day 2 ─── 🔁 巡逻 + 打前哨                        [估计 8h]
├── Patrol        StatefulAction  遍历点位列表, 逐个 GoToPoint
├── AttackOutpost StatefulAction  去(16,11)等前哨站hp=0
└── Patrol xml    XML             点位集在xml中可配置

Day 3 ─── 💚 资源管理                          [估计 8h]
├── CheckLowHP            ConditionNode  hp<阈值 → 回血
├── CheckDefenceBuff      ConditionNode  防御增益特化
├── CheckAmmoZero         ConditionNode  无弹触发
├── GoHeal                StatefulAction 去补给点等回血
└── GoGetAmmo             StatefulAction 去补给点领弹

Day 4 ─── 🎮 底盘+云台控制 + 小陀螺               [估计 6h]
├── SetSpinMode       SyncAction      前哨站存活→关, 摧毁→开
├── SetTripodMode     SyncAction      有目标→停转, 无目标→360°
├── ConfirmRespawn    SyncAction      自主复活确认
└── 组装主树XML       XML             ReactiveSequence 串联全部子树

Day 5 ─── 🏃 躲避 + 追击                        [估计 8h]
├── DodgeTrigger      ConditionNode  无敌方视野 + 受伤害
├── Dodge             StatefulAction 随机/预设移动路径
├── ChaseEnemy        StatefulAction 追击, 持续更新目标坐标
├── KeepDistance      StatefulAction 自适应距离 2.8m~1.8m
└── RadarChase        StatefulAction 自瞄无目标→雷达坐标追击

Day 6 ─── 🎯 高级战斗 + 坐标解算                 [估计 8h]
├── LowHPTargetSelect ConditionNode  残血<100 + 距离<4m → 优先
├── CoordinateConvert UpdateNode     极坐标→RM地图坐标
├── 全体联调         集成测试         整树跑通, 修复bug
└── 参数调优         yaml            实战参数整定

Day 7 ─── 🖥️ Rviz2 / Gazebo 仿真               [估计 8h]
├── 哨兵URDF/SDF     模型            简单底盘+云台模型
├── Rviz2可视化       话题            显示坐标、目标、血量
├── Gazebo世界        场景            地图+障碍+补给点
└── 仿真联调          跑通            哨兵在仿真中完成全流程

Day 8 ─── 🎛️ QT 交互界面                       [估计 8h]
├── rqt / 自定义QT    Dashboard      实时显示黑板关键值
├── 参数面板          Dynamic Reconf  滑动条改阈值
├── 话题注入          Topic Pub       手动发送坐标/血量
├── 树状态监控        Groot2连接      实时看BT流转
└── 快捷指令          Service         一键归位/暂停/切换姿态

Day 9 ─── 🧪 测试 + 缓冲                        [估计 8h]
├── 单元测试          关键Condition  边界条件验证
├── 场景测试          裁判系统联调    全流程跑通
├── 性能压测          100Hz稳定      CPU/内存检查
└── 缓冲时间          修bug          前两天遗留问题
```

#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <limits>

// ============================================================
// 安全位置计算节点 — SyncActionNode（单 tick 完成）
// ============================================================

/*
    @brief 动态计算安全位置：基于地图、当前位置、雷达威胁信息生成安全点位
    黑板读取:
      current_x       (double)       — 当前X坐标
      current_y       (double)       — 当前Y坐标
      map_width       (double)       — 地图宽度 (m)
      map_height      (double)       — 地图高度 (m)
      radar_data      (float[12])    — 敌方单位坐标数组
      safe_points_x   (vector<double>) — 已有安全点列表（可为空，用于增量更新或覆盖）
      safe_points_y   (vector<double>)

    黑板写入:
      safe_points_x   (vector<double>) — 动态计算的安全点位X
      safe_points_y   (vector<double>) — 动态计算的安全点位Y

    计算流程（占位符，待后续实现）:
      1. 读取地图边界，确定有效活动区域
      2. 读取雷达数据，解析各敌方单位位置，映射威胁系数
      3. 根据威胁场 + 距离约束，生成 N 个安全候选点
      4. 按距离排序，写入结果

    @return SUCCESS 计算完成; FAILURE 输入数据不足
*/
class CalculateSafePosition : public BT::SyncActionNode {
public:
    CalculateSafePosition(const std::string& name, const BT::NodeConfig& config)
        : SyncActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {
            BT::InputPort<double>("current_x",     "当前X坐标"),
            BT::InputPort<double>("current_y",     "当前Y坐标"),
            BT::InputPort<double>("map_width",     "地图宽度(m)"),
            BT::InputPort<double>("map_height",    "地图高度(m)"),
            BT::InputPort<std::vector<float>>("radar_data", "敌方雷达坐标数组[12]"),
            BT::OutputPort<std::vector<double>>("safe_points_x", "动态安全点X列表"),
            BT::OutputPort<std::vector<double>>("safe_points_y", "动态安全点Y列表"),
        };
    }

    BT::NodeStatus tick() override {
        // ============================================================
        // 1. 读取输入
        // ============================================================
        double cx = 0, cy = 0, mw = 0, mh = 0;
        if (!getInput("current_x", cx) || !getInput("current_y", cy))
            return BT::NodeStatus::FAILURE;
        if (!getInput("map_width", mw) || !getInput("map_height", mh))
            return BT::NodeStatus::FAILURE;

        std::vector<float> radar;
        getInput("radar_data", radar);

        // ============================================================
        // 2. 占位符逻辑 — 待后续实现高级威胁映射
        // ============================================================
        //
        // TODO: 在此处实现完整的威胁场计算和最优安全点选择逻辑
        //
        //   步骤建议:
        //   a) 解析 radar_data[0..11] → 6个敌方单位的 (x, y) 坐标
        //   b) 对每个单位计算: dist = hypot(cx - ex, cy - ey)
        //   c) 威胁权重映射 (距离越近威胁越大)
        //   d) 构建威胁场，在有效区域内采样候选点
        //   e) 按 (安全性, 可达性) 排序选出 N 个点
        //   f) 写入 safe_points_x / safe_points_y
        //
        //   当前占位：生成 4 个角落安全点
        //
        // 示例威胁解析:
        //   for (int i = 0; i < 6; ++i) {
        //       float ex = radar[2*i], ey = radar[2*i+1];
        //       if (ex == 0 && ey == 0) continue;  // 雷达未识别
        //       double dist = std::hypot(cx - ex, cy - ey);
        //       double threat = 1.0 / (dist + 1e-6);
        //       // ... 威胁场叠加 ...
        //   }

        // ---- 占位: 四个角落 + 地图中心偏移 ----
        std::vector<double> sx, sy;
        const double margin = 1.0;  // 离地图边缘的安全边距

        auto add_if_valid = [&](double x, double y) {
            if (x >= margin && x <= mw - margin &&
                y >= margin && y <= mh - margin) {
                sx.push_back(x);
                sy.push_back(y);
            }
        };

        add_if_valid(margin,        margin);         // 左下
        add_if_valid(mw - margin,   margin);         // 右下
        add_if_valid(margin,        mh - margin);    // 左上
        add_if_valid(mw - margin,   mh - margin);    // 右上
        add_if_valid(mw * 0.5,      mh * 0.5);       // 中心 (作为基准)

        // 如果当前位置离角落太近，额外添加反射点
        for (size_t i = 0; i < sx.size(); ++i) {
            double reflect_x = 2 * cx - sx[i];
            double reflect_y = 2 * cy - sy[i];
            add_if_valid(reflect_x, reflect_y);
        }

        // ============================================================
        // 3. 写入输出
        // ============================================================
        if (sx.empty())
            return BT::NodeStatus::FAILURE;

        setOutput("safe_points_x", sx);
        setOutput("safe_points_y", sy);

        return BT::NodeStatus::SUCCESS;
    }
};

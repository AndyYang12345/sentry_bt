/**
 * ============================================================
 * SentryCostmap 可视化测试工具
 * ============================================================
 * 功能:
 *   1. 画布绘制代价地图 (墙面、敌方单位、安全点、代价热力)
 *   2. 下拉框选择模式: 放置墙面 / 放置敌方 / 查询点 / 寻安全点
 *   3. 墙面模式: 点击两点 → 弹出输入高度 → 注册
 *   4. 敌方模式: 点击放置敌方单位 → 弹出设置权重
 *   5. 查询模式: 点击任意位置 → 显示 height/isPassable/threatCost
 *   6. 安全点模式: 点击位置 → 显示安全点和各点代价
 *   7. 右键清除最近添加的元素
 *
 * 编译依赖: Qt5 (Widgets)
 */

#include <QApplication>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QStatusBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QPointF>
#include <QMenu>

#include <cmath>
#include <vector>
#include <string>
#include <sstream>

#include "decision_process/utils/sentry_costmap.hpp"

// ============================================================
// 常量
// ============================================================
static constexpr double MAP_W       = 28.0;
static constexpr double MAP_H       = 15.0;
static constexpr double SCALE       = 30.0;   // pixels per meter
static constexpr double MARGIN      = 40.0;   // 画布边距 (px)

// ============================================================
// 自定义 QGraphicsView — 支持鼠标点击
// ============================================================
class CostmapView : public QGraphicsView {
    Q_OBJECT
public:
    enum Mode { MODE_WALL, MODE_THREAT, MODE_QUERY, MODE_SAFEPOINT };

    CostmapView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent), scene_(scene) {}

    void setMode(Mode m) { mode_ = m; resetPicker(); }
    Mode mode() const { return mode_; }

    void setCostmap(SentryCostmap* cm) { costmap_ = cm; }

signals:
    void wallPointPicked(QPointF p1, QPointF p2, double height);
    void threatPointPicked(QPointF p, double weight);
    void queryPointPicked(QPointF p);
    void safePointPicked(QPointF p);
    void rightClickClear(QPointF p);

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (!costmap_) { QGraphicsView::mousePressEvent(event); return; }
        QPointF world = mapToScene(event->pos());
        QPointF mapPt = sceneToMap(world);

        if (event->button() == Qt::RightButton) {
            emit rightClickClear(mapPt);
            return;
        }

        if (event->button() != Qt::LeftButton) return;

        switch (mode_) {
        case MODE_WALL:
            if (!picking_) {
                pick1_ = mapPt;
                picking_ = true;
                // 画临时起点标记
                drawTempMarker(pick1_, Qt::blue);
            } else {
                pick2_ = mapPt;
                picking_ = false;
                clearTempMarkers();
                bool ok = false;
                double h = QInputDialog::getDouble(this, "墙面高度",
                    "输入墙面高度 (m):", 1.0, 0.0, 10.0, 2, &ok);
                if (ok) emit wallPointPicked(pick1_, pick2_, h);
            }
            break;
        case MODE_THREAT:
        case MODE_QUERY:
        case MODE_SAFEPOINT:
            applyModeClick(mapPt);
            break;
        }
    }

private:
    void applyModeClick(QPointF mapPt) {
        switch (mode_) {
        case MODE_THREAT: {
            bool ok = false;
            double w = QInputDialog::getDouble(this, "威胁权重",
                "输入威胁权重:", 1.0, 0.1, 100.0, 2, &ok);
            if (ok) emit threatPointPicked(mapPt, w);
            break;
        }
        case MODE_QUERY:   emit queryPointPicked(mapPt);  break;
        case MODE_SAFEPOINT: emit safePointPicked(mapPt); break;
        default: break;
        }
    }

    void resetPicker() { picking_ = false; clearTempMarkers(); }

    void drawTempMarker(QPointF mapPt, QColor color) {
        QPointF sp = mapToScene(mapPt);
        auto* item = scene_->addEllipse(sp.x()-4, sp.y()-4, 8, 8,
                                        QPen(color), QBrush(color));
        temp_items_.push_back(item);
    }

    void clearTempMarkers() {
        for (auto* item : temp_items_) { scene_->removeItem(item); delete item; }
        temp_items_.clear();
    }

    QPointF mapToScene(QPointF mapPt) const {
        return QPointF(MARGIN + mapPt.x() * SCALE,
                       MARGIN + (MAP_H - mapPt.y()) * SCALE);
    }

    static QPointF sceneToMap(QPointF scenePt) {
        return QPointF((scenePt.x() - MARGIN) / SCALE,
                       MAP_H - (scenePt.y() - MARGIN) / SCALE);
    }

    Mode mode_ = MODE_QUERY;
    SentryCostmap* costmap_ = nullptr;
    QGraphicsScene* scene_;
    bool picking_ = false;
    QPointF pick1_, pick2_;
    std::vector<QGraphicsItem*> temp_items_;
};

// ============================================================
// 主窗口
// ============================================================
class TestWindow : public QMainWindow {
    Q_OBJECT
public:
    TestWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("SentryCostmap 可视化测试");
        setMinimumSize(1000, 700);

        // 代价地图
        costmap_ = makeTempMap();

        // ---- 场景 & 视图 ----
        scene_ = new QGraphicsScene(this);
        view_  = new CostmapView(scene_, this);
        view_->setCostmap(&costmap_);
        view_->setRenderHint(QPainter::Antialiasing);
        view_->setDragMode(QGraphicsView::ScrollHandDrag);
        view_->setSceneRect(0, 0, MARGIN*2 + MAP_W*SCALE, MARGIN*2 + MAP_H*SCALE);

        // ---- 控件 ----
        comboMode_ = new QComboBox(this);
        comboMode_->addItem("🔍 查询点", QVariant(static_cast<int>(CostmapView::MODE_QUERY)));
        comboMode_->addItem("🧱 放置墙面 (点两点)", QVariant(static_cast<int>(CostmapView::MODE_WALL)));
        comboMode_->addItem("👾 放置敌方单位", QVariant(static_cast<int>(CostmapView::MODE_THREAT)));
        comboMode_->addItem("🛡️ 寻找安全点", QVariant(static_cast<int>(CostmapView::MODE_SAFEPOINT)));

        btnClear_ = new QPushButton("清空全部", this);
        btnReset_ = new QPushButton("重置测试地图", this);
        lblInfo_  = new QLabel("点击地图查看信息", this);
        lblInfo_->setWordWrap(true);
        lblInfo_->setMinimumHeight(40);

        QVBoxLayout* sideBar = new QVBoxLayout;
        sideBar->addWidget(new QLabel("模式:"));
        sideBar->addWidget(comboMode_);
        sideBar->addWidget(btnClear_);
        sideBar->addWidget(btnReset_);
        sideBar->addStretch();

        QVBoxLayout* rightPanel = new QVBoxLayout;
        rightPanel->addWidget(lblInfo_);
        rightPanel->addStretch();

        QHBoxLayout* mainLayout = new QHBoxLayout;
        mainLayout->addLayout(sideBar);
        mainLayout->addWidget(view_, 1);
        mainLayout->addLayout(rightPanel);

        QWidget* central = new QWidget(this);
        central->setLayout(mainLayout);
        setCentralWidget(central);

        statusBar()->showMessage("就绪 | 右键点击可清除最近添加的元素");

        // ---- 信号 ----
        connect(comboMode_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &TestWindow::onModeChanged);
        connect(btnClear_,  &QPushButton::clicked, this, &TestWindow::onClearAll);
        connect(btnReset_,  &QPushButton::clicked, this, &TestWindow::onReset);

        connect(view_, &CostmapView::wallPointPicked,
                this, &TestWindow::onWallPicked);
        connect(view_, &CostmapView::threatPointPicked,
                this, &TestWindow::onThreatPicked);
        connect(view_, &CostmapView::queryPointPicked,
                this, &TestWindow::onQueryPicked);
        connect(view_, &CostmapView::safePointPicked,
                this, &TestWindow::onSafePointPicked);
        connect(view_, &CostmapView::rightClickClear,
                this, &TestWindow::onRightClick);

        // ---- 初始绘制 ----
        redrawAll();
    }

private slots:
    void onModeChanged(int idx) {
        auto mode = static_cast<CostmapView::Mode>(
            comboMode_->itemData(idx).toInt());
        view_->setMode(mode);
        statusBar()->showMessage(
            QString("当前模式: %1").arg(comboMode_->currentText()));
    }

    void onWallPicked(QPointF p1, QPointF p2, double height) {
        std::string id = "wall_" + std::to_string(wallCounter_++);
        costmap_.registerWall(id, p1.x(), p1.y(), p2.x(), p2.y(), height);
        redrawAll();
        statusBar()->showMessage(
            QString("已注册墙面 %1: (%2,%3)→(%4,%5) h=%6")
                .arg(QString::fromStdString(id))
                .arg(p1.x(),0,'f',1).arg(p1.y(),0,'f',1)
                .arg(p2.x(),0,'f',1).arg(p2.y(),0,'f',1)
                .arg(height));
    }

    void onThreatPicked(QPointF p, double weight) {
        int id = threatCounter_++;
        costmap_.setThreat(id, p.x(), p.y(), weight);
        redrawAll();
        statusBar()->showMessage(
            QString("已放置敌方 #%1: (%2,%3) weight=%4")
                .arg(id).arg(p.x(),0,'f',1).arg(p.y(),0,'f',1).arg(weight));
    }

    void onQueryPicked(QPointF p) {
        double h = costmap_.heightAt(p.x(), p.y());
        bool pass = costmap_.isPassable(p.x(), p.y());
        double cost = costmap_.threatCost(p.x(), p.y());
        auto info = costmap_.cellInfo(p.x(), p.y());

        QString text = QString(
            "📍 查询点: (%1, %2)\n"
            "  高度: %3 m\n"
            "  可通行: %4\n"
            "  威胁代价: %5\n"
            "  障碍物: %6")
            .arg(p.x(),0,'f',1).arg(p.y(),0,'f',1)
            .arg(h,0,'f',1)
            .arg(pass ? "✅ 是" : "❌ 否")
            .arg(cost,0,'f',3)
            .arg(info.blocking_wall
                 ? QString::fromStdString(info.blocking_wall->id)
                 : "无");

        lblInfo_->setText(text);

        // 画查询标记
        clearQueryMarkers();
        QPointF sp = mapToScene(p);
        queryMarker_ = scene_->addEllipse(sp.x()-5, sp.y()-5, 10, 10,
                                          QPen(Qt::red, 2), QBrush(Qt::red));
        queryItems_.push_back(queryMarker_);

        double displayCostQ = (cost > 9999.0) ? 9999.0 : cost;
        QString tag = QString("Q (%1,%2)\ncost=%3")
                          .arg(p.x(),0,'f',1).arg(p.y(),0,'f',1).arg(displayCostQ,0,'f',2);
        auto* textItem = scene_->addText(tag);
        textItem->setPos(sp.x()+8, sp.y()-20);
        textItem->setDefaultTextColor(Qt::red);
        textItem->setScale(0.7);
        queryItems_.push_back(textItem);
    }

    void onSafePointPicked(QPointF p) {
        const double radius = 5.0, step = 0.5;
        const double COST_CAP = 9999.0;  // 显示限幅，避免 1e308 撑破画布

        // 寻找安全点
        auto safep = costmap_.findSafePoint(p.x(), p.y(), radius, step);

        // 若找不到可通行点（全被墙围住），给出提示
        if (safep.cost >= 999.0) {
            lblInfo_->setText(QString(
                "⚠️ 搜索半径 %1m 内无可通行安全点！\n"
                "   当前区域可能被墙面完全包围。")
                .arg(radius));
            clearQueryMarkers();
            return;
        }

        // 采样几个点展示代价（限幅到 COST_CAP）
        QString costList;
        for (double dy = -2.0; dy <= 2.0; dy += 2.0) {
            for (double dx = -2.0; dx <= 2.0; dx += 2.0) {
                double cx = safep.x + dx, cy = safep.y + dy;
                if (cx < 0 || cx > MAP_W || cy < 0 || cy > MAP_H) continue;
                double c = costmap_.threatCost(cx, cy);
                if (c > COST_CAP) c = COST_CAP;
                costList += QString("  (%1,%2) cost=%3\n")
                                .arg(cx,0,'f',1).arg(cy,0,'f',1).arg(c,0,'f',1);
            }
        }

        double displayCost = (safep.cost > COST_CAP) ? COST_CAP : safep.cost;
        lblInfo_->setText(QString(
            "🛡️ 安全点: (%1, %2)\n"
            "  代价: %3\n"
            "  搜索半径: %4m  步长: %5m\n"
            "  周围采样代价:\n%6")
            .arg(safep.x,0,'f',1).arg(safep.y,0,'f',1)
            .arg(displayCost,0,'f',1).arg(radius).arg(step)
            .arg(costList));

        // 画安全点和采样网格
        clearQueryMarkers();

        // 搜索范围框
        QPointF tl = mapToScene(QPointF(
            std::max(0.0, p.x()-radius),
            std::max(0.0, p.y()-radius)));
        QPointF br = mapToScene(QPointF(
            std::min(MAP_W, p.x()+radius),
            std::min(MAP_H, p.y()+radius)));
        auto* rect = scene_->addRect(QRectF(tl, br), QPen(Qt::darkGreen, 1, Qt::DashLine));
        queryItems_.push_back(rect);

        // 🔵 蓝色：当前点击位置
        QPointF cp = mapToScene(QPointF(p.x(), p.y()));
        auto* curDot = scene_->addEllipse(cp.x()-5, cp.y()-5, 10, 10,
                                          QPen(Qt::blue, 2), QBrush(Qt::blue));
        queryItems_.push_back(curDot);

        // 🟢 绿色：最优安全点
        QPointF sp = mapToScene(QPointF(safep.x, safep.y));
        auto* marker = scene_->addEllipse(sp.x()-6, sp.y()-6, 12, 12,
                                          QPen(Qt::green, 3), QBrush(Qt::green));
        queryItems_.push_back(marker);

        auto* tag = scene_->addText(
            QString("S (%1,%2)\ncost=%3")
                .arg(safep.x,0,'f',1).arg(safep.y,0,'f',1).arg(safep.cost,0,'f',3));
        tag->setPos(sp.x()+8, sp.y()-20);
        tag->setDefaultTextColor(Qt::darkGreen);
        tag->setScale(0.7);
        queryItems_.push_back(tag);

        // 采样格点代价标签
        for (double dy = -2.0; dy <= 2.0; dy += 2.0) {
            for (double dx = -2.0; dx <= 2.0; dx += 2.0) {
                double cx = safep.x + dx, cy = safep.y + dy;
                if (cx < 0 || cx > MAP_W || cy < 0 || cy > MAP_H) continue;
                double c = costmap_.threatCost(cx, cy);
                QPointF gp = mapToScene(QPointF(cx, cy));
                auto* dot = scene_->addEllipse(gp.x()-2, gp.y()-2, 4, 4,
                                               QPen(Qt::darkCyan), QBrush(Qt::cyan));
                queryItems_.push_back(dot);
                auto* ct = scene_->addText(QString::number(c, 'f', 2));
                ct->setPos(gp.x()+4, gp.y()-10);
                ct->setDefaultTextColor(Qt::darkCyan);
                ct->setScale(0.5);
                queryItems_.push_back(ct);
            }
        }
    }

    void onRightClick(QPointF mapPt) {
        // 尝试清除最近的墙面或敌方
        (void)mapPt;
        if (!costmap_.threats().empty()) {
            costmap_.clearThreats();
            threatCounter_ = 1;
            redrawAll();
            statusBar()->showMessage("已清除所有敌方单位");
        } else if (costmap_.wallCount() > 5) {  // 保留初始的 5 面墙
            // 需要直接操作底层 vector... 这里简化处理
            statusBar()->showMessage("右键清除: 请使用清空按钮重置");
        }
    }

    void onClearAll() {
        costmap_ = SentryCostmap(MAP_W, MAP_H);
        wallCounter_  = 0;
        threatCounter_ = 1;
        redrawAll();
        statusBar()->showMessage("已清空所有墙面和敌方单位");
    }

    void onReset() {
        costmap_ = makeTempMap();
        wallCounter_  = 5;
        threatCounter_ = 1;
        redrawAll();
        statusBar()->showMessage("已重置为测试地图");
    }

private:
    // ============================================================
    // 绘制
    // ============================================================
    void redrawAll() {
        scene_->clear();
        queryItems_.clear();
        queryMarker_ = nullptr;

        drawGrid();
        drawWalls();
        drawThreats();
        drawLabels();
    }

    void drawGrid() {
        QPen pen(QColor(200, 200, 200), 0.5);
        for (int ix = 0; ix <= static_cast<int>(MAP_W); ++ix) {
            QPointF p1 = mapToScene(QPointF(ix, 0));
            QPointF p2 = mapToScene(QPointF(ix, MAP_H));
            scene_->addLine(p1.x(), p1.y(), p2.x(), p2.y(), pen);
        }
        for (int iy = 0; iy <= static_cast<int>(MAP_H); ++iy) {
            QPointF p1 = mapToScene(QPointF(0, iy));
            QPointF p2 = mapToScene(QPointF(MAP_W, iy));
            scene_->addLine(p1.x(), p1.y(), p2.x(), p2.y(), pen);
        }
        // 地图边框
        QPointF tl = mapToScene(QPointF(0, MAP_H));
        QPointF br = mapToScene(QPointF(MAP_W, 0));
        scene_->addRect(QRectF(tl, br), QPen(Qt::black, 2));
    }

    void drawWalls() {
        for (size_t i = 0; i < costmap_.wallCount(); ++i) {
            const auto* w = costmap_.getWall(i);
            if (!w) continue;
            QPointF tl = mapToScene(QPointF(w->x_low(), w->y_high()));
            QPointF br = mapToScene(QPointF(w->x_high(), w->y_low()));
            int alpha = std::min(200, 50 + static_cast<int>(w->height * 40));
            QColor color(139, 69, 19, alpha);
            scene_->addRect(QRectF(tl, br), QPen(Qt::darkRed, 1), QBrush(color));
            auto* text = scene_->addText(
                QString("%1\nh=%2").arg(QString::fromStdString(w->id)).arg(w->height));
            QPointF center = mapToScene(
                QPointF((w->x_low()+w->x_high())/2, (w->y_low()+w->y_high())/2));
            text->setPos(center.x()-20, center.y()-15);
            text->setDefaultTextColor(Qt::darkRed);
            text->setScale(0.5);
        }
    }

    void drawThreats() {
        for (const auto& t : costmap_.threats()) {
            QPointF sp = mapToScene(QPointF(t.x, t.y));
            int r = std::max(5, static_cast<int>(t.weight * 3));
            QColor color(255, 100, 0, 180);
            scene_->addEllipse(sp.x()-r, sp.y()-r, r*2, r*2,
                                              QPen(Qt::red, 2), QBrush(color));
            auto* text = scene_->addText(
                QString("E%1\nw=%2").arg(t.id).arg(t.weight));
            text->setPos(sp.x()+r+2, sp.y()-15);
            text->setDefaultTextColor(Qt::red);
            text->setScale(0.5);
        }
    }

    void drawLabels() {
        // 坐标轴标签
        for (int ix = 0; ix <= static_cast<int>(MAP_W); ix += 4) {
            QPointF p = mapToScene(QPointF(ix, 0));
            auto* t = scene_->addText(QString::number(ix));
            t->setPos(p.x()-8, p.y()+2);
            t->setScale(0.6);
        }
        for (int iy = 0; iy <= static_cast<int>(MAP_H); iy += 3) {
            QPointF p = mapToScene(QPointF(0, iy));
            auto* t = scene_->addText(QString::number(iy));
            t->setPos(p.x()-20, p.y()-8);
            t->setScale(0.6);
        }
    }

    void clearQueryMarkers() {
        // 从 scene 中移除并 delete（QGraphicsScene::removeItem 不 delete，需手动）
        for (auto* item : queryItems_) {
            scene_->removeItem(item);
            delete item;
        }
        queryItems_.clear();
        queryMarker_ = nullptr;
    }

    static QPointF mapToScene(QPointF mapPt) {
        return QPointF(MARGIN + mapPt.x() * SCALE,
                       MARGIN + (MAP_H - mapPt.y()) * SCALE);
    }

    // ============================================================
    // 成员
    // ============================================================
    SentryCostmap costmap_;
    QGraphicsScene* scene_ = nullptr;
    CostmapView*    view_  = nullptr;
    QComboBox*      comboMode_ = nullptr;
    QPushButton*    btnClear_  = nullptr;
    QPushButton*    btnReset_  = nullptr;
    QLabel*         lblInfo_   = nullptr;

    int wallCounter_   = 5;   // makeTempMap 已注册 5 面墙
    int threatCounter_ = 1;

    // 查询标记
    QGraphicsItem* queryMarker_ = nullptr;
    std::vector<QGraphicsItem*> queryItems_;
};

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    TestWindow win;
    win.show();
    return app.exec();
}

#include "test_custom_costmap.moc"

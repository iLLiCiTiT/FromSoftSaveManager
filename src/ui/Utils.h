#pragma once

#include <qevent.h>
#include <QLibrary>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QUuid>


inline bool tryLoadDsrResources() {
    QLibrary lib("inv_resources_dsr");
    return lib.load();
}

inline bool tryLoadDs3Resources() {
    QLibrary lib("inv_resources_ds3");
    return lib.load();
}

namespace {
    bool g_dsrResourcesLoaded = tryLoadDsrResources();
    bool g_ds3ResourcesLoaded = tryLoadDs3Resources();
}

inline bool hasDSRInventoryResources() {return g_dsrResourcesLoaded;}
inline bool hasDS3InventoryResources() {return g_ds3ResourcesLoaded;}

// TODO add path related helpers
// - e.g. join using 'std::filesystem::path'

inline std::string generateUUID() {
    QUuid uuid = QUuid::createUuid();
    QString s = uuid.toString(QUuid::WithoutBraces);
    return s.toStdString();
}

// Spinbox that does listen to wheel evens only if has focus
class FocusSpinBox: public QSpinBox {
    Q_OBJECT
public:
    explicit FocusSpinBox(QWidget* parent = nullptr);
    void wheelEvent(QWheelEvent *event) override ;
};

// Label showing pixmaps with correct aspect ratio
class PixmapLabel: public QLabel {
public:
    explicit PixmapLabel(QPixmap pix, QWidget* parent = nullptr);
    void setSourcePixmap(QPixmap pix);
    void resizeEvent(QResizeEvent *event) override;
    QSize minimumSizeHint();
private:
    QSize getPixmapSize();
    void setResizedPixmap();
    QSize m_lastSize;
    float m_aspectRatio;
    QPixmap m_pix;
    QPixmap m_emptyPix;
};

// Custom tab widget
class TabButton: public QPushButton {
    Q_OBJECT
signals:
    void requested(int tabIdx);
public:
    explicit TabButton(int tabIdx, const QString& tabLabel, QWidget* parent);
    void setSelected(bool selected);
private slots:
    void onClick();
private:
    int m_tabIdx;
    bool m_isSelected = false;
};

struct TabButtonWrap {
    QWidget* widget = nullptr;
    TabButton* btn = nullptr;
};

class TabWidget: public QWidget {
    Q_OBJECT
public:
    explicit TabWidget(QWidget* parent = nullptr);
    void addTab(const QString& tabLabel, QWidget* widget);

private slots:
    void setCurrentTab(int tabIdx);

private:
    std::vector<TabButtonWrap> m_tabs;
    int m_currentTabId = -1;
    QWidget* m_barWidget = nullptr;
    QHBoxLayout* m_barLayout = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QWidget* m_emptyWidget = nullptr;
};

class BaseClickableFrame: public QFrame {
    Q_OBJECT
public:
    explicit BaseClickableFrame(QWidget* parent);
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    virtual void onMouseRelease() = 0;
    bool m_mousePressed = false;
};

class ClickableFrame: public BaseClickableFrame {
    Q_OBJECT
public:
    explicit ClickableFrame(QWidget* parent): BaseClickableFrame(parent) {};
signals:
    void clicked();
private:
    void onMouseRelease() override {
        emit clicked();
    };
};
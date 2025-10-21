#include "Utils.h"

#include <QStyle>

FocusSpinBox::FocusSpinBox(QWidget* parent): QSpinBox(parent) {
    setFocusPolicy(Qt::StrongFocus);
};

void FocusSpinBox::wheelEvent(QWheelEvent *event) {
    if (hasFocus())
        QSpinBox::wheelEvent(event);
    else
        event->ignore();
};

PixmaxLabel::PixmaxLabel(QPixmap pix, QWidget* parent)
    : QLabel(parent)
    , m_pix(pix)
{
    m_aspectRatio = (float)pix.width() / (float)pix.height();
    m_lastSize = QSize(0, 0);
};

void PixmaxLabel::setSourcePixmap(QPixmap pix) {
    m_pix = pix;
    m_aspectRatio = (float)pix.width() / (float)pix.height();
    setResizedPixmap();
};

void PixmaxLabel::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    setResizedPixmap();
};

QSize PixmaxLabel::minimumSizeHint() {
    QSize size = getPixmapSize();
    if (size != m_lastSize) setResizedPixmap();
    return size;
};

QSize PixmaxLabel::getPixmapSize() {
    int height = fontMetrics().height();
    height += height % 2;
    if (m_aspectRatio > 1) return QSize(height, height / m_aspectRatio);
    return QSize(height * m_aspectRatio, height);
};

void PixmaxLabel::setResizedPixmap() {
    if (m_pix.isNull()) setPixmap(m_emptyPix);
    m_lastSize = getPixmapSize();
    setPixmap(m_pix.scaled(m_lastSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
};

TabButton::TabButton(int tabIdx, const QString& tabLabel, QWidget* parent)
    : QPushButton(tabLabel, parent),
    m_tabIdx(tabIdx)
{
    setDefault(false);
    setAutoDefault(false);
    connect(this, SIGNAL(clicked()), this, SLOT(onClick()));
};

void TabButton::setSelected(bool selected) {
    if (selected == m_isSelected) return;
    m_isSelected = selected;
    setProperty("selected", selected ? "1" : "0");
    style()->polish(this);

};

void TabButton::onClick() {
    emit requested(m_tabIdx);
};


TabWidget::TabWidget(QWidget* parent): QWidget(parent) {
    m_barWidget = new QWidget(this);
    m_barWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    m_barLayout = new QHBoxLayout(m_barWidget);
    m_barLayout->setContentsMargins(0, 0, 0, 0);
    m_barLayout->setSpacing(0);
    m_barLayout->addStretch(1);

    m_contentWidget = new QWidget(this);
    m_contentWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_emptyWidget = new QWidget(this);
    m_emptyWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->addWidget(m_emptyWidget, 1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_barWidget, 0);
    layout->addWidget(m_contentWidget, 1);
};

void TabWidget::addTab(const QString& tabLabel, QWidget* widget) {
    int tabIdx = m_tabs.size();
    TabButton* tabBtn = new TabButton(tabIdx, tabLabel, this);
    m_barLayout->insertWidget(m_barLayout->count() - 1, tabBtn, 0);

    widget->setParent(m_contentWidget);
    widget->setVisible(false);

    m_tabs.push_back({
        .widget = widget,
        .btn = tabBtn,
    });

    connect(tabBtn, SIGNAL(requested(int)), this, SLOT(setCurrentTab(int)));

    if (tabIdx == 0) setCurrentTab(tabIdx);
};

void TabWidget::setCurrentTab(int tabIdx) {
    if (tabIdx == m_currentTabId) return;
    if (tabIdx < 0 || tabIdx >= m_tabs.size()) return;
    if (m_currentTabId >= 0) {
        m_tabs.at(m_currentTabId).btn->setSelected(false);
        m_tabs.at(m_currentTabId).widget->setVisible(false);
    }
    m_currentTabId = tabIdx;
    m_tabs.at(m_currentTabId).btn->setSelected(true);
    m_tabs.at(m_currentTabId).widget->setVisible(true);
    m_contentLayout->takeAt(0);
    m_contentLayout->addWidget(m_tabs.at(m_currentTabId).widget, 1);
};

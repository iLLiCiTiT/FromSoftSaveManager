#include "ScrollHelper.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QWidget>
#include <algorithm>

namespace {
    // The painted bars are a few pixels wide; the mouse gets a wider strip.
    constexpr int HIT_WIDTH = 14;
}

namespace fssm::ui {
PaintedScroll::PaintedScroll(QWidget* owner): m_owner(owner) {
    // Hover feedback on the thumb.
    m_owner->setMouseTracking(true);
}

void PaintedScroll::setGeometry(const QRect& viewport, const QRect& track, int contentHeight) {
    m_viewport = viewport;
    m_track = track;
    m_maximum = std::max(0, contentHeight - viewport.height());
    m_value = std::clamp(m_value, 0, m_maximum);
}

void PaintedScroll::clear() {
    m_viewport = {};
    m_track = {};
    m_maximum = 0;
    m_thumbHovered = false;
}

QRect PaintedScroll::thumb() const {
    if (m_maximum <= 0 || m_track.height() <= 0) return {};
    const int h = m_track.height();
    const int thumbH = std::min(h, std::max(m_minThumb, h * h / (h + m_maximum)));
    const int thumbY = m_track.top() + (h - thumbH) * m_value / m_maximum;
    return {m_track.left(), thumbY, m_track.width(), thumbH};
}

QRect PaintedScroll::hitTrack() const {
    if (m_track.width() >= HIT_WIDTH) return m_track;
    const int extra = HIT_WIDTH - m_track.width();
    return m_track.adjusted(-extra / 2, 0, extra - extra / 2, 0);
}

bool PaintedScroll::overThumb(const QPoint& pos) const {
    if (!scrollable()) return false;
    const QRect hit = hitTrack();
    const QRect t = thumb();
    return hit.contains(pos) && pos.y() >= t.top() && pos.y() <= t.bottom();
}

bool PaintedScroll::contains(const QPoint& pos) const {
    return m_viewport.contains(pos) || (scrollable() && hitTrack().contains(pos));
}

bool PaintedScroll::setValue(int value) {
    value = std::clamp(value, 0, m_maximum);
    if (value == m_value) return false;
    m_value = value;
    m_owner->update();
    return true;
}

void PaintedScroll::setThumbHovered(bool hovered) {
    if (hovered == m_thumbHovered) return;
    m_thumbHovered = hovered;
    m_owner->update();
}

void PaintedScroll::paint(QPainter& painter, const QColor& trackColor, const QColor& thumbColor,
                          int thumbOutset) const {
    if (!scrollable()) return;
    painter.fillRect(m_track, trackColor);
    QRect t = thumb().adjusted(-thumbOutset, 0, thumbOutset, 0);
    if (t.width() < 1) t = QRect(m_track.center().x(), t.top(), 1, t.height());
    QColor color = thumbColor;
    if (highlighted()) {
        // Brighter and slightly wider, so it reads as grabbable.
        color = color.lighter(m_drag == Drag::Thumb ? 135 : 120);
        t.adjust(-1, 0, 1, 0);
    }
    painter.fillRect(t, color);
}

bool PaintedScroll::mousePress(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return false;
    const QPoint pos = event->position().toPoint();
    if (scrollable() && hitTrack().contains(pos)) {
        if (overThumb(pos)) {
            m_drag = Drag::Thumb;
            m_pressPos = pos;
            m_pressValue = m_value;
            m_owner->update();
        } else {
            // Page towards the click, like a regular scroll bar.
            const int page = std::max(m_step, m_viewport.height() - m_step);
            setValue(m_value + (pos.y() < thumb().top() ? -page : page));
            setThumbHovered(overThumb(pos));
        }
        return true;
    }
    if (m_contentDrag && scrollable() && m_viewport.contains(pos)) {
        m_drag = Drag::ContentPending;
        m_pressPos = pos;
        m_pressValue = m_value;
        return true;
    }
    return false;
}

bool PaintedScroll::mouseMove(QMouseEvent* event) {
    const QPoint pos = event->position().toPoint();
    switch (m_drag) {
        case Drag::Thumb: {
            const int range = m_track.height() - thumb().height();
            if (range > 0) {
                const qint64 delta = qint64(pos.y() - m_pressPos.y()) * m_maximum / range;
                setValue(m_pressValue + int(delta));
            }
            return true;
        }
        case Drag::ContentPending:
            if ((pos - m_pressPos).manhattanLength() < QApplication::startDragDistance()) return true;
            m_drag = Drag::Content;
            m_owner->setCursor(Qt::ClosedHandCursor);
            [[fallthrough]];
        case Drag::Content:
            setValue(m_pressValue - (pos.y() - m_pressPos.y()));
            return true;
        case Drag::None:
            break;
    }
    setThumbHovered(overThumb(pos));
    return false;
}

bool PaintedScroll::mouseRelease(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || m_drag == Drag::None) return false;
    if (m_drag == Drag::Content) m_owner->unsetCursor();
    m_drag = Drag::None;
    const QPoint pos = event->position().toPoint();
    m_thumbHovered = overThumb(pos);
    m_owner->update();
    return true;
}

bool PaintedScroll::wheel(QWheelEvent* event, bool force) {
    if (!force && !contains(event->position().toPoint())) return false;
    const int delta = event->angleDelta().y() * m_step / 120;
    setValue(m_value - delta);
    event->accept();
    return true;
}

void PaintedScroll::leave() {
    if (m_drag == Drag::None) setThumbHovered(false);
}
}

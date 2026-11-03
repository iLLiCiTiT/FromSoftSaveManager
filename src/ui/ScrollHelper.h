#pragma once

#include <QColor>
#include <QPoint>
#include <QRect>

class QMouseEvent;
class QPainter;
class QWheelEvent;
class QWidget;

namespace fssm::ui {
// Vertical scrolling for a custom-painted list or grid. The owner widget sets
// the geometry from its layout (or paint) code, draws the content offset by
// value() and forwards its mouse events. The helper handles the wheel,
// dragging the thumb, paging on a click on the track and, when enabled,
// left-button dragging of the content itself.
class PaintedScroll {
public:
    explicit PaintedScroll(QWidget* owner);

    // viewport: where the content scrolls; track: the painted scroll bar (it
    // stays put, so it is anchored to the panel's edge, not to the content).
    // Clamps the current value to the new range.
    void setGeometry(const QRect& viewport, const QRect& track, int contentHeight);
    // Nothing to scroll (e.g. no character); keeps the value.
    void clear();
    void reset() { m_value = 0; }

    int value() const { return m_value; }
    int maximum() const { return m_maximum; }
    bool scrollable() const { return m_maximum > 0; }
    const QRect& viewport() const { return m_viewport; }
    const QRect& track() const { return m_track; }
    QRect thumb() const;
    // Thumb under the mouse or being dragged.
    bool highlighted() const { return m_thumbHovered || m_drag == Drag::Thumb; }
    // Over the content or the scroll bar.
    bool contains(const QPoint& pos) const;
    bool draggingContent() const { return m_drag == Drag::Content; }

    bool setValue(int value);
    // Pixels per wheel notch; also the base of a track click's page.
    void setStep(int step) { m_step = step; }
    void setMinimumThumbHeight(int height) { m_minThumb = height; }
    // Left-button drag on the viewport scrolls; only for content without clicks.
    void setContentDrag(bool enabled) { m_contentDrag = enabled; }

    // Draws the track and the thumb, the thumb brighter while highlighted.
    // thumbOutset widens (or with a negative value narrows) the thumb on both sides.
    void paint(QPainter& painter, const QColor& trackColor, const QColor& thumbColor, int thumbOutset = 0) const;

    // Return true when the helper used the event; the owner then skips its own handling.
    bool mousePress(QMouseEvent* event);
    bool mouseMove(QMouseEvent* event);
    bool mouseRelease(QMouseEvent* event);
    // Scrolls when the pointer is over this area (or force is set).
    bool wheel(QWheelEvent* event, bool force = false);
    void leave();

private:
    enum class Drag { None, Thumb, ContentPending, Content };
    QRect hitTrack() const;
    bool overThumb(const QPoint& pos) const;
    void setThumbHovered(bool hovered);

    QWidget* m_owner;
    QRect m_viewport;
    QRect m_track;
    int m_value = 0;
    int m_maximum = 0;
    int m_step = 24;
    int m_minThumb = 16;
    bool m_contentDrag = false;
    bool m_thumbHovered = false;
    Drag m_drag = Drag::None;
    QPoint m_pressPos;
    int m_pressValue = 0;
};
}

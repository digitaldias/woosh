#include "WaveformView.h"
#include <QPainter>
#include <QPaintEvent>

WaveformView::WaveformView(QWidget* parent) : QWidget(parent) {}

void WaveformView::setPlaceholderText(const QString& text) {
    placeholder_ = text;
    update();
}

void WaveformView::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(event->rect(), palette().base());
    painter.setPen(Qt::gray);
    painter.drawRect(rect().adjusted(1, 1, -2, -2));
    painter.drawText(rect(), Qt::AlignCenter, placeholder_.isEmpty() ? "Waveform placeholder" : placeholder_);
}



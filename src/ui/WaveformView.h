#pragma once

#include <QWidget>
#include <QString>

class WaveformView : public QWidget {
    Q_OBJECT
public:
    explicit WaveformView(QWidget* parent = nullptr);
    void setPlaceholderText(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString placeholder_;
};




#ifndef WIDGET_H
#define WIDGET_H

#include "qcustomplot.h"
#include "qdsp.h"

#include <QSharedPointer>
#include <QTimer>
#include <QVector>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void updateGraphs();
    void updateWindow(int index);
    void updateFftSize(int index);

private:
    dsp::QComplexSignal prepareSignal();
    void updateKeys();
    void setupGraphs();

private:
    Ui::Widget *ui;

    QVector<int> fftSizes_;
    dsp::QFft *fft_;
    QVector<QSharedPointer<dsp::QAbstractWindow>> windowFunctions_;
    dsp::QRealSignal window_;

    QTimer *mainTimer_;

    QVector<double> timeKeys_;
    QVector<double> windowedTimeKeys_;
    QVector<double> frequencyKeys_;

    QCPGraph *iGraph_;
    QCPGraph *qGraph_;
    QCPGraph *windowedIGraph_;
    QCPGraph *windowedQGraph_;

    QCPGraph *magnitudeGraph_;
    QCPGraph *phaseGraph_;

    QCPColorMap *spectrogram_;

};
#endif // WIDGET_H

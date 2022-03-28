#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qcustomplot.h"
#include "qdsp.h"
#include "processors.h"

#include <QMainWindow>
#include <QTimer>
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void newTimeDomainTask(const QComplexSignal&);
    void newFrequencyDomainTask(const QComplexSignal&);
    void pulseChanged(const QComplexSignal&);
    void periodChanged(int);

private slots:
    void plotSignal(const QComplexSignal& data);
    void updatePulse();
    void updateSignal();
    void updateSignalNoise();
    void resetControlsConfiguration();

private:
    void plotPulse();
    void updateAxes();
    void setupControls();
    void setupGraphs();
    void setupProcessors();
    void setupWindowFunctions();

private:
    // Constants
    static constexpr int minPulseSize = 20;
    static constexpr int maxPulseSize = 128;
    static constexpr int defaultPulseSize = 40;

    static constexpr int minFrequencyDeviationPercent = 0;
    static constexpr int maxFrequencyDeviationPercent = 100;
    static constexpr int defaultFrequencyDeviationPercent = 30;

    static constexpr int minPulsePeriodPower = 8;
    static constexpr int maxPulsePeriodPower = 16;
    static constexpr int defaultPulsePeriodPower = 9;
    // 1 << n equals pow(2, n);
    static constexpr int defaultPulsePeriod = 1 << defaultPulsePeriodPower;

    static constexpr int minPulseDelay = 0;
    static constexpr int maxPulseDelay = (1 << (defaultPulsePeriodPower - 1)) - 1;
    static constexpr int defaultPulseDelay = minPulseDelay;

    static constexpr int minPeriodCount = 1;
    static constexpr int maxPeriodCount = 100;
    static constexpr int defaultPeriodCount = 50;

    static constexpr double defaultNoiseDispersion = 0.01;

    static constexpr double signalMaxVoltage = 1.0;
    static constexpr double voltageDisplayFactor = 1.5;
    static constexpr double compressedPulseTimeDisplayFactor = 2;
    static constexpr double signalTimeDisplayFactor = 3;

    static const QString voltageAxisCaption;
    static const QString voltageMagnitudeAxisCaption;
    static const QString timeAxisCaption;

    static const QCPRange linearRange;
    static const QCPRange logarithmicRange;

private:
    Ui::MainWindow *ui;

    QVector<QSharedPointer<dsp::QAbstractWindow>> windowFunctions_;

    int pulsePeriod_ = 0;

    dsp::QComplexSignal pulse_;
    dsp::QRealSignal window_;
    dsp::QComplexSignal windowedPulse_;
    dsp::QComplexSignal compressedPulse_;
    dsp::QComplexSignal signal_;
    dsp::QComplexSignal signalNoise_;
    dsp::QComplexSignal compressedSignal_;

    QCPAxisRect* compressedPulseAxisRect_;
    QCPGraph *iPulseGraph_;
    QCPGraph *qPulseGraph_;
    QVector<double> pulseTimeKeys_;
    QCPGraph *compressedPulseGraph_;
    QVector<double> compressedPulseTimeKeys_;

    QCPAxisRect* compressedSignalAxisRect_;
    QCPGraph *iSignalGraph_;
    QCPGraph *qSignalGraph_;
    QCPGraph *comressedSignalGraph_;
    QVector<double> signalTimeKeys_;

    QThread *processingThread_;
    QTimeDomainProcessor *timeDomainProcessor_;
    QFrequencyDomainProcessor *frequencyDomainProcessor_;

    QTimer *fpsTimer_;
    int fpsCounter_ = 0;
    QLabel *fpsLabel_;
};

#endif // MAINWINDOW_H

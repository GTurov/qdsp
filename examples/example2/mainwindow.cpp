#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace dsp;

const QString MainWindow::voltageAxisCaption = "Voltage, V";
const QString MainWindow::voltageMagnitudeAxisCaption = "Magnitude, dBV";
const QString MainWindow::timeAxisCaption = "Time, samples";

const QCPRange MainWindow::linearRange = QCPRange(-signalMaxVoltage/5, signalMaxVoltage * voltageDisplayFactor);
const QCPRange MainWindow::logarithmicRange = QCPRange(5, -100);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Setup QCustomplot instance
    // Visit https://www.qcustomplot.com for reference
    setupGraphs();

    // Signal-slot support for QComplexSignal
    qRegisterMetaType<QComplexSignal>("QComplexSignal");

    // Window functions for better radar resolution
    setupWindowFunctions();

    // Convolution processors and thread
    setupProcessors();

    // Sliders and combo boxes setup
    setupControls();

    // FPS counter uses counter and timer wich prints counter value and resets
    // it every second
    fpsLabel_ = new QLabel;
    ui->statusbar->addWidget(fpsLabel_);

    fpsTimer_ = new QTimer(this);
    connect(fpsTimer_, &QTimer::timeout, [&](){
        fpsLabel_->setText("FPS: " + QString::number(fpsCounter_));
        fpsCounter_ = 0;
    });

    fpsTimer_->start(1000);

    // Generate default pulse and received signal
    updatePulse();
    // Adds some noise and runs the signal processing loop
    updateSignalNoise();
}

MainWindow::~MainWindow()
{
    // Stops the processing thread
    processingThread_->quit();
    // Wait for the thread has finished execution
    processingThread_->wait();
    delete ui;
}

void MainWindow::plotPulse()
{
    // Source pulse
    iPulseGraph_->setData(pulseTimeKeys_, pulse_.i().asQVector());
    qPulseGraph_->setData(pulseTimeKeys_, pulse_.q().asQVector());

    // Compressed pulse
    if (ui->scaleComboBox->currentIndex() == 0) {
        // Linear scale
        compressedPulseGraph_->setData(compressedPulseTimeKeys_, abs(compressedPulse_).asQVector());
    } else {
        // Logarithmic scale evaluated as 20*log10(abs(value))
        QRealSignal r = abs(compressedPulse_);
        std::transform(r.begin(), r.end(),r.begin(), [](auto x){ return factorToDb(x); });
        compressedPulseGraph_->setData(compressedPulseTimeKeys_, r.asQVector());
    }
}

void MainWindow::plotSignal(const QComplexSignal& data)
{
    // If signal time axis keys has different size, update them
    if (signalTimeKeys_.size() != data.size()) {
        signalTimeKeys_.resize(data.size());
        for (int i = 0; i < signalTimeKeys_.size(); ++i) {
            signalTimeKeys_[i] = i;
        }
    }

    // Received signal
    iSignalGraph_->setData(signalTimeKeys_, signalNoise_.i().asQVector());
    qSignalGraph_->setData(signalTimeKeys_, signalNoise_.q().asQVector());

    // Compressed signal
    if (ui->scaleComboBox->currentIndex() == 0) {
        // Linear range
        comressedSignalGraph_->setData(signalTimeKeys_, abs(data).asQVector());
    } else {
        // Logarithmic scale evaluated as 20*log10(abs(value))
        QRealSignal r = abs(data);
        std::transform(r.begin(), r.end(),r.begin(), [](auto x){ return factorToDb(x); });
        comressedSignalGraph_->setData(signalTimeKeys_, r.asQVector());
    }

    // All graphs data updated, it's should to repaint them
    ui->plot->replot();

    // Increses the FPS counter when signal has been processed
    ++fpsCounter_;

    // Runs the new signal processing
    updateSignalNoise();
}

void MainWindow::updateAxes()
{
    // Updates axes ranges and labels
    if (ui->scaleComboBox->currentIndex() == 0) {
        compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setRange(linearRange);
        compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);

        compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setRange(linearRange);
        compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    } else {
        compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setRange(logarithmicRange);
        compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setLabel(voltageMagnitudeAxisCaption);

        compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setRange(logarithmicRange);
        compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setLabel(voltageMagnitudeAxisCaption);
    }

    // Axes scale changed, it is nessessuary to replot the compressed pulse.
    // Compressed signal will been updated by loop
    plotPulse();
}

void MainWindow::updatePulse()
{
    // Generates chirp
    pulse_ = QComplexSignal::chirp(ui->chirpSizeSlider->value(),
                                   -0.5 * (double)ui->deviationSlider->value()/100,
                                   0.5 * (double)ui->deviationSlider->value()/100);

    // Generates keys for source pulse time-axis
    pulseTimeKeys_.resize(pulse_.size());
    for (int i = 0; i < pulse_.size(); ++i) {
        pulseTimeKeys_[i] = i;
    }

    windowedPulse_ = pulse_;
    // Complex number convolution needs one complex conjugated argument
    std::for_each(windowedPulse_.begin(), windowedPulse_.end(), [](QComplex& x){ x = std::conj(x); });
    // For better resolution pulse should be windowed
    windowedPulse_ *= windowFunctions_.at(ui->windowComboBox->currentIndex())->makeWindow(windowedPulse_.size());

    // Signal for processors update
    emit pulseChanged(windowedPulse_);

    // For single compressed pulse demonstration used only time domain convolution
    compressedPulse_ = convolution(pulse_, windowedPulse_, true, true);
    // Normalizing
    compressedPulse_ /= pulse_.size();

    // Generates keys for compressed pulse time-axis
    compressedPulseTimeKeys_.resize(compressedPulse_.size());
    for (int i = 0; i < compressedPulseTimeKeys_.size(); ++i) {
        compressedPulseTimeKeys_[i] = i;
    }

    // Plots pulse graph
    plotPulse();

    // Generates signal based on new pulse
    updateSignal();
}

void MainWindow::updateSignal()
{
    signal_.resize(pulsePeriod_ * ui->periodCountSlider->value());
    signal_.fill(0);
    auto pulseStart = signal_.begin();
    for(int i = 0; i < ui->periodCountSlider->value(); ++i) {
        std::copy(pulse_.begin(), pulse_.end(), pulseStart + ui->pulseDelaySlider->value());
        std::advance(pulseStart, pulsePeriod_);
    }
}

void MainWindow::updateSignalNoise()
{
    signalNoise_ = signal_ + QComplexSignal::gaussianNoise(signal_.size(), (double)ui->noiseDispersionSlider->value()/ui->noiseDispersionSlider->maximum());
    if (ui->methodComboBox->currentIndex() == 0) {
        emit newTimeDomainTask(signalNoise_);
    } else {
        emit newFrequencyDomainTask(signalNoise_);
    }
}

void MainWindow::resetControlsConfiguration()
{
    // Default controls configuration
    ui->chirpSizeSlider->setValue(defaultPulseSize);
    ui->deviationSlider->setValue(defaultFrequencyDeviationPercent);
    ui->pulseDelaySlider->setValue(defaultPulseDelay);
    ui->periodSizeSlider->setValue(defaultPulsePeriodPower);
    ui->periodCountSlider->setValue(defaultPeriodCount);
    ui->noiseDispersionSlider->setValue(defaultNoiseDispersion*ui->noiseDispersionSlider->maximum());
    ui->windowComboBox->setCurrentIndex(0);
    ui->methodComboBox->setCurrentIndex(0);
    ui->scaleComboBox->setCurrentIndex(0);
}

void MainWindow::setupControls()
{
    // Window function names combobox setup
    for (const auto window: windowFunctions_) {
        ui->windowComboBox->addItem(window->name());
    }

    // Initializes controls ranges
    ui->chirpSizeSlider->setRange(minPulseSize, maxPulseSize);

    ui->deviationSlider->setRange(minFrequencyDeviationPercent, maxFrequencyDeviationPercent);

    ui->pulseDelaySlider->setRange(minPulseDelay, maxPulseDelay);

    ui->periodSizeSlider->setRange(minPulsePeriodPower, maxPulsePeriodPower);
    connect(ui->periodSizeSlider, &QSlider::valueChanged,
            [&](int size) {
        pulsePeriod_ = pow(2, size);
        ui->periodSizeLabel->setNum((int)pulsePeriod_);
        emit MainWindow::periodChanged(pulsePeriod_);
    });

    ui->periodCountSlider->setRange(minPeriodCount, maxPeriodCount);

    connect(ui->noiseDispersionSlider, &QSlider::valueChanged,
            [&](int value){
        // Used fixed length fractional part to avoid label width jitter
        ui->noiseDeviationLabel->setText(QString::number((double)value/ui->noiseDispersionSlider->maximum(), 'f', 3));
    });

    // Setup default controls values
    resetControlsConfiguration();

    // Controls values changing will been update the pulse and signal
    connect(ui->chirpSizeSlider, qOverload<int>(&QSlider::valueChanged),
            this, &MainWindow::updatePulse);

    connect(ui->deviationSlider, qOverload<int>(&QSlider::valueChanged),
            this, &MainWindow::updatePulse);

    connect(ui->pulseDelaySlider, qOverload<int>(&QSlider::valueChanged),
            this, &MainWindow::updateSignal);

    connect(ui->periodSizeSlider, qOverload<int>(&QSlider::valueChanged),
            this, &MainWindow::updateSignal);

    connect(ui->periodCountSlider, qOverload<int>(&QSlider::valueChanged),
            this, &MainWindow::updateSignal);

    connect(ui->windowComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::updatePulse);

    connect(ui->scaleComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateAxes);

    connect(ui->resetPushButton, &QPushButton::clicked,
            this, &MainWindow::resetControlsConfiguration);
}

void MainWindow::setupGraphs()
{
    // Visit https://www.qcustomplot.com for reference
    // Fonts, labels and colors
    const QFont captionFont = QFont("arial", 10, QFont::Bold);
    const QFont axisFont = QFont("arial");

    const QColor blue = QColor(32, 159, 223);
    const QColor green = QColor(153, 202, 83);

    // Allow zoom and drag
    ui->plot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    // Manual legend creating
    ui->plot->setAutoAddPlottableToLegend(false);
    // Clear standart plot
    ui->plot->plotLayout()->clear();


    // Source pulse label
    QCPTextElement* sourcePulseLabel = new QCPTextElement(ui->plot);
    sourcePulseLabel->setText("Source pulse");
    sourcePulseLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(0, 0, sourcePulseLabel);

    // Source pulse axis rectangle
    QCPAxisRect* sourcePulseAxisRect = new QCPAxisRect(ui->plot);
    sourcePulseAxisRect->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    sourcePulseAxisRect->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    sourcePulseAxisRect->axis(QCPAxis::atLeft)->setRange(-signalMaxVoltage * voltageDisplayFactor,
                                                 signalMaxVoltage * voltageDisplayFactor);
    sourcePulseAxisRect->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    sourcePulseAxisRect->axis(QCPAxis::atBottom)->setLabel(timeAxisCaption);
    sourcePulseAxisRect->axis(QCPAxis::atBottom)->setRange(0, maxPulseSize);

    ui->plot->plotLayout()->addElement(1, 0, sourcePulseAxisRect);

    // In-phase source pulse graph
    iPulseGraph_ = ui->plot->addGraph(
                sourcePulseAxisRect->axis(QCPAxis::atBottom),
                sourcePulseAxisRect->axis(QCPAxis::atLeft));
    iPulseGraph_->setPen(QPen(blue));
    iPulseGraph_->setName("I");

    // Quadrature source pulse graph
    qPulseGraph_ = ui->plot->addGraph(
                sourcePulseAxisRect->axis(QCPAxis::atBottom),
                sourcePulseAxisRect->axis(QCPAxis::atLeft));
    qPulseGraph_->setPen(QPen(green));
    qPulseGraph_->setName("Q");

    // Legend with pulse graphs description
    QCPLegend *sourcePulseLegend = new QCPLegend;
    sourcePulseAxisRect->insetLayout()->addElement(sourcePulseLegend, Qt::AlignTop|Qt::AlignRight);
    sourcePulseLegend->setLayer("legend");
    sourcePulseLegend->addItem(new QCPPlottableLegendItem(sourcePulseLegend, iPulseGraph_));
    sourcePulseLegend->addItem(new QCPPlottableLegendItem(sourcePulseLegend, qPulseGraph_));

    // Received signal label
    QCPTextElement* receivedSignalLabel = new QCPTextElement(ui->plot);
    receivedSignalLabel->setText("Received signal");
    receivedSignalLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(2, 0, receivedSignalLabel);

    // Received signal axis rectangle
    QCPAxisRect* receivedSignalAxisRect = new QCPAxisRect(ui->plot);
    receivedSignalAxisRect->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    receivedSignalAxisRect->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    receivedSignalAxisRect->axis(QCPAxis::atLeft)->setRange(-signalMaxVoltage * voltageDisplayFactor,
                                                 signalMaxVoltage * voltageDisplayFactor);
    receivedSignalAxisRect->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    receivedSignalAxisRect->axis(QCPAxis::atBottom)->setLabel(timeAxisCaption);
    receivedSignalAxisRect->axis(QCPAxis::atBottom)->setRange(0, signalTimeDisplayFactor*defaultPulsePeriod);

    ui->plot->plotLayout()->addElement(3, 0, receivedSignalAxisRect);

    // In-phase received signal graph
    iSignalGraph_ = ui->plot->addGraph(
                receivedSignalAxisRect->axis(QCPAxis::atBottom),
                receivedSignalAxisRect->axis(QCPAxis::atLeft));
    iSignalGraph_->setPen(QPen(blue));
    iSignalGraph_->setName("I");

    // Quadrature received signal graph
    qSignalGraph_ = ui->plot->addGraph(
                receivedSignalAxisRect->axis(QCPAxis::atBottom),
                receivedSignalAxisRect->axis(QCPAxis::atLeft));
    qSignalGraph_->setPen(QPen(green));
    qSignalGraph_->setName("Q");

    // Legend with received signal graphs description
    QCPLegend *receivedSignalLegend = new QCPLegend;
    receivedSignalAxisRect->insetLayout()->addElement(receivedSignalLegend, Qt::AlignTop|Qt::AlignRight);
    receivedSignalLegend->setLayer("legend");
    receivedSignalLegend->addItem(new QCPPlottableLegendItem(receivedSignalLegend, iSignalGraph_));
    receivedSignalLegend->addItem(new QCPPlottableLegendItem(receivedSignalLegend, qSignalGraph_));

    // Compressed pulse label
    QCPTextElement* compressedPulseLabel = new QCPTextElement(ui->plot);
    compressedPulseLabel->setText("Compressed pulse");
    compressedPulseLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(0, 1, compressedPulseLabel);

    // Compressed pulse axis rectangle
    compressedPulseAxisRect_ = new QCPAxisRect(ui->plot);
    compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    compressedPulseAxisRect_->axis(QCPAxis::atLeft)->setRange(linearRange);
    compressedPulseAxisRect_->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    compressedPulseAxisRect_->axis(QCPAxis::atBottom)->setLabel(timeAxisCaption);
    compressedPulseAxisRect_->axis(QCPAxis::atBottom)->setRange(0, compressedPulseTimeDisplayFactor*maxPulseSize);

    ui->plot->plotLayout()->addElement(1, 1, compressedPulseAxisRect_);

    // Compressed pulse graph
    compressedPulseGraph_ = ui->plot->addGraph(
                compressedPulseAxisRect_->axis(QCPAxis::atBottom),
                compressedPulseAxisRect_->axis(QCPAxis::atLeft));
    compressedPulseGraph_->setPen(QPen(blue));
    compressedPulseGraph_->setName("Magnitude");

    // Compressed pulse legend
    QCPLegend *compressedPulseLegend = new QCPLegend;
    compressedPulseAxisRect_->insetLayout()->addElement(compressedPulseLegend, Qt::AlignTop|Qt::AlignRight);
    compressedPulseLegend->setLayer("legend");
    compressedPulseLegend->addItem(new QCPPlottableLegendItem(compressedPulseLegend, compressedPulseGraph_));

    // Compressed signal label
    QCPTextElement* compressedSignalLabel = new QCPTextElement(ui->plot);
    compressedSignalLabel->setText("Compressed signal");
    compressedSignalLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(2, 1, compressedSignalLabel);

    // Compressed signal axis rectangle
    compressedSignalAxisRect_ = new QCPAxisRect(ui->plot);
    compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    compressedSignalAxisRect_->axis(QCPAxis::atLeft)->setRange(linearRange);
    compressedSignalAxisRect_->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    compressedSignalAxisRect_->axis(QCPAxis::atBottom)->setLabel(timeAxisCaption);
    compressedSignalAxisRect_->axis(QCPAxis::atBottom)->setRange(0, signalTimeDisplayFactor*defaultPulsePeriod);

    ui->plot->plotLayout()->addElement(3, 1, compressedSignalAxisRect_);

    // Compressed signal graph
    comressedSignalGraph_ = ui->plot->addGraph(
                compressedSignalAxisRect_->axis(QCPAxis::atBottom),
                compressedSignalAxisRect_->axis(QCPAxis::atLeft));
    comressedSignalGraph_->setPen(QPen(blue));
    comressedSignalGraph_->setName("Magnitude");

    // Compressed signal legend
    QCPLegend *compressedSignalLegend = new QCPLegend;
    compressedSignalAxisRect_->insetLayout()->addElement(compressedSignalLegend, Qt::AlignTop|Qt::AlignRight);
    compressedSignalLegend->setLayer("legend");
    compressedSignalLegend->addItem(new QCPPlottableLegendItem(compressedSignalLegend, comressedSignalGraph_));

    // Connecting axes of two graphs for simultaneous scale change
    connect(receivedSignalAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            compressedSignalAxisRect_->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::setRange));

    connect(compressedSignalAxisRect_->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            receivedSignalAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::setRange));
}

void MainWindow::setupProcessors()
{
    // Initializes processing thread
    processingThread_ = new QThread(this);
    processingThread_->start();

    // Creeates time domain processor and moves it to processing thread
    timeDomainProcessor_ = new QTimeDomainProcessor();
    timeDomainProcessor_->moveToThread(processingThread_);
    connect(this, &MainWindow::newTimeDomainTask, timeDomainProcessor_, &QTimeDomainProcessor::process, Qt::QueuedConnection);
    connect(this, &MainWindow::pulseChanged, timeDomainProcessor_, &QTimeDomainProcessor::setPulse, Qt::QueuedConnection);
    connect(timeDomainProcessor_, &QTimeDomainProcessor::ready, this, &MainWindow::plotSignal, Qt::QueuedConnection);

    // Creeates frequenccy domain processor and moves it to processing thread
    frequencyDomainProcessor_ = new QFrequencyDomainProcessor();
    frequencyDomainProcessor_->moveToThread(processingThread_);
    connect(this, &MainWindow::newFrequencyDomainTask, frequencyDomainProcessor_, &QFrequencyDomainProcessor::process, Qt::QueuedConnection);
    connect(this, &MainWindow::pulseChanged, frequencyDomainProcessor_, &QFrequencyDomainProcessor::setPulse ,Qt::QueuedConnection);
    connect(this, &MainWindow::periodChanged, frequencyDomainProcessor_, &QFrequencyDomainProcessor::setFftSize ,Qt::QueuedConnection);
    connect(frequencyDomainProcessor_, &QFrequencyDomainProcessor::ready, this, &MainWindow::plotSignal, Qt::QueuedConnection);
    connect(processingThread_, &QThread::finished, frequencyDomainProcessor_, &QFrequencyDomainProcessor::deleteLater);
}

void MainWindow::setupWindowFunctions()
{
    // Using QSharedPointer instead raw pointers.
    windowFunctions_ << QSharedPointer<QAbstractWindow>(new QRectangleWindow)
                     << QSharedPointer<QAbstractWindow>(new QBartlettHannWindow)
                     << QSharedPointer<QAbstractWindow>(new QBlackmanWindow)
                     << QSharedPointer<QAbstractWindow>(new QBlackmanHarrisWindow)
                     << QSharedPointer<QAbstractWindow>(new QBohmanWindow)
                     << QSharedPointer<QAbstractWindow>(new QChebyshevWindow)
                     << QSharedPointer<QAbstractWindow>(new QFlattopWindow)
                     << QSharedPointer<QAbstractWindow>(new QGaussianWindow)
                     << QSharedPointer<QAbstractWindow>(new QHammingWindow)
                     << QSharedPointer<QAbstractWindow>(new QHannWindow)
                     << QSharedPointer<QAbstractWindow>(new QKaiserWindow)
                     << QSharedPointer<QAbstractWindow>(new QNuttallWindow)
                     << QSharedPointer<QAbstractWindow>(new QParzenWindow)
                     << QSharedPointer<QAbstractWindow>(new QTriangularWindow)
                     << QSharedPointer<QAbstractWindow>(new QTukeyWindow);
}

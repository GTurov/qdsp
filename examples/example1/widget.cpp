#include "widget.h"
#include "ui_widget.h"

using namespace dsp;

// Constants
constexpr int msecInSec = 1000;
constexpr int usecInSec = 1000000;
constexpr dsp::QFrequency samplingFreq = 10_MHz;
constexpr double timeScaleMax = usecInSec / samplingFreq.Hz();
constexpr double sourceSignalMaxVoltage = 1.0;
constexpr double sourceSignalMaxPhase = 2*M_PI;
constexpr double voltageDisplayFactor = 4.0;
constexpr int fftSizeMin = 16;
constexpr int fftSizeMax = 1024;
constexpr int displayFps = 20;
constexpr int spectrumInitialMax = 10;
constexpr int spectrumInitialMin = -60;
constexpr int spectrogramSize = 100;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , fft_(new QFft)
    , mainTimer_(new QTimer(this))
{
    ui->setupUi(this);

    // Setup QCustomplot instance
    // Visit https://www.qcustomplot.com for reference
    this->setupGraphs();

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

    // Window function names combobox setup
    for (const auto window: windowFunctions_) {
        ui->windowComboBox->addItem(window->name());
    }

    connect(ui->windowComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &Widget::updateWindow);

    // FFT combobox setup
    for (int fftSize = fftSizeMin; fftSize <= fftSizeMax; fftSize = qNextPowerOfTwo(fftSize)) {
        fftSizes_ << fftSize;
        ui->fftSizeComboBox->addItem(QString::number(fftSize));
    }

    connect(ui->fftSizeComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &Widget::updateFftSize);

    // Timescale axis keys setup
    for (int i = 0; i < fftSizeMax; ++i) {
        timeKeys_.append(((double)i / fftSizeMax) * timeScaleMax);
    }

    // Initial interface config
    ui->windowComboBox->setCurrentIndex(0);
    ui->fftSizeComboBox->setCurrentIndex(4);

    // Setup timer
    mainTimer_->setInterval(msecInSec / displayFps);
    connect(mainTimer_, &QTimer::timeout, this, &Widget::updateGraphs);
    mainTimer_->start();
}

Widget::~Widget()
{
    delete fft_;
    delete ui;
}

void Widget::updateGraphs()
{
    // Generates signal + noise
    QComplexSignal source = prepareSignal();

    // using window function before FFT
    QComplexSignal windowedSignal = source * window_;

    // FFT computing
    QComplexSignal spectrum = fft_->compute(windowedSignal);

    // Rotates FFT result for 0 Hz at center using STL algorithms
    std::rotate(spectrum.begin(), spectrum.begin() + spectrum.size()/2, spectrum.end());

    // Prepare magnitude with log scale using STL algorithms
    QRealSignal magnitude( spectrum.size(), spectrum.clock());
    std::transform(spectrum.begin(), spectrum.end(), magnitude.begin(),
                   [](QComplex x){ return factorToDb(abs(x)); });

    // Prepare phase using STL algorithms
    QRealSignal phase(spectrum.size(), spectrum.clock());
    std::transform(spectrum.begin(), spectrum.end(), phase.begin(),
                   [](QComplex x){ return arg(x); });

    // Sets data for signal I & Q graphs
    iGraph_->setData(timeKeys_, source.i().asQVector(), true);
    qGraph_->setData(timeKeys_, source.q().asQVector(), true);

    // Sets data for windowed signal I & Q graphs
    windowedIGraph_->setData(windowedTimeKeys_, windowedSignal.i().asQVector(), true);
    windowedQGraph_->setData(windowedTimeKeys_, windowedSignal.q().asQVector(), true);

    // Sets data for magnitude & phase graphs
    magnitudeGraph_->setData(frequencyKeys_, magnitude.asQVector(), true);
    phaseGraph_->setData(frequencyKeys_, phase.asQVector(), true);

    // Shifts spectrogram
    for (int row = spectrogram_->data()->valueSize()-1; row > 0; --row) {
        for (int col = 0; col < spectrogram_->data()->keySize(); ++col) {
            spectrogram_->data()->setCell(col, row, spectrogram_->data()->cell(col, row-1));
        }
    }

    // Sets new data for lower row of the spectrogram
    for (int i = 0; i < magnitude.size(); ++i) {
        spectrogram_->data()->setCell(i, 0, magnitude[i]);
    }

    // Plots new data on QCustomPlot
    ui->plot->replot();
}

void Widget::updateWindow(int index)
{
    window_ = windowFunctions_[index]->makeWindow(fft_->size());
}

void Widget::updateFftSize(int index)
{
    fft_->setSize(fftSizes_[index]);

    spectrogram_->data()->clear();
    spectrogram_->data()->setSize(fft_->size(), spectrogramSize);
    spectrogram_->data()->setKeyRange(QCPRange(-samplingFreq.MHz()/2,
                                               samplingFreq.MHz()/2));
    spectrogram_->data()->setValueRange(QCPRange(0, timeScaleMax * spectrogramSize));
    spectrogram_->data()->fill(-100);

    updateWindow(ui->windowComboBox->currentIndex());

    // Recalculates X-axis keys
    updateKeys();
}

void Widget::updateKeys()
{
    frequencyKeys_.resize(fft_->size());
    for (int i = 0; i < fft_->size(); ++i) {
        frequencyKeys_[i] = ((double)i/fft_->size() - 0.5) * samplingFreq.MHz();
    }

    windowedTimeKeys_ = timeKeys_.mid(0, fft_->size());
}

QComplexSignal Widget::prepareSignal()
{
    // Generates new sine with user selected amplitude, frequency and phase
    QComplexSignal result = QComplexSignal::harmonic(
                fftSizes_.last(), samplingFreq,
                QFrequency(((double)ui->frequencySlider->value() / ui->frequencySlider->maximum() - 0.5) * samplingFreq),
                (double)ui->amplitudeSlider->value() / ui->amplitudeSlider->maximum() * sourceSignalMaxVoltage,
                (double)ui->phaseSlider->value() / ui->phaseSlider->maximum() * sourceSignalMaxPhase);

    // Additive gaussian noise with user selected dispersion
    QComplexSignal::gaussianNoise(result.begin(), result.size(),
                (double)ui->noiseDispersionSlider->value() / ui->noiseDispersionSlider->maximum());

    return result;
}

void Widget::setupGraphs()
{
    // Visit https://www.qcustomplot.com for reference
    // Fonts, labels and colors
    const QFont captionFont = QFont("arial", 10, QFont::Bold);
    const QFont axisFont = QFont("arial");

    const QString timeAxisCaption = "Time, us";
    const QString voltageAxisCaption = "Voltage, V";
    const QString voltageMagnitudeAxisCaption = "Magnitude, dBV";
    const QString frequencyAxisCaption = "Frequency, MHz";
    const QString phaseAxisCaption = "Phase, rad";

    const QColor blue = QColor(32, 159, 223);
    const QColor green = QColor(153, 202, 83);
    const QColor pink = QColor(255, 200, 223);

    // Allow zoom and drag
    ui->plot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    // Manual legend creating
    ui->plot->setAutoAddPlottableToLegend(false);
    // Clear standart plot
    ui->plot->plotLayout()->clear();


    QCPTextElement* sourceLabel = new QCPTextElement(ui->plot);
    sourceLabel->setText("Signal");
    sourceLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(0, 0, sourceLabel);

    QCPLayoutGrid *signalsLayout = new QCPLayoutGrid;
    ui->plot->plotLayout()->addElement(1, 0, signalsLayout);

    // Source signal Graphs
    QCPAxisRect* sourceAxisRect = new QCPAxisRect(ui->plot);
    sourceAxisRect->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    sourceAxisRect->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    sourceAxisRect->axis(QCPAxis::atLeft)->setRange(-sourceSignalMaxVoltage * voltageDisplayFactor,
                                                 sourceSignalMaxVoltage * voltageDisplayFactor);
    sourceAxisRect->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    sourceAxisRect->axis(QCPAxis::atBottom)->setLabel(timeAxisCaption);
    sourceAxisRect->axis(QCPAxis::atBottom)->setRange(0, timeScaleMax);

    signalsLayout->addElement(0, 0, sourceAxisRect);

    iGraph_ = ui->plot->addGraph(
                sourceAxisRect->axis(QCPAxis::atBottom),
                sourceAxisRect->axis(QCPAxis::atLeft));
    iGraph_->setPen(QPen(blue));
    iGraph_->setName("I");

    qGraph_ = ui->plot->addGraph(
                sourceAxisRect->axis(QCPAxis::atBottom),
                sourceAxisRect->axis(QCPAxis::atLeft));
    qGraph_->setPen(QPen(green));
    qGraph_->setName("Q");

    QCPLegend *sourceLegend = new QCPLegend;
    sourceAxisRect->insetLayout()->addElement(sourceLegend, Qt::AlignTop|Qt::AlignRight);
    sourceLegend->setLayer("legend");
    sourceLegend->addItem(new QCPPlottableLegendItem(sourceLegend, iGraph_));
    sourceLegend->addItem(new QCPPlottableLegendItem(sourceLegend, qGraph_));

    QCPTextElement* windowedLabel = new QCPTextElement(ui->plot);
    windowedLabel->setText("Windowed signal");
    windowedLabel->setFont(captionFont);
    signalsLayout->addElement(1, 0, windowedLabel);

    // Windowed signal Graphs
    QCPAxisRect* windowedAxisRect = new QCPAxisRect(ui->plot);
    windowedAxisRect->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    windowedAxisRect->axis(QCPAxis::atLeft)->setLabel(voltageAxisCaption);
    windowedAxisRect->axis(QCPAxis::atLeft)->setRange(-sourceSignalMaxVoltage * voltageDisplayFactor,
                                                 sourceSignalMaxVoltage * voltageDisplayFactor);
    windowedAxisRect->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    windowedAxisRect->axis(QCPAxis::atBottom)->setLabel(timeAxisCaption);
    windowedAxisRect->axis(QCPAxis::atBottom)->setRange(0, timeScaleMax);

    signalsLayout->addElement(2, 0, windowedAxisRect);

    windowedIGraph_ = ui->plot->addGraph(
                windowedAxisRect->axis(QCPAxis::atBottom),
                windowedAxisRect->axis(QCPAxis::atLeft));
    windowedIGraph_->setPen(QPen(blue));
    windowedIGraph_->setName("I");

    windowedQGraph_ = ui->plot->addGraph(
                windowedAxisRect->axis(QCPAxis::atBottom),
                windowedAxisRect->axis(QCPAxis::atLeft));
    windowedQGraph_->setPen(QPen(green));
    windowedQGraph_->setName("Q");

    QCPLegend *windowedLegend = new QCPLegend;
    windowedAxisRect->insetLayout()->addElement(windowedLegend, Qt::AlignTop|Qt::AlignRight);
    windowedLegend->setLayer("legend");
    windowedLegend->addItem(new QCPPlottableLegendItem(windowedLegend, windowedIGraph_));
    windowedLegend->addItem(new QCPPlottableLegendItem(windowedLegend, windowedQGraph_));

    // Connecting axes of two graphs for simultaneous scale change
    connect(sourceAxisRect->axis(QCPAxis::atLeft), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            windowedAxisRect->axis(QCPAxis::atLeft), qOverload<const QCPRange&>(&QCPAxis::setRange));

    connect(windowedAxisRect->axis(QCPAxis::atLeft), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            sourceAxisRect->axis(QCPAxis::atLeft), qOverload<const QCPRange&>(&QCPAxis::setRange));

    connect(sourceAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            windowedAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::setRange));

    connect(windowedAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            sourceAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::setRange));

    QCPTextElement* fftLabel = new QCPTextElement(ui->plot);
    fftLabel->setText("Spectrum");
    fftLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(0, 1, fftLabel);

    // FFT graphs
    QCPAxisRect* fftAxisRect = new QCPAxisRect(ui->plot);
    fftAxisRect->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    fftAxisRect->axis(QCPAxis::atLeft)->setLabel(voltageMagnitudeAxisCaption);
    fftAxisRect->axis(QCPAxis::atLeft)->setRange(spectrumInitialMin, spectrumInitialMax);
    fftAxisRect->axis(QCPAxis::atRight)->setLabelFont(axisFont);
    fftAxisRect->axis(QCPAxis::atRight)->setLabel(phaseAxisCaption);
    fftAxisRect->axis(QCPAxis::atRight)->setRange(-3*M_PI, 3*M_PI);
    fftAxisRect->axis(QCPAxis::atRight)->setTicker(QSharedPointer<QCPAxisTickerPi>(new QCPAxisTickerPi));
    fftAxisRect->axis(QCPAxis::atRight)->setVisible(true);
    fftAxisRect->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    fftAxisRect->axis(QCPAxis::atBottom)->setLabel(frequencyAxisCaption);
    fftAxisRect->axis(QCPAxis::atBottom)->setRange(-samplingFreq.MHz()/2,
                                                   samplingFreq.MHz()/2);

    ui->plot->plotLayout()->addElement(1, 1, fftAxisRect);

    phaseGraph_ = ui->plot->addGraph(
                fftAxisRect->axis(QCPAxis::atBottom),
                fftAxisRect->axis(QCPAxis::atRight));
    phaseGraph_->setPen(QPen(pink));
    phaseGraph_->setName("Phase");

    magnitudeGraph_ = ui->plot->addGraph(
                fftAxisRect->axis(QCPAxis::atBottom),
                fftAxisRect->axis(QCPAxis::atLeft));
    magnitudeGraph_->setPen(QPen(blue));
    magnitudeGraph_->setName("Magnitude");

    QCPLegend *fftLegend = new QCPLegend;
    fftAxisRect->insetLayout()->addElement(fftLegend, Qt::AlignTop|Qt::AlignRight);
    fftLegend->setLayer("legend");
    fftLegend->addItem(new QCPPlottableLegendItem(fftLegend, magnitudeGraph_));
    fftLegend->addItem(new QCPPlottableLegendItem(fftLegend, phaseGraph_));

    QCPTextElement* spectrogramLabel = new QCPTextElement(ui->plot);
    spectrogramLabel->setText("Spectrogram");
    spectrogramLabel->setFont(captionFont);
    ui->plot->plotLayout()->addElement(0, 2, spectrogramLabel);

    // Spectrogram graph
    QCPLayoutGrid *spectrogramLayout = new QCPLayoutGrid;
    ui->plot->plotLayout()->addElement(1, 2, spectrogramLayout);

    QCPAxisRect* spectrogramAxisRect = new QCPAxisRect(ui->plot);
    spectrogramAxisRect->axis(QCPAxis::atLeft)->setLabelFont(axisFont);
    spectrogramAxisRect->axis(QCPAxis::atLeft)->setLabel(timeAxisCaption);
    spectrogramAxisRect->axis(QCPAxis::atLeft)->setRange(0, timeScaleMax*spectrogramSize);
    spectrogramAxisRect->axis(QCPAxis::atBottom)->setLabelFont(axisFont);
    spectrogramAxisRect->axis(QCPAxis::atBottom)->setLabel(frequencyAxisCaption);
    spectrogramAxisRect->axis(QCPAxis::atBottom)->setRange(-samplingFreq.MHz()/2,
                                                           samplingFreq.MHz()/2);
    spectrogramAxisRect->setRangeDrag(Qt::Orientation::Horizontal);
    spectrogramAxisRect->setRangeZoom(Qt::Orientation::Horizontal);
    spectrogramLayout->addElement(0, 0, spectrogramAxisRect);

    spectrogram_ = new QCPColorMap(
                spectrogramAxisRect->axis(QCPAxis::atBottom),
                spectrogramAxisRect->axis(QCPAxis::atLeft));

    connect(spectrogramAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            fftAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::setRange));

    connect(fftAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            spectrogramAxisRect->axis(QCPAxis::atBottom), qOverload<const QCPRange&>(&QCPAxis::setRange));

    // Spectrogram scale
    QCPColorScale *spectrogramScale = new QCPColorScale(ui->plot);
    spectrogram_->setColorScale(spectrogramScale);
    spectrogram_->setGradient(QCPColorGradient::gpJet);
    spectrogram_->data()->setKeySize(fftSizeMax);
    spectrogram_->data()->setKeyRange(QCPRange(-samplingFreq.MHz()/2,
                                               samplingFreq.MHz()/2));
    spectrogram_->data()->setValueSize(spectrogramSize);
    spectrogram_->data()->setValueRange(QCPRange(0, timeScaleMax * spectrogramSize));

    connect(spectrogramScale, &QCPColorScale::dataRangeChanged,
            fftAxisRect->axis(QCPAxis::atLeft), qOverload<const QCPRange&>(&QCPAxis::setRange));

    connect(fftAxisRect->axis(QCPAxis::atLeft), qOverload<const QCPRange&>(&QCPAxis::rangeChanged),
            spectrogramScale, &QCPColorScale::setDataRange);

    // Aligns spectrogram and spectrogram scale margins
    QCPMarginGroup *spectrogramMarginGroup = new QCPMarginGroup(ui->plot);
    spectrogramAxisRect->setMarginGroup(QCP::msBottom|QCP::msTop, spectrogramMarginGroup);
    spectrogramScale->setMarginGroup(QCP::msBottom|QCP::msTop, spectrogramMarginGroup);
    spectrogramScale->axis()->setLabel(voltageMagnitudeAxisCaption);
    spectrogramScale->axis()->setRange(spectrumInitialMin, spectrumInitialMax);

    spectrogramLayout->addElement(0, 1, spectrogramScale);
}




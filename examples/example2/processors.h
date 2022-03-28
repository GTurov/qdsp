#ifndef PROCESSORS_H
#define PROCESSORS_H

#include "qdsp.h"

#include <QObject>

using namespace dsp;

class QConvolutionProcessor: public QObject
{
    Q_OBJECT
public:
    QConvolutionProcessor() = default;

signals:
    void ready(const QComplexSignal&);

public slots:
    virtual void process(const QComplexSignal& data) = 0;
    virtual void setPulse(const QComplexSignal& pulse) = 0;

protected:
    QComplexSignal result_;
    QComplexSignal pulse_;
};

class QTimeDomainProcessor: public QConvolutionProcessor
{
    Q_OBJECT
public:
    QTimeDomainProcessor() = default;

public slots:
    void process(const QComplexSignal& data) override
    {
        // Standart convolution of two signals
        result_ = convolution(data, pulse_);
        // Normalize the result
        result_ /= pulse_.size();
        emit ready(result_);
    }

    void setPulse(const QComplexSignal& pulse) override { pulse_ = pulse; }
};


class QFrequencyDomainProcessor: public QConvolutionProcessor
{
    Q_OBJECT
public:
    QFrequencyDomainProcessor()
        : iFft_(true)
    {}

public slots:
    void process(const QComplexSignal& data) override
    {
        result_.resize(data.size());
        if (fft_.size() != 0) {
            // Using QComplexSignalFrame for calculating spectrums  of signal parts
            QComplexSignalFrame fr(data, fft_.size());

            // Partial signal processing. One part size is equal FFT size.
            auto currentResultBegin = result_.begin();
            for (int i = 0; i < data.size() / fft_.size(); ++i) {
                // A spectrum of convolution of two signals is multiplication
                // of signals spectrums.
                // Using inverted FFT to restore the result signal
                QComplexSignal r = iFft_(fft_(fr) * pulseSpectrum_);

                // Normalize the result
                r /= (double)pulse_.size() / fft_.size();

                // Copy partial result to result signal
                std::copy(r.begin(), r.end(), currentResultBegin);
                std::advance(currentResultBegin, fft_.size());

                // Shifts the frame
                fr.shift(fft_.size());
            }
        }
        result_.setClock(data.clock());
        emit ready(result_);
    }

    void setPulse(const QComplexSignal& pulse) override
    {
        pulse_ = pulse;
        // Computing the pulse spectrum previously makes processing faster
        pulseSpectrum_ = fft_(pulse_);
    }

    void setFftSize(int size)
    {
        fft_.setSize(size);
        iFft_.setSize(size);
        pulseSpectrum_ = fft_(pulse_);
    }

private:
    QComplexSignal pulseSpectrum_;
    QFft fft_;
    QFft iFft_;
};

#endif // PROCESSORS_H

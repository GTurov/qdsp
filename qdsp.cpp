#include "qdsp.h"

#include <limits>
#include <math.h>
#include <stdexcept>

#include <QDebug>
#include <QRandomGenerator>

namespace dsp {

QString complexToString(dsp::QComplex value)
{
    if (value == dsp::QComplex()) {
        return "0";
    }
    QString result;
    if (value.real() != 0) {
        result += QString::number(value.real());
    }
    if (value.imag() != 0) {
        result += (value.imag() > 0 ? (value.real() != 0 ? "+" : "") : "-");
        if (std::abs(value.imag()) != 1) {
            result += QString::number(std::abs(value.imag()));
        }
        result += "i";
    }
    return result;
}

namespace detail {

double generateRandomDouble()
{
    static QRandomGenerator generator(QDateTime::currentMSecsSinceEpoch());
    static constexpr uint32_t max = std::numeric_limits<uint32_t>::max();

    return 2*(double)generator.generate()/max - 1;
}

std::pair<double, double> generateGaussianPair()
{
    double u = 0;
    double v = 0;
    double s = 0;
    do {
        u = generateRandomDouble();
        v = generateRandomDouble();
        s = u*u + v*v;
    } while (s > 1);
    double r = sqrt(-2 * log(s) / s);
    return {u * r, v * r};
}

} // namespace detail

QRealSignal QRealSignal::harmonic(int size, double normFreq, double magnitude, double phase)
{
    QRealSignal result(size, 0);
    QRealSignal::harmonic(result.begin(), size, normFreq, magnitude, phase);
    return result;
}

QRealSignal QRealSignal::harmonic(int size, QFrequency clock, QFrequency freq, double magnitude, double phase)
{
    QRealSignal result = QRealSignal::harmonic(size, freq/clock, magnitude, phase);
    result.setClock(clock);
    return result;
}

QRealSignal QRealSignal::meander(int size, double normFreq, double magnitude, double phase)
{
    QRealSignal result(size, 0);
    QRealSignal::meander(result.begin(), size, normFreq, magnitude, phase);
    return result;
}

QRealSignal QRealSignal::meander(int size, QFrequency clock, QFrequency freq, double magnitude, double phase)
{
    QRealSignal result = QRealSignal::meander(size, freq/clock, magnitude, phase);
    result.setClock(clock);
    return result;
}

QRealSignal QRealSignal::saw(int size, double normFreq, double magnitude, double phase)
{
    QRealSignal result(size, 0);
    QRealSignal::saw(result.begin(), size, normFreq, magnitude, phase);
    return result;
}

QRealSignal QRealSignal::saw(int size, QFrequency clock, QFrequency freq, double magnitude, double phase)
{
    QRealSignal result = QRealSignal::saw(size, freq/clock, magnitude, phase);
    result.setClock(clock);
    return result;
}

QRealSignal QRealSignal::chirp(int size, double startNormFreq, double stopNormFreq, double magnitude, double phase)
{
    QRealSignal result(size, 0);
    if (result.isEmpty()) {
        return result;
    }
    QRealSignal::chirp(result.begin(), size, startNormFreq, stopNormFreq, magnitude, phase);
    return result;
}

QRealSignal QRealSignal::chirp(int size, QFrequency clock, QFrequency startFreq, QFrequency stopFreq, double magnitude, double phase)
{
    QRealSignal result = QRealSignal::chirp(size, startFreq/clock, stopFreq/clock, magnitude, phase);
    result.setClock(clock);
    return result;
}

QRealSignal QRealSignal::uniformNoise(int size, QFrequency clock, double dispersion, double mean)
{
    QRealSignal result(size, clock, 0);
    QRealSignal::uniformNoise(result.begin(), size, dispersion, mean);
    return result;
}

QRealSignal QRealSignal::uniformNoise(int size, double dispersion, double mean)
{
   return QRealSignal::uniformNoise(size, noFrequency, dispersion, mean);
}

QRealSignal QRealSignal::gaussianNoise(int size, QFrequency clock, double dispersion, double mean)
{
    QRealSignal result(size, clock, 0);
    QRealSignal::gaussianNoise(result.begin(), size, dispersion, mean);
    return result;
}

QRealSignal QRealSignal::gaussianNoise(int size, double dispersion, double mean)
{
   return QRealSignal::gaussianNoise(size, noFrequency, dispersion, mean);
}

QRealSignal QComplexSignal::i() const
{
    QRealSignal result(this->size());
    std::transform(this->begin(), this->end(), result.begin(),
                   [](QComplex x) {return x.real();});
    return result;
}

QRealSignal QComplexSignal::q() const
{
    QRealSignal result(this->size());
    std::transform(this->begin(), this->end(), result.begin(),
                   [](QComplex x) {return x.imag();});
    return result;
}

QComplexSignal QComplexSignal::harmonic(int size, double normFreq, double magnitude, double phase)
{
    QComplexSignal result(size, 0);
    QComplexSignal::harmonic(result.begin(), size, normFreq, magnitude, phase);
    return result;
}

QComplexSignal QComplexSignal::harmonic(int size, QFrequency clock, QFrequency freq, double magnitude, double phase)
{
    QComplexSignal result = QComplexSignal::harmonic(size, freq/clock, magnitude, phase);
    result.setClock(clock);
    return result;
}

QComplexSignal QComplexSignal::chirp(int size, double startNormFreq, double stopNormFreq, double magnitude, double phase)
{
    QComplexSignal result(size, 0);
    if (result.isEmpty()) {
        return result;
    }
    QComplexSignal::chirp(result.begin(), size, startNormFreq, stopNormFreq, magnitude, phase);
    return result;
}

QComplexSignal QComplexSignal::chirp(int size, QFrequency clock, QFrequency startFreq, QFrequency stopFreq, double magnitude, double phase)
{
    QComplexSignal result = QComplexSignal::chirp(size, startFreq/clock, stopFreq/clock, magnitude, phase);
    result.setClock(clock);
    return result;
}

QComplexSignal QComplexSignal::uniformNoise(int size, QFrequency clock, double dispersion, double mean)
{
    QComplexSignal result(size, clock, 0);
    QComplexSignal::uniformNoise(result.begin(), size, dispersion, mean);
    return result;
}

QComplexSignal QComplexSignal::uniformNoise(int size, double dispersion, double mean)
{
   return QComplexSignal::uniformNoise(size, noFrequency, dispersion, mean);
}

QComplexSignal QComplexSignal::gaussianNoise(int size, QFrequency clock, double dispersion, double mean)
{
    QComplexSignal result(size, clock, 0);
    QComplexSignal::gaussianNoise(result.begin(), size, dispersion, mean);
    return result;
}

QComplexSignal QComplexSignal::gaussianNoise(int size, double dispersion, double mean)
{
   return QComplexSignal::gaussianNoise(size, noFrequency, dispersion, mean);
}

inline QDebug& operator<<(QDebug& debug, WaveformDataType dataType)
{
    QDebugStateSaver saver(debug);
    switch (dataType) {
    case WaveformDataType::INT8: debug.nospace() << "INT8"; break;
    case WaveformDataType::INT16: debug.nospace() << "INT16"; break;
    case WaveformDataType::INT32: debug.nospace() << "INT32"; break;
    case WaveformDataType::INT64: debug.nospace() << "INT64"; break;
    case WaveformDataType::FLOAT: debug.nospace() << "FLOAT"; break;
    case WaveformDataType::DOUBLE: debug.nospace() << "DOUBLE"; break;
    default: debug.nospace() << "unknown";
    }
    return debug;
}

//----------------------------Формат файла Waveform-----------------------------
//
// Байты:    0       1       2       3       4       5       6       7
//     0 +-------+-------+-------+-------+-------+-------+-------+-------+
//       |   A   |   B   |       C       |       D       |  Не задейств. |
//     8 +-------+-------+-------+-------+-------+-------+-------+-------+
//       |              Размер сигнала, отсчётов (int64_t)               |
//    16 +-------+-------+-------+-------+-------+-------+-------+-------+
//       |              Частота дискретизации, Гц (double)               |
//    24 +-------+-------+-------+-------+-------+-------+-------+-------+
//       |                  Дата и время, сек (int64_t)                  |
//    32 +-------+-------+-------+-------+-------+-------+-------+-------+
//       |                 Описание, опционально (char[])                |
//       |                              ...                              |
//   ... +-------+-------+-------+-------+-------+-------+-------+-------+
//       |                             Данные                            |
//       |                              ...                              |
//       +-------+-------+-------+-------+-------+-------+-------+-------+
//
//   A - версия формата и размер заголовка, выраженный в блоках по 32 байта.
//       Побитово:
//           7       6       5       4       3       2       1       0
//       +-------+-------+-------+-------+-------+-------+-------+-------+
//       |      Версия формата (0x01)    |    Размер заголовка (0x01)    |
//       +-------+-------+-------+-------+-------+-------+-------+-------+
//
//   B - тип сигнала. Побитово:
//           7       6       5       4       3       2       1       0
//       +-------+-------+-------+-------+-------+-------+-------+-------+
//       |        Зарезервировано        |действ.| целый |  Размерность  |
//       |          под описание         |   /   |   /   |     данных    |
//       |        маркеров сигнала       | компл.| дробн.|  1б/2б/4б/8б  |
//       +-------+-------+-------+-------+-------+-------+-------+-------+
//
//   C - Длина описания, выраженная в блоках по 32 байта. (uint16_t)
//
//   D - отношение пикового значения сигнала в Вольтах к максимальному значению
//       шкалы, выраженное в дБ и округлённое до целого в меньшую сторону,
//       только для целочисленных сигналов. (int16_t)
//------------------------------------------------------------------------------

uint8_t detail::encodeSize(WaveformDataType dataType)
{
    switch (dataType) {
    case WaveformDataType::INT8: return 0x00;
    case WaveformDataType::INT16: return 0x01;
    case WaveformDataType::INT32: return 0x02;
    case WaveformDataType::INT64: return 0x03;
    case WaveformDataType::FLOAT: return 0x02;
    default: return 0x03;
    }
}

WaveformDataType detail::decodeSize(uint8_t dataType, bool isInteger)
{
    if (isInteger) {
        switch (dataType) {
        case 0x00: return WaveformDataType::INT8;
        case 0x01: return WaveformDataType::INT16;
        case 0x02: return WaveformDataType::INT32;
        case 0x03: return WaveformDataType::INT64;
        default: throw;
        }
    } else {
        switch (dataType) {
        case 0x02: return WaveformDataType::FLOAT;
        case 0x03: return WaveformDataType::DOUBLE;
        default: throw;
        }
    }
    throw;
}

void detail::writeWaveformHeader(QIODevice& file, const QWaveformFileInfo& info)
{
    QByteArray header(waveformHeaderConstants::headerSize_, 0);
    header[waveformHeaderConstants::versionOffset_] = 0x11;
    header[waveformHeaderConstants::typeOffset_] = (info.isComplex() & waveformHeaderConstants::isComplexBitMask_) << waveformHeaderConstants::isComplexBitOffset_ |
                                                             (info.isInteger() & waveformHeaderConstants::isIntegerBitMask_) << waveformHeaderConstants::isIntegerBitOffset_ |
                                                             (detail::encodeSize(info.dataType()) & waveformHeaderConstants::dataTypeBitMask_);
    int16_t refLevel = info.refLevel();
    header.replace(waveformHeaderConstants::referenceOffset_, sizeof (int16_t), reinterpret_cast<char*>(&refLevel), sizeof (int16_t));
    int64_t signalSize = info.signalSize();
    header.replace(waveformHeaderConstants::sizeOffset_, sizeof (int64_t), reinterpret_cast<char*>(&signalSize), sizeof (int64_t));
    double clock = info.clock().value();
    header.replace(waveformHeaderConstants::clockOffset_, sizeof (double), reinterpret_cast<char*>(&clock), sizeof (double));
    int64_t dateTime = info.dateTime().toSecsSinceEpoch();
    header.replace(waveformHeaderConstants::dateOffset_, sizeof (int64_t), reinterpret_cast<char*>(&dateTime), sizeof (int64_t));

    QByteArray description = info.comment().toUtf8();
    if (description.size() / waveformHeaderConstants::descriptionBlockSize_ > (int)std::numeric_limits<uint16_t>::max()) {
        description.resize((int)std::numeric_limits<uint16_t>::max()*waveformHeaderConstants::descriptionBlockSize_);
    }
    uint16_t descrSize = std::ceil((double)description.size() / waveformHeaderConstants::descriptionBlockSize_);
    description.append(descrSize*waveformHeaderConstants::descriptionBlockSize_ - description.size(), 0);

    header.replace(waveformHeaderConstants::descriptionSizeOffset_, sizeof (uint16_t), reinterpret_cast<char*>(&descrSize), sizeof (uint16_t));

    if (file.write(header) != header.size()) {
        throw std::runtime_error(detail::msgWriteError.toStdString());
    }
    if (file.write(description) != description.size()) {
        throw std::runtime_error(detail::msgWriteError.toStdString());
    }
}

QWaveformFileInfo detail::readWaveformHeader(QIODevice& file)
{
    QByteArray header = file.read(waveformHeaderConstants::headerSize_);
    if (header.size() != waveformHeaderConstants::headerSize_) {
        throw std::runtime_error(detail::msgInvalidFileFormat.toStdString());
    }
    if ((uint8_t)header[waveformHeaderConstants::versionOffset_] != 0x11) {
        throw std::runtime_error(detail::msgInvalidFileFormat.toStdString());
    }
    QWaveformFileInfo info;
    info.setComplex(header[waveformHeaderConstants::typeOffset_] >> waveformHeaderConstants::isComplexBitOffset_ & waveformHeaderConstants::isComplexBitMask_);
    bool isInteger = header[waveformHeaderConstants::typeOffset_] >> waveformHeaderConstants::isIntegerBitOffset_ & waveformHeaderConstants::isIntegerBitMask_;
    info.setDataType(detail::decodeSize(header[waveformHeaderConstants::typeOffset_] & waveformHeaderConstants::dataTypeBitMask_, isInteger));
    info.setRefLevel(*reinterpret_cast<uint16_t*>(header.data() + waveformHeaderConstants::referenceOffset_));
    info.setSignalSize(*reinterpret_cast<uint64_t*>(header.data() + waveformHeaderConstants::sizeOffset_));
    info.setClock(QFrequency(*reinterpret_cast<double*>(header.data() + waveformHeaderConstants::clockOffset_)));
    info.setDateTime(QDateTime::fromSecsSinceEpoch(*reinterpret_cast<int64_t*>(header.data() + waveformHeaderConstants::dateOffset_)));
    info.setComment(QString::fromUtf8(file.read(*reinterpret_cast<uint16_t*>(header.data() + waveformHeaderConstants::descriptionSizeOffset_) * waveformHeaderConstants::descriptionBlockSize_)));
    return info;
}

QWaveformFileInfo QWaveformFileInfo::analyse(const QString& fileName)
{
    QFile file(fileName);
    if (!file.exists()) {
        throw std::runtime_error(detail::msgFileNotExist.toStdString());
    }
    file.open(QIODevice::ReadOnly);
    if (!file.isReadable()) {
        throw std::runtime_error(detail::msgReadError.toStdString());
    }
    return detail::readWaveformHeader(file);
}

QFft::QFft(int fftSize, bool inverted)
    : fftPower_(floor(log2(fftSize))), inverted_(inverted)
{
    this->updateRotateMultiplers_();
}

void QFft::setPower(int power)
{
    fftPower_ = power;
    this->updateRotateMultiplers_();
}

int QFft::reverseBits_(int number, int bitCount)
{
    int result = 0;
    int saveBits = number;
    for (int i = 1; i <= bitCount; ++i) {
        result = result << 1;
        int nextBit = saveBits & 1;
        result = result | nextBit;
        saveBits = saveBits >> 1;
    }
    return result;
}

void QFft::updateRotateMultiplers_()
{
    if (fftPower_ < 0) {
        throw std::invalid_argument("FFT power must be positive");
    }
    if (fftPower_ == 0) {
        rotateMultiplers_.resize(0);
        buffer_.resize(0);
        return;
    }
    fftSize_ = 0x1<<fftPower_;
    buffer_.resize(fftSize_);

    // Классический расчёт
    rotateMultiplers_.resize(fftSize_/2);
    for (int i = 0; i < fftSize_/2; i++) {
        rotateMultiplers_[i] = QComplex(cos(2*M_PI/fftSize_*i),
                                            -sin(2*M_PI/fftSize_*i));
    }
    // Расчёт через комплексную экспоненту
    // rotateMultiplers_ = QComplexSignal::harmonic(fftSize_/2, -1.0/fftSize_, 1);
}

QComplexSignal QFft::process_(QFrequency resultBandwidth) const
{
    for (int i = fftPower_; i > 0; --i) {
      int pow2i = 1 << i;
      for (int j = 0; j < fftSize_/pow2i; ++j) {
          for (int k = 0; k < pow2i/2; ++k) {
              QComplex a = buffer_[pow2i*j+k] + buffer_[pow2i*j+k+pow2i/2];
              QComplex b = rotateMultiplers_[k*1<<(fftPower_-i)] *
                      (buffer_[pow2i*j+k] - buffer_[pow2i*j+k+pow2i/2]);
              buffer_[pow2i*j+k] = a;
              buffer_[pow2i*j+k+pow2i/2] = b;
          }
      }
    }
    QComplexSignal result_(fftSize_, resultBandwidth);
    for (int i = 0; i < fftSize_; ++i) {
        QComplex x =  buffer_[this->reverseBits_(i, fftPower_)];
        result_[i] = inverted_ ? QComplex(x.imag(), x.real()) : x/(double)fftSize_;
    }
    return result_;
}

const QString QBartlettHannWindow::windowName = "Bartlett-Hann";

QRealSignal QBartlettHannWindow::generate(int size)
{
    constexpr double a0 = 0.62;
    constexpr double a1 = 0.48;
    constexpr double a2 = 0.38;

    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = a0 + a1 * x + a2*cos(2*M_PI * x);
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QBlackmanWindow::windowName = "Blackman";

QRealSignal QBlackmanWindow::generate(int size)
{
    constexpr double denum = 18608;
    constexpr double b0 = 7938.0 / denum;
    constexpr double b1x2 = 9240.0 / denum;
    constexpr double b2x2 = 1430.0 / denum;

    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = b0 +
                b1x2*cos(2*M_PI * x) +
                b2x2*cos(4*M_PI * x);
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QBlackmanHarrisWindow::windowName = "Blackman-Harris";

QRealSignal QBlackmanHarrisWindow::generate(int size)
{
    constexpr double b0 = 0.35875;
    constexpr double b1x2 = 0.48829;
    constexpr double b2x2 = 0.14128;
    constexpr double b3x2 = 0.01168;

    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = b0 +
                b1x2*cos(2*M_PI * x) +
                b2x2*cos(4*M_PI * x) +
                b3x2*cos(6*M_PI * x);
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QBohmanWindow::windowName = "Bohman";

QRealSignal QBohmanWindow::generate(const int size)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = (1 - 2*std::abs(x)) * cos(2*M_PI * x) +
                M_1_PI*sin(2*M_PI * std::abs(x));
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QChebyshevWindow::windowName = "Chebyshev";

QRealSignal QChebyshevWindow::generate(int size, double alpha)
{
    double h = pow(10, -alpha);
    double beta = cosh(acosh(1.0/h) / size);
    QComplexSignal spectrum(size);

    for (int i = 0; i < size; ++i) {
        QComplex x = beta * cos(M_PI * i / size);
        QComplex y = acos(x);
        spectrum[i] = pow(-1, i)*cos((double)size * y) / cosh((double)size * acosh(beta));
    }
    QRealSignal result = dft(spectrum, spectrum.size(), true).i();
    auto it = result.begin();
    double max  = *(++it);
    std::for_each(++it, result.end(), [&max](auto &x){ max = x > max ? x : max; });
    result /= max;
    return result;
}

const QString QFlattopWindow::windowName = "Flat top";

QRealSignal QFlattopWindow::generate(int size)
{
    constexpr double a0 = 1;
    constexpr double a1 = 1.932;
    constexpr double a2 = 1.29;
    constexpr double a3 = 0.388;
    constexpr double a4 = 0.03;
    constexpr double aSum = a0 + a1 + a2 + a3 + a4;

    constexpr double a0n = a0 / aSum;
    constexpr double a1n = a1 / aSum;
    constexpr double a2n = a2 / aSum;
    constexpr double a3n = a3 / aSum;
    constexpr double a4n = a4 / aSum;

    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = a0n +
                a1n*cos(2*M_PI * x) +
                a2n*cos(4*M_PI * x) +
                a3n*cos(6*M_PI * x) +
                a4n*cos(8*M_PI * x);
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QGaussianWindow::windowName = "Gaussian";

QRealSignal QGaussianWindow::generate(int size, double alpha)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step, alpha](qreal& u) {
        u = exp(-pow(2*alpha * x, 2) / 2);
        x += step;
    });
    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QHammingWindow::windowName = "Hamming";

QRealSignal QHammingWindow::generate(int size, double alpha)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step, alpha](qreal& u) {
        u = alpha + (1 - alpha) * cos(2*M_PI * x);
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QHannWindow::windowName = "Hann";

QRealSignal QHannWindow::generate(int size)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = (1 + cos(2*M_PI * x)) / 2;
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QKaiserWindow::windowName = "Kaiser";

QRealSignal QKaiserWindow::generate(int size, double alpha)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step, alpha](qreal& u) {
        u = std::cyl_bessel_i(0, M_PI * alpha * sqrt(1 - 2*2*x*x)) /
                std::cyl_bessel_i(0, M_PI * alpha);

        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QNuttallWindow::windowName = "Nuttall";

QRealSignal QNuttallWindow::generate(int size)
{
    constexpr double b0 = 0.355768;
    constexpr double b1x2 = 0.487396;
    constexpr double b2x2 = 0.144232;
    constexpr double b3x2 = 0.012604;

    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;

    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = b0 +
                b1x2*cos(2*M_PI * x) +
                b2x2*cos(4*M_PI * x) +
                b3x2*cos(6*M_PI * x);
        x += step;
    });

    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QParzenWindow::windowName = "Parzen";

QRealSignal QParzenWindow::generate(int size)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;
    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        if (std::abs(x) <= 1.0/4) {
            u = 1 - 6 * (4*x*x) * (1 - 2*std::abs(x));
        } else {
            u = 2 * pow(1 - 2 * std::abs(x), 3);
        }
        x += step;
    });
    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QTriangularWindow::windowName = "Triangular";

QRealSignal QTriangularWindow::generate(int size)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;
    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step](qreal& u) {
        u = 1.0 + 2 * x;
        x += step;
    });
    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QTukeyWindow::windowName = "Tukey";

QRealSignal QTukeyWindow::generate(int size, double alpha)
{
    QRealSignal result(size);
    double step = 1.0 / (size - 1);
    double x = -0.5;
    std::for_each(result.begin(), result.begin() + size / 2 + size % 2,
                  [&x, step, alpha](qreal& u) {
        if (std::abs(x) <= alpha / 2) {
            u = 1;
        } else {
            u = 0.5 * (1 + cos(2*M_PI / alpha * (x - alpha/2)));
        }
        x += step;
    });
    std::copy_n(result.begin(), size/2, result.rbegin());
    return result;
}

const QString QRectangleWindow::windowName = "Rectangle";

QRealSignal QRectangleWindow::generate(int size)
{
    return QRealSignal(size, 1);
}

namespace modulation {

QRealSignal amplitude(const QRealSignal& source, double M, double normFreq, double U, double phase)
{
    QRealSignal result(source.size(), source.clock());
    int i = 0;
    std::transform(source.begin(), source.end(), result.begin(),[normFreq, U, M, phase, &i](double x){
        return  U * (1 + M*x) * sin(2*M_PI*normFreq*(i++) + phase);
    });
    return result;
}

QRealSignal frequency(const QRealSignal& source, double m, double normFreq, double U, double phase)
{
    QRealSignal result(source.size(), source.clock());
    int i = 0;
    double integral = 0;
    std::transform(source.begin(), source.end(), result.begin(),[normFreq, U, m, phase, &i, &integral](double x){
        integral += x;
        return  U * sin(2*M_PI*normFreq*(i++) + m*2*M_PI*normFreq*integral + phase);
    });
    return result;
}

QRealSignal phase(const QRealSignal& source, double m, double normFreq, double U, double phase)
{
    QRealSignal result(source.size(), source.clock());
    int i = 0;
    std::transform(source.begin(), source.end(), result.begin(),[normFreq, U, m, phase, &i](double x){
        return  U * sin(2*M_PI*normFreq*(i++) + 2*M_PI*m*x+phase);
    });
    return result;
}

} // namespace modulation

} // namespace dsp

QDebug operator<<(QDebug debug, dsp::QComplex value)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << dsp::complexToString(value);
    return debug;
}

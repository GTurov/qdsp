#ifndef QDSP_H
#define QDSP_H

// uncomment to enable warnings
//#define QDSP_ENABLE_DEEP_COPY_WARNING

// uncomment to enable warnings
//#define QDSP_ENABLE_SIZE_WARNING

// uncomment to enable warnings
//#define QDSP_ENABLE_CLOCK_WARNING

// uncomment to use 32-bit discrete.
//#define QDSP_FLOAT_DISCRETE

#include <complex>
#include <numeric>

#include <QtMath>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QVector>
#include <QString>

namespace dsp {

namespace detail {

static const QString msgDeepCopyWarning = "Deep copy warning!";
static const QString msgFileNotExist = "file does not exists";
static const QString msgInvalidFileName = "invalid file name";
static const QString msgInvalidFileFormat = "invalid file format";
static const QString msgNoClockError = "clock must be positive";
static const QString msgReadError = "read error";
static const QString msgWriteError = "write error";

} // namespace detail

#ifdef QDSP_FLOAT_DISCRETE
using QReal = float;
#else
using QReal = double;
#endif

using QComplex = std::complex<QReal>;

QString complexToString(dsp::QComplex value);

inline double dbToFactor(double dB) {
    return pow(10, dB/20);
}

inline double factorToDb(double factor) {
    return 20*log10(factor);
}

inline double dbmToVolts(double dBm, double r = 50)
{
    return sqrt((pow(10, (dBm-30) / 10)*r));
}

inline double voltsToDbm(double volts, double r = 50)
{
    return 10 *log10(volts*volts/r) + 30;
}

class QFrequency {
public:
    explicit QFrequency() = default;
    explicit constexpr QFrequency(double Hz)
        : value_(Hz)
    {}
    constexpr double mHz() const { return  value_/1e-3; }
    constexpr double Hz() const { return  value_; }
    constexpr double kHz() const { return  value_/1e3; }
    constexpr double MHz() const { return  value_/1e6; }
    constexpr double GHz() const { return  value_/1e9; }

    constexpr double value() const { return this->Hz(); }

    constexpr bool isPositive() const { return  value_ > 0; }

    void clear() { value_ = 0; }

private:
    double value_ = 0;
};

inline constexpr QFrequency noFrequency = QFrequency(0);

inline QDebug operator<<(QDebug debug, QFrequency freq)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << freq.Hz() << "Hz";
    return debug;
}

inline constexpr QFrequency operator+(QFrequency f)
{
    return f;
}

inline constexpr QFrequency operator-(QFrequency f)
{
    return QFrequency(-f.value());
}

inline constexpr QFrequency operator+(QFrequency lhs, QFrequency rhs)
{
    return QFrequency(lhs.value() + rhs.value());
}

inline constexpr QFrequency operator-(QFrequency lhs, QFrequency rhs)
{
    return QFrequency(lhs.value() - rhs.value());
}

inline constexpr double operator/(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() / rhs.value();
}

inline constexpr QFrequency operator/(QFrequency lhs, double rhs)
{
    return QFrequency(lhs.value() / rhs);
}

inline constexpr QFrequency operator*(QFrequency lhs, double rhs)
{
    return QFrequency(lhs.value() * rhs);
}

inline constexpr QFrequency operator*(double lhs, QFrequency rhs)
{
    return QFrequency(lhs * rhs.value());
}

inline constexpr bool operator==(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() == rhs.value();
}

inline constexpr bool operator!=(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() != rhs.value();
}

inline constexpr bool operator>(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() > rhs.value();
}

inline constexpr bool operator>=(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() >= rhs.value();
}

inline constexpr bool operator<(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() < rhs.value();
}

inline constexpr bool operator<=(QFrequency lhs, QFrequency rhs)
{
    return lhs.value() <= rhs.value();
}

namespace detail {

template<class Info>
int getResultSize(int lhs, int rhs, const Info& msg)
{
#ifdef QDSP_ENABLE_SIZE_WARNING
    if (lhs != rhs) {
        qDebug() << msg << "Warning: arguments have different size" << lhs << "vs" << rhs;
    }
#else
    Q_UNUSED(msg);
#endif
    return std::min(lhs, rhs);
}

template<class Info>
QFrequency getResultClock(QFrequency lhs, QFrequency rhs, const Info& msg)
{
#ifndef QDSP_ENABLE_CLOCK_WARNING
    Q_UNUSED(msg);
#endif
    if ((lhs.isPositive() && rhs.isPositive()) && lhs == rhs) {
        return lhs;
    } else if ((lhs.isPositive() && rhs.isPositive()) && lhs != rhs) {
#ifdef QDSP_ENABLE_CLOCK_WARNING
        qDebug() << msg << "Warning: arguments have different clock" << scientific << lhs.value() << "vs" << rhs.value() << fixed <<" Result has no clock.";
#endif
        return noFrequency;
    } else if (lhs.isPositive() && !rhs.isPositive()) {
        return lhs;
    } else if (!lhs.isPositive() && rhs.isPositive()) {
        return rhs;
    } else {
        return noFrequency;
    }
}

double generateRandomDouble();

std::pair<double, double> generateGaussianPair();

template<typename Discrete>
class QSignal {
public:
    typedef typename QVector<Discrete>::ConstIterator ConstIterator;
    typedef typename QVector<Discrete>::Iterator Iterator;
    typedef typename QVector<Discrete>::const_iterator const_iterator;
    typedef typename QVector<Discrete>::const_pointer const_pointer;
    typedef typename QVector<Discrete>::const_reference const_reference;
    typedef typename QVector<Discrete>::const_reverse_iterator const_reverse_iterator;
    typedef typename QVector<Discrete>::difference_type difference_type;
    typedef typename QVector<Discrete>::iterator iterator;
    typedef typename QVector<Discrete>::pointer pointer;
    typedef typename QVector<Discrete>::reference reference;
    typedef typename QVector<Discrete>::reverse_iterator reverse_iterator;
    typedef typename QVector<Discrete>::size_type size_type;
    typedef typename QVector<Discrete>::value_type value_type;

public:
    QSignal() = default;
    explicit QSignal(QFrequency clock)
        : clock_(clock)
    {
        checkClock(clock);
    }
    explicit QSignal(int size)
        : data_(size)
    {}
    QSignal(int size, QFrequency clock)
        : clock_(clock), data_(size)
    {
        checkClock(clock);
    }
    explicit QSignal(int size, const Discrete& value)
        : data_(size, value)
    {}
    QSignal(int size, QFrequency clock, const Discrete& value)
        : clock_(clock), data_(size, value)
    {
        checkClock(clock);
    }
    QSignal(const QSignal<Discrete>& other)
        : clock_(other.clock_), data_(other.data_)
    {
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
    }
    QSignal(QSignal<Discrete>&& other)
        : clock_(other.clock_), data_(std::move(other.data_))
    {}
    QSignal(const QVector<Discrete>& data)
        : data_(data)
    {
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
    }
    QSignal(QFrequency clock, const QVector<Discrete>& data)
        : clock_(clock), data_(data)
    {
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
        checkClock(clock);
    }
    QSignal(std::initializer_list<Discrete> args)
        : data_(args)
    {}
    QSignal(QFrequency clock, std::initializer_list<Discrete> args)
        : clock_(clock), data_(args)
    {
        checkClock(clock);
    }
    void append(const Discrete& value) { data_.append(value); }
    void append(const QSignal<Discrete>& other)
    {
        this->setClock(detail::getResultClock(this->clock(), other.clock(), Q_FUNC_INFO));
        data_.append(other.data_);
    }
    const Discrete& at(int i) const {return data_.at(i); }
    reference back() { return data_.back(); }
    const_reference back() const { return data_.back(); }
    iterator begin() { return data_.begin(); }
    const_iterator begin() const { return data_.begin(); }
    int capacity() const { return  data_.capacity(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    const_iterator cend() const { return data_.cend(); }
    void clear() { data_.clear(); }
    QFrequency clock() const { return clock_; }
    const_iterator constBegin() const { return data_.constBegin(); }
    const Discrete*  constData() const { return data_.constData(); }
    const_iterator constEnd() const { return data_.constEnd(); }
    const Discrete& constFirst() const { return data_.constFirst(); }
    const Discrete& constLast() const { return data_.constLast(); }
    bool contains(const Discrete& value) const { return data_.contains(value); }
    int count(const Discrete& value) const { return data_.count(value); }
    int count() const { return data_.count(); }
    const_reverse_iterator crbegin() { return data_.crbegin(); }
    const const_reverse_iterator crend() { return data_.crend(); }
    Discrete* data() { return data_.data(); }
    const Discrete* data() const { return data_.data(); }
    double duration(int n) const { return this->hasClock() ? n/clock_.value() : 0; }
    double duration(int from, int to) const { return this->duration(to - from); }
    double duration() const { return this->duration(this->size()); }
    bool empty() const { return data_.empty(); }
    iterator end() { return data_.end(); }
    const_iterator end() const { return data_.end(); }
    iterator erase(iterator pos) { return data_.erase(pos); }
    iterator erase(iterator begin, iterator end) { return data_.erase(begin, end); }
    Discrete& first() { return data_.first(); }
    const Discrete& first() const { return data_.first(); }
    Discrete& front() { return data_.front(); }
    const_reference front() const { return data_.front(); }
    void insert(int i, const Discrete& value) { data_.insert(i, value); }
    void insert(int i, int count, const Discrete& value)
    { data_.insert(i, count, value); }
    iterator insert(iterator before, int count, const Discrete& value)
    { return data_.insert(before, count, value); }
    iterator insert(iterator before, const Discrete& value)
    { return data_.insert(before, value); }
    bool isEmpty() const { return data_.isEmpty(); }
    bool hasClock() const { return clock_.isPositive(); }
    Discrete& last() {  return data_.last(); }
    const Discrete& last() const {  return data_.last(); }
    int length() const { return data_.length(); }
    void move(int from, int to) { data_.move(from, to); }
    void pop_back() { data_.pop_back(); }
    void pop_front() { data_.pop_front(); }
    void prepend(const Discrete& value) { data_.prepend(value); }
    void push_back(const Discrete& value) { data_.push_back(value); }
    void push_front(const Discrete& value) { data_.push_front(value); }
    reverse_iterator rbegin() { return data_.rbegin(); }
    const_reverse_iterator rbegin() const { return data_.rbegin(); }
    void remove(int i) { data_.remove(i); }
    void remove(int i, int count) { data_.remove(i, count); }
    int  removeAll(const Discrete& t) { return data_.removeAll(t); }
    void removeAt(int i) { data_.removeAt(i); }
    void removeFirst() { data_.removeFirst(); }
    void removeLast() { data_.removeLast(); }
    bool removeOne(const Discrete& t) { return data_.removeOne(t); }
    reverse_iterator rend() { return data_.rend(); }
    const_reverse_iterator rend() const { return data_.rend(); }
    void replace(int i, const Discrete& value) { data_.replace(i, value); }
    void reserve(int size) { data_.reserve(size); }
    void resetClock() { clock_ = noFrequency; }
    void resize(int size)
    {
        if (size != data_.size()) {
            data_.resize(size);
        }
    }
    void setClock(QFrequency freq)
    {
        if (checkClock(freq)) {
            clock_ = freq;
        }
    }
    void shrink_to_fit() { data_.shrink_to_fit(); }
    int size() const { return data_.size(); }
    void squeeze() { data_.squeeze(); }
    void swap(QSignal<Discrete>& other)
    {
        data_.swap(other.data_);
        clock_ = other.clock();
    }
    Discrete takeAt(int i) { return data_.takeAt(i); }
    Discrete takeFirst() { return data_.takeFirst(); }
    Discrete takeLast() { return data_.takeLast(); }
    const QVector<Discrete>& asQVector() const { return data_; }
    QList<Discrete> toList() const { return data_.toList(); }
    std::vector<Discrete> toStdVector() const { return data_.toStdVector(); }
    Discrete value(int i) const { return data_.value(i); }
    Discrete value(int i, const Discrete& defaultValue) const
    { return data_.value(i, defaultValue); }
    Discrete& operator[](int i) { return data_.operator[](i); }
    const Discrete& operator[](int i) const { return data_.operator[](i); }

    auto& operator=(const QSignal<Discrete>& other)
    {
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
        clock_ = other.clock_;
        data_.operator=(other.data_);
        return *this;
    }
    auto& operator=(QSignal<Discrete>&& other)
    {
        clock_ = other.clock_;
        data_.operator=(std::move(other.data_));
        return *this;
    }

    bool operator==(const QSignal<Discrete>& other) const
    { return data_.operator==(other.data_) && clock_ == other.clock(); }

    bool operator!=(const QSignal<Discrete>& other) const
    { return data_.operator!=(other.data_) || clock_ != other.clock(); }

    auto& operator<<(const Discrete& value)
    {
        data_.operator<<(value);
        return *this;
    }

private:
    bool checkClock(QFrequency freq)
    {
        if (freq.value() < 0) {
            throw std::invalid_argument("clock must be positive");
        }
        return true;
    }
    QFrequency clock_;
    QVector<Discrete> data_;
};

} // namespace detail

class QComplexSignal;

class QRealSignal: public detail::QSignal<QReal> {
public:
    QRealSignal() = default;
    explicit QRealSignal(QFrequency clock)
        : QSignal<QReal>(clock)
    {}
    explicit QRealSignal(int size)
        : QSignal<QReal>(size)
    {}
    explicit QRealSignal(int size, QFrequency clock)
            : QSignal<QReal>(size, clock)
        {}
    explicit QRealSignal(int size, const QReal& value)
        : QSignal<QReal>(size, value)
    {}
    explicit QRealSignal(int size, QFrequency clock, const QReal& value)
        : QSignal<QReal>(size, clock, value)
    {}
    QRealSignal(const QRealSignal& other)
        : QSignal<QReal>(other)
    {}
    QRealSignal(QRealSignal&& other)
        : QSignal<QReal>(std::move(other))
    {}
    QRealSignal(std::initializer_list<QReal> args)
        : QSignal<QReal>(args)
    {}
    explicit QRealSignal(QFrequency clock, std::initializer_list<QReal> args)
        : QSignal<QReal>(clock, args)
    {}
    virtual ~QRealSignal() = default;
    QRealSignal& fill(const QReal& value, int size = -1)
    {
        if (size != -1) {
            this->resize(size);
        }
        std::fill(this->begin(), this->end(), value);
        return *this;
    }
    QRealSignal mid(int pos, int length = -1)
    {
        if (length == -1) {
            return *this;
        }
        if (this->isEmpty()) {
            return {};
        }
        QRealSignal result(length, this->clock());
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
        std::copy(this->begin() + pos, this->begin() + pos + length, result.begin());
        return result;
    }
    QRealSignal& operator=(const QRealSignal &other) = default;
    QRealSignal& operator=(QRealSignal &&other) = default;

    template<class OutputIt>
    static void harmonic(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)
    {
        int i = 0;
        std::for_each(start, start + size,
                      [normFreq, magnitude, phase, &i](auto& x) {
            x += magnitude * sin(2*M_PI*normFreq*(i++) + phase);
        });
    }
    static QRealSignal harmonic(int size, double normFreq, double magnitude = 1.0, double phase = 0);
    static QRealSignal harmonic(int size,QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0);

    template<class OutputIt>
    static void meander(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)
    {
        if (normFreq != 0) {
            int i = 0;
            double period = 1 / normFreq;
            std::for_each(start, start + size,
                          [magnitude, period, phase, &i](auto& x) {
                x += (i - period * phase/(2*M_PI) - floor(((double)i - period * phase/(2*M_PI)) / period) * period  < period / 2 ?
                          magnitude : -magnitude);
                ++i;
            });
        } else {
            std::fill(start, start + size, phase < M_PI ? magnitude : -magnitude);
        }
    }
    static QRealSignal meander(int size, double normFreq, double magnitude = 1.0, double phase = 0);
    static QRealSignal meander(int size,QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0);

    template<class OutputIt>
    static void saw(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)
    {
        if (normFreq != 0) {
            int i = 0;
            double period = 1 / normFreq;
            double step = 2 * magnitude / period;
            std::for_each(start, start + size,
                          [magnitude, period, phase, &i, step](auto& x) {
                x += step * (i - period * phase/(2*M_PI) - floor(((double)i - period * phase/(2*M_PI)) / period) * period) - magnitude;
                ++i;
            });
        } else {
            std::fill(start, start + size, 2*magnitude * (phase / (2*M_PI) - 0.5));
        }
    }
    static QRealSignal saw(int size, double normFreq, double magnitude = 1.0, double phase = 0);
    static QRealSignal saw(int size,QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0);

    template<class OutputIt>
    static void chirp(OutputIt start, int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0)
    {
        int i = 0;
        double currentFreq = startNormFreq;
        double freqStep = (double)(stopNormFreq - startNormFreq) / (2 * (size - 1));
        std::for_each(start, start + size,
                      [&currentFreq, freqStep, magnitude, phase, &i](auto& x) {
            x += magnitude * sin(2*M_PI*currentFreq*(i++) + phase);
            currentFreq += freqStep;
        });
    }
    static QRealSignal chirp(int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0);
    static QRealSignal chirp(int size,QFrequency clock, QFrequency startFreq, QFrequency stopFreq, double magnitude = 1.0, double phase = 0);

    template<class OutputIt>
    static void uniformNoise(OutputIt start, int size, double dispersion = 1.0, double mean = 0)
    {
        std::for_each(start, start + size,
                      [dispersion, mean](auto &x) {
            x += detail::generateRandomDouble() * dispersion + mean;
        });
    }
    static QRealSignal uniformNoise(int size,QFrequency clock, double v = 1.0, double mean = 0);
    static QRealSignal uniformNoise(int size, double v = 1.0, double mean = 0);

    template<class OutputIt>
    static void gaussianNoise(OutputIt start, int size, double dispersion = 1.0, double mean = 0)
    {
        for (int i = 0; i < size/2; ++i) {
            auto pair = detail::generateGaussianPair();
            *(start + 2*i) = pair.first * dispersion  + mean;
            *(start + 2*i+1) = pair.second * dispersion + mean;
        }
        if (size % 2 != 0) {
            auto pair = detail::generateGaussianPair();
            *(start + size-1) = pair.first * dispersion + mean;
        }
    }
    static QRealSignal gaussianNoise(int size,QFrequency clock, double dispersion = 1.0, double mean = 0);
    static QRealSignal gaussianNoise(int size, double dispersion = 1.0, double mean = 0);
};

class QComplexSignal: public detail::QSignal<QComplex> {
public:
    QComplexSignal() = default;
    explicit QComplexSignal(QFrequency clock)
        : QSignal<QComplex>(clock)
    {}
    explicit QComplexSignal(int size)
        : QSignal<QComplex>(size)
    {}
    explicit QComplexSignal(int size, QFrequency clock)
           : QSignal<QComplex>(size, clock)
       {}
    explicit QComplexSignal(int size, const QComplex& value)
        :QSignal<QComplex>(size, value)
    {}
    explicit QComplexSignal(int size, QFrequency clock, const QComplex& value)
        :QSignal<QComplex>(size, clock, value)
    {}
    QComplexSignal(const QComplexSignal& other)
        :QSignal<QComplex>(other)
    {}
    QComplexSignal(const QRealSignal& other)
        :QSignal<QComplex>(other.size(), other.clock())
    {
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
        std::copy(other.begin(), other.end(), this->begin());
    }
    QComplexSignal(QComplexSignal&& other)
        :QSignal<QComplex>(std::move(other))
    {}
    QComplexSignal(std::initializer_list<QComplex> args)
        :QSignal<QComplex>(args)
    {}
    explicit QComplexSignal(QFrequency clock, std::initializer_list<QComplex> args)
        :QSignal<QComplex>(clock, args)
    {}
    virtual ~QComplexSignal() = default;
    using QSignal<QComplex>::append;
    void append(const QRealSignal& other)
    {
        this->setClock(detail::getResultClock(this->clock(), other.clock(), Q_FUNC_INFO));
        int oldSize = this->size();
        this->resize(oldSize + other.size());
        std::copy(other.begin(), other.end(), this->begin() + oldSize);
    }
    auto& fill(const QComplex& value, int size = -1)
    {
        if (size != -1) {
            this->resize(size);
        }
        std::fill(this->begin(), this->end(), value);
        return *this;
    }
    QComplexSignal mid(int pos, int length = -1)
    {
        if (length == -1) {
            return *this;
        }
        if (this->isEmpty()) {
            return {};
        }
        QComplexSignal result(length, this->clock());
#ifdef QDSP_ENABLE_DEEP_COPY_WARNING
        qDebug() << Q_FUNC_INFO << detail::msgDeepCopyWarning;
#endif
        std::copy(this->begin() + pos, this->begin() + pos + length, result.begin());
        return result;
    }
    QRealSignal i() const;
    QRealSignal q() const;
    QComplexSignal& operator=(const QComplexSignal &other) = default;
    QComplexSignal& operator=(QComplexSignal &&other) = default;

    template<class OutputIt>
    static void harmonic(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)
    {
        int i = 0;
        std::for_each(start, start + size,
                      [ normFreq, magnitude, phase, &i](auto& x) {
            x += magnitude * QComplex(cos(2*M_PI*normFreq * i + phase),
                                         sin(2*M_PI*normFreq * i + phase));
            ++i;
        });
    }
    static QComplexSignal harmonic(int size, double normFreq, double magnitude = 1.0, double phase = 0);
    static QComplexSignal harmonic(int size,QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0);

    template<class OutputIt>
    static void chirp(OutputIt start, int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0)
    {
        int i = 0;
        double currentFreq = startNormFreq;
        double freqStep = (double)(stopNormFreq - startNormFreq) / (2 * (size - 1));
        std::for_each(start, start + size,
                      [&currentFreq, freqStep, magnitude, phase, &i](auto& x) {
            x += magnitude * QComplex(cos(2*M_PI*currentFreq * i + phase),
                                     sin(2*M_PI*currentFreq * i + phase));
            ++i;
            currentFreq += freqStep;
        });
    }
    static QComplexSignal chirp(int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0);
    static QComplexSignal chirp(int size,QFrequency clock, QFrequency startFreq, QFrequency stopFreq, double magnitude = 1.0, double phase = 0);

    template<class OutputIt>
    static void uniformNoise(OutputIt start, int size, double dispersion = 1.0, double mean = 0)
    {
        std::for_each(start, start + size,
                      [dispersion, mean](auto &x) {
            x += QComplex(detail::generateRandomDouble() * dispersion + mean,
                          detail::generateRandomDouble() * dispersion + mean);
        });
    }
    static QComplexSignal uniformNoise(int size,QFrequency clock, double dispersion = 1.0, double mean = 0);
    static QComplexSignal uniformNoise(int size, double dispersion = 1.0, double mean = 0);

    template<class OutputIt>
    static void gaussianNoise(OutputIt start, int size, double dispersion = 1.0, double mean = 0)
    {
        std::for_each(start, start + size,
                      [dispersion, mean](QComplex &x) {
            auto pair = detail::generateGaussianPair();
            x += QComplex(pair.first * dispersion + mean,
                          pair.second * dispersion + mean);
        });
    }
    static QComplexSignal gaussianNoise(int size,QFrequency clock, double dispersion = 1.0, double mean = 0);
    static QComplexSignal gaussianNoise(int size, double dispersion = 1.0, double mean = 0);
};


template <typename T>
inline constexpr bool isDiscrete = std::is_same<T, QComplex>::value || std::is_arithmetic<T>::value;

template <typename T>
inline constexpr bool isRealDiscrete = std::is_arithmetic<T>::value;

template <typename T>
inline constexpr bool isComplexDiscrete = std::is_same<T, QComplex>::value;

template <typename T>
inline constexpr bool isSignal = std::is_same<QRealSignal, T>::value || std::is_same<QComplexSignal, T>::value;

template <typename T>
inline constexpr bool isRealSignal = std::is_same<QRealSignal, T>::value;

template <typename T>
inline constexpr bool isComplexSignal = std::is_same<QComplexSignal, T>::value;

template <typename T>
inline constexpr bool isSignalBased = std::is_base_of<QRealSignal, T>::value || std::is_base_of<QComplexSignal, T>::value;

template <typename T>
inline constexpr bool isRealSignalBased = std::is_base_of<QRealSignal, T>::value;

template <typename T>
inline constexpr bool isComplexSignalBased = std::is_base_of<QComplexSignal, T>::value;


namespace detail {

template<class T, class = std::enable_if_t<isSignalBased<T>>>
class QSignalFrame {
public:

    typedef typename T::ConstIterator ConstIterator;
    typedef typename T::const_iterator const_iterator;
    typedef typename T::const_pointer const_pointer;
    typedef typename T::const_reference const_reference;
    typedef typename T::const_reverse_iterator const_reverse_iterator;
    typedef typename T::difference_type difference_type;
    typedef typename T::size_type size_type;
    typedef typename T::value_type value_type;

    using Discrete = value_type;

    QSignalFrame() = delete;
    QSignalFrame(const T& signal)
        : signal_(&signal), frameWidth_(signal.size())
    {}
    QSignalFrame(const QSignalFrame& other)
        : signal_(other.signal_), frameStart_(other.frameStart_), frameWidth_(other.frameWidth_)
    {}
    QSignalFrame(const T& signal, int width, int startPos = 0)
        : signal_(&signal), frameStart_(startPos), frameWidth_(width)
    {}
    QSignalFrame(const QSignalFrame& other, int width, int startPos = 0)
        : signal_(other.signal_), frameStart_(other.frameStart_ + startPos), frameWidth_(width)
    {}
    const Discrete& at(int i) const {return signal_->at(frameStart_ + i); }
    const_reference back() const { return *this->crbegin(); }
    const_iterator begin() const { return signal_->begin() + frameStart_; }
    const_iterator cbegin() const { return signal_->cbegin() + frameStart_; }
    const_iterator cend() const { return this->cbegin() + frameWidth_; }
    QFrequency clock() const { return signal_->clock(); }
    const_iterator constBegin() const { return signal_->constBegin() + frameStart_; }
    const Discrete*  constData() const { return signal_->constData() + frameStart_; }
    const_iterator constEnd() const { return signal_->constBegin() + frameWidth_; }
    const Discrete& constFirst() const { return *this->cbegin(); }
    const Discrete& constLast() const { return *this->crbegin(); }
    int count() const { return this->width(); }
    const_reverse_iterator crbegin() const { return this->rbegin(); }
    const_reverse_iterator crend() const { return this->rend(); }
    const Discrete* data() const { return signal_->data() + frameStart_; }
    const QSignalFrame& decreaseWidth(int decrement = 1) { return this->resize(this->size() - decrement); }
    double duration(int n) const { return signal_->duration(n); }
    double duration(int from, int to) const { return this->duration(to - from); }
    double duration() const { return this->duration(this->size()); }
    bool empty() const { return frameWidth_ == 0; }
    const_iterator end() const { return this->begin() + frameWidth_; }
    const Discrete& first() const { return *this->cbegin(); }
    const_reference front() const { return this->first(); }
    const QSignalFrame& increaseWidth(int increment = 1) { return this->resize(this->size() + increment); }
    bool isEmpty() const { return this->empty(); }
    bool isSignalEmpty() const { return signal_->empty(); }
    bool hasClock() const { return signal_->clock().isPositive(); }
    const Discrete& last() const { return *this->crbegin(); }
    int length() const { return this->width(); }
    int position() const { return frameStart_; }
    const_reverse_iterator rbegin() const { return this->rend() - frameWidth_; }
    const_reverse_iterator rend() const { return signal_->rend() - frameStart_; }
    const QSignalFrame& resetPosition()
    {
        frameStart_ = 0;
        return *this;
    }
    const QSignalFrame& resetSize()
    {
        frameWidth_ = signal_->size();
        return *this;
    }
   const QSignalFrame& resize(int size)
    {
       frameWidth_ = size;
       return *this;
    }
    const QSignalFrame& setPosition(int pos)
    {
        frameStart_ = pos;
        return *this;
    }
    const QSignalFrame& setWidth(int width) { return this->resize(width); }
    int signalSize() const { return signal_->size(); }
    int size() const { return this->width(); }
    const QSignalFrame& shift(int num = 1)
    {
        frameStart_ += num;
        return *this;
    }
    void swap(QSignalFrame& other)
    {
       std::swap(signal_, other.signal_);
       std::swap(frameStart_, other.frameStart_);
       std::swap(frameWidth_, other.frameWidth_);
    }
    T toSignal() const
    {
        T result(this->size(), this->clock());
        std::copy(this->begin(), this->end(), result.begin());
        return result;
    }
    Discrete value(int i) const { return signal_->value(frameStart_ + i); }
    Discrete value(int i, const Discrete& defaultValue) const
    { return signal_->value(frameStart_ + i, defaultValue); }
    int width() const { return frameWidth_; }
    const Discrete& operator[](int i) const { return signal_->operator[](frameStart_ + i); }

private:
    const T* signal_;
    int frameStart_ = 0;
    int frameWidth_ = 0;
};

} // namespace detail

using QRealSignalFrame = detail::QSignalFrame<QRealSignal>;
using QComplexSignalFrame = detail::QSignalFrame<QComplexSignal>;

template <typename T>
inline constexpr bool isFrame = std::is_same<QRealSignalFrame, T>::value || std::is_same<QComplexSignalFrame, T>::value;

template <typename T>
inline constexpr bool isRealSignalFrame = std::is_same<QRealSignalFrame, T>::value;

template <typename T>
inline constexpr bool isComplexSignalFrame = std::is_same<QComplexSignalFrame, T>::value;

template <typename T>
inline constexpr bool isSignalContainer = isSignalBased<T> || isFrame<T>;

template <typename T>
inline constexpr bool isRealSignalContainer = isRealSignalBased<T> || isRealSignalFrame<T>;

template <typename T>
inline constexpr bool isComplexSignalContainer = isComplexSignalBased<T> || isComplexSignalFrame<T>;

template <typename T1, typename T2>
inline constexpr bool hasSignalResult = (isSignalContainer<T1> && isSignalContainer<T2>) ||
        (isSignalContainer<T1> && isDiscrete<T2>) ||
        (isDiscrete<T1> && isSignalContainer<T2>);

template <typename T1, typename T2>
inline constexpr bool hasComplexResult = ((isComplexSignalContainer<T1> || isComplexDiscrete<T1>) && (isSignalContainer<T2> || isDiscrete<T2>)) ||
        ((isSignalContainer<T1> || isDiscrete<T1>) && (isComplexSignalContainer<T2> || isComplexDiscrete<T2>));

template <typename T1, typename T2>
inline constexpr bool hasRealResult = (isRealDiscrete<T1> && isRealDiscrete<T2>) ||
        (isRealSignalContainer<T1> && isRealDiscrete<T2>) ||
        (isRealDiscrete<T1> && isRealSignalContainer<T2>) ||
        (isRealSignalContainer<T1> && isRealSignalContainer<T2>);

namespace detail {

template <class T1, class T2, class = std::enable_if_t<hasSignalResult<T1, T2> && hasComplexResult<T1, T2>>>
QComplexSignal signalResult(){ return {}; };

template <class T1, class T2, class = std::enable_if_t<hasSignalResult<T1, T2> && hasRealResult<T1, T2>>>
QRealSignal signalResult(){ return {}; };

template <class T1, class T2, class = std::enable_if_t<hasSignalResult<T1, T2>>>
using SignalResult = decltype (detail::signalResult<T1, T2>());

} // namespace detail

template<class T, class = std::enable_if_t<isSignalContainer<T>>>
QDebug operator<<(QDebug debug, const T& s)
{
    QDebugStateSaver saver(debug);
    if constexpr (std::is_same<T, QRealSignal>::value) {
        debug.nospace() << "QRealSignal(";
    } else if constexpr (std::is_same<T, QComplexSignal>::value) {
        debug.nospace() << "QComplexSignal(";
    } else if constexpr (std::is_same<T, QRealSignalFrame>::value) {
        debug.nospace() << "QRealSignalFrame(";
    } else {
        debug.nospace() << "QComplexSignalFrame(";
    }
    debug.nospace() << s.clock() << ")(";
    for (auto it = s.begin(); it != s.end(); ++it) {
        debug.nospace() << (it == s.begin() ? "" : ", ") << *it;
    }
    debug << ")";
    return debug;
}

template <class T, class = std::enable_if_t<isSignalContainer<T>>>
auto operator+(const T& lhs)
{
    detail::SignalResult<T, T> result(lhs.clock(), lhs.size());
    std::copy(lhs.begin(), lhs.end(), result.begin());
    return result;
}

template <class T, class = std::enable_if_t<isSignalContainer<T>>>
auto operator-(const T& lhs)
{
    detail::SignalResult<T, T> result(lhs.clock(), lhs.size());
    std::transform(lhs.begin(), lhs.end(), result.begin(),
                   [](auto a){ return -a; });
    return result;
}

template <class T1, class T2, class = std::enable_if_t<isSignalContainer<T1> && isSignalContainer<T2>>>
auto operator+(const T1& lhs, const T2& rhs)
{
    detail::SignalResult<T1, T2> result(detail::getResultSize(lhs.size(), rhs.size(), Q_FUNC_INFO),
                                        detail::getResultClock(lhs.clock(), rhs.clock(), Q_FUNC_INFO));
    std::transform(lhs.begin(), lhs.begin() + result.size(), rhs.begin(), result.begin(),
                   [](auto a, auto b){ return a + b; });
    return result;
}

template <class T1, class T2, class = std::enable_if_t<isSignalContainer<T1> && isSignalContainer<T2>>>
auto operator-(const T1& lhs, const T2& rhs)
{
    detail::SignalResult<T1, T2> result(detail::getResultSize(lhs.size(), rhs.size(), Q_FUNC_INFO),
                                        detail::getResultClock(lhs.clock(), rhs.clock(), Q_FUNC_INFO));
    std::transform(lhs.begin(), lhs.begin() + result.size(), rhs.begin(), result.begin(),
                   [](auto a, auto b){ return a - b; });
    return result;
}

template <class T1, class T2, class = std::enable_if_t<hasSignalResult<T1, T2>>>
auto operator*(const T1& lhs, const T2& rhs)
{
    if constexpr (isSignalContainer<T1> && isSignalContainer<T2>) {
        detail::SignalResult<T1, T2> result(detail::getResultSize(lhs.size(), rhs.size(), Q_FUNC_INFO),
                                            detail::getResultClock(lhs.clock(), rhs.clock(), Q_FUNC_INFO));
        std::transform(lhs.begin(), lhs.begin() + result.size(), rhs.begin(), result.begin(),
                       [](auto a, auto b){ return a * b; });
        return result;
    } else {
        if constexpr (isDiscrete<T1>) {
            detail::SignalResult<T1, T2> result(rhs);
            std::transform(result.begin(), result.end(), result.begin(),
                           [lhs](auto value) { return value *= lhs; });
            return result;
        } else {
            detail::SignalResult<T1, T2> result(lhs);
            std::transform(result.begin(), result.end(), result.begin(),
                           [rhs](auto value) { return value *= rhs; });
            return result;
        }
    }
}

template <class T1, class T2, class = std::enable_if_t<isSignalContainer<T1>&&isDiscrete<T2>>>
auto operator/(const T1& lhs, const T2& rhs)
{
    detail::SignalResult<T1, T2> result(lhs);
    std::transform(result.begin(), result.end(), result.begin(),
                   [rhs](auto value) { return value /= rhs; });
    return result;
}

template <class T1, class T2, class = std::enable_if_t< (isComplexSignalBased<T1> && isSignalContainer<T2>) ||
                                                        (isRealSignalBased<T1> && isRealSignalContainer<T2>) >>
T1& operator+=(T1& lhs, const T2& rhs)
{
    lhs.setClock(detail::getResultClock(lhs.clock(), rhs.clock(), Q_FUNC_INFO));
    lhs.resize(detail::getResultSize(lhs.size(), rhs.size(), Q_FUNC_INFO));
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(),
                   [](auto lhs_, auto rhs_) { return lhs_ += rhs_; });
    return lhs;
}

template <class T1, class T2, class = std::enable_if_t< (isComplexSignalBased<T1> && isSignalContainer<T2>) ||
                                                        (isRealSignalBased<T1> && isRealSignalContainer<T2>) >>
T1& operator-=(T1& lhs, const T2& rhs)
{
    lhs.setClock(detail::getResultClock(lhs.clock(), rhs.clock(), Q_FUNC_INFO));
    lhs.resize(detail::getResultSize(lhs.size(), rhs.size(), Q_FUNC_INFO));
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(),
                   [](auto lhs_, auto rhs_) { return lhs_ -= rhs_; });
    return lhs;
}

template <class T1, class T2, class = std::enable_if_t< (isComplexSignalBased<T1> && isSignalContainer<T2>) ||
                                                        (isComplexSignalBased<T1> && isDiscrete<T2>) ||
                                                        (isRealSignalBased<T1> && isRealSignalContainer<T2>) ||
                                                        (isRealSignalBased<T1> && isRealDiscrete<T2>) >>
T1& operator*=(T1& lhs, const T2& rhs)
{
    if constexpr (isSignalContainer<T1> && isSignalContainer<T2>) {
        lhs.setClock(detail::getResultClock(lhs.clock(), rhs.clock(), Q_FUNC_INFO));
        lhs.resize(detail::getResultSize(lhs.size(), rhs.size(), Q_FUNC_INFO));
        std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(),
                       [](auto lhs_, auto rhs_) { return lhs_ *= rhs_; });
    } else {
        std::for_each(lhs.begin(), lhs.end(),
                       [rhs](auto& discrete) { discrete *= rhs; });
    }
    return lhs;
}

template <class T1, class T2, class = std::enable_if_t< (isComplexSignalBased<T1> && isDiscrete<T2>) ||
                                                        (isRealSignalBased<T1> && isRealDiscrete<T2>) >>
T1& operator/=(T1& lhs, const T2& rhs)
{
    std::for_each(lhs.begin(), lhs.end(),
                   [rhs](auto& discrete) { discrete /= rhs; });
    return lhs;
}

class QWaveformOptions
{
public:
    QWaveformOptions()
        : dateTime_(QDateTime::currentDateTime())
    {}
    explicit QWaveformOptions(const QString& comment)
        : comment_(comment), dateTime_(QDateTime::currentDateTime())
    {}
    QWaveformOptions(const QString& comment, QDateTime dateTime)
        : comment_(comment), dateTime_(dateTime)
    {}
    explicit QWaveformOptions(QString&& comment)
        : comment_(std::move(comment)), dateTime_(QDateTime::currentDateTime())
    {}
    QWaveformOptions(QString&& comment, QDateTime dateTime)
        : comment_(std::move(comment)), dateTime_(dateTime)
    {}
    virtual ~QWaveformOptions() = default;

    const QString& comment() const { return comment_; }
    void setComment(const QString& comment) { comment_ = comment; }

    QDateTime dateTime() const { return dateTime_; }
    void setDateTime(QDateTime dateTime) { dateTime_ = dateTime;}
private:
    QString comment_;
    QDateTime dateTime_;
};

enum class WaveformDataType {INT8, INT16, INT32, INT64, FLOAT, DOUBLE};

inline QDebug& operator<<(QDebug& debug, WaveformDataType dataType);

class QWaveformFileInfo: public QWaveformOptions
{
public:
    QWaveformFileInfo() = default;
    QWaveformFileInfo(double clock, WaveformDataType dataType, int64_t signalSize,
                      bool complex, int16_t refLevel, const QString& comment, QDateTime dateTime)
        : QWaveformOptions(comment, dateTime), signalClock_(clock), signalSize_(signalSize),
          dataType_(dataType), refLevel_(refLevel), isComplex_(complex)
    {}
    QWaveformFileInfo(double clock, WaveformDataType dataType, int64_t signalSize,
                      bool complex, int16_t refLevel = 0, const QString& comment = "")
        : QWaveformOptions(comment), signalClock_(clock), signalSize_(signalSize),
          dataType_(dataType), refLevel_(refLevel), isComplex_(complex)
    {}

    QFrequency clock() const { return signalClock_; }
    void setClock(QFrequency clock) { signalClock_ = clock; }

    WaveformDataType dataType() const { return dataType_; }
    void setDataType(WaveformDataType dataType) { dataType_ = dataType; }
    bool isInteger() const { return !(dataType_ == WaveformDataType::FLOAT || dataType_ == WaveformDataType::DOUBLE); }

    bool isComplex() const { return isComplex_; }
    void setComplex(bool isComplex) { isComplex_ = isComplex; }

    int signalSize() const { return signalSize_; }
    void setSignalSize(int64_t size) { signalSize_ = size; }

    int16_t refLevel() const { return refLevel_; }
    void setRefLevel(int16_t value) { refLevel_ = value; }

    static QWaveformFileInfo analyse(const QString& fileName);

private:
    QFrequency signalClock_;
    int64_t signalSize_ = 0;
    WaveformDataType dataType_ = WaveformDataType::DOUBLE;
    int16_t refLevel_ = 0;
    bool isComplex_ = 1;
};

namespace detail {

namespace waveformHeaderConstants
{
    static constexpr int headerSize_ = 32;
    static constexpr int descriptionBlockSize_ = 32;

    static constexpr int isComplexBitOffset_ = 3;
    static constexpr int isComplexBitMask_ = 0x01;
    static constexpr int isIntegerBitOffset_ = 2;
    static constexpr int isIntegerBitMask_ = 0x01;
    static constexpr int dataTypeBitMask_ = 0x03;

    static constexpr int versionOffset_ = 0;
    static constexpr int typeOffset_ = 1;
    static constexpr int descriptionSizeOffset_ = 2;
    static constexpr int referenceOffset_ = 4;
    static constexpr int sizeOffset_ = 8;
    static constexpr int clockOffset_ = 16;
    static constexpr int dateOffset_ = 24;
    static constexpr int descriptionOffset_ = headerSize_;
};

uint8_t encodeSize(WaveformDataType dataType);

WaveformDataType decodeSize(uint8_t dataType, bool isInteger);

// Затычка для функций прогресса загрузки и записи
inline void dummyProgressFunction(int)
{}

void writeWaveformHeader(QIODevice& file, const QWaveformFileInfo& info);
QWaveformFileInfo readWaveformHeader(QIODevice& file);

template<class Signal, class T, class=std::enable_if_t<isSignalContainer<Signal> &&
                                          (std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value ||
                                          std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value ||
                                          std::is_same<T, float>::value || std::is_same<T, double>::value)>>
void writeBinaryData(QIODevice& output, const Signal& signal, double scaleFactor, std::function<void(int)> progressFunction)
{
    int sampleSize = (isComplexSignalContainer<Signal>?2:1)*sizeof (T);
    int64_t step = signal.size() / 100;
    int64_t samplesLeft = signal.size();
    int64_t n = 0;
    int bufferSize = 1024*1024; // 1 Msample
    T* buffer = new T[(isComplexSignalContainer<Signal>?2:1)*bufferSize];
    while (samplesLeft > 0) {
        int samplesToWrite = samplesLeft > bufferSize ? bufferSize : samplesLeft;
        for (int i = 0; i < samplesToWrite; ++i) {
            if constexpr (isComplexSignalContainer<Signal>) {
                if constexpr (std::numeric_limits<T>::is_integer) {
                    buffer[2*i] = (T)round(signal.at(i).real() * scaleFactor);
                    buffer[2*i+1] = (T)round(signal.at(i).imag() * scaleFactor);
                } else {
                    buffer[2*i] = (T)(signal.at(i).real() * scaleFactor);
                    buffer[2*i+1] = (T)(signal.at(i).imag() * scaleFactor);
                }
            } else {
                buffer[i] = (T)(signal.at(i) * scaleFactor);
            }
            if (step != 0 && n % step == 0) {
                progressFunction((n*100) / signal.size());
            }
            ++n;
        }
        if (output.write(reinterpret_cast<char*>(buffer), samplesToWrite * sampleSize) != samplesToWrite * sampleSize ) {
            throw std::runtime_error(msgWriteError.toStdString());
        }
        samplesLeft -= samplesToWrite;
    }
    delete [] buffer;
    progressFunction(100);
}

template<class Signal, class=std::enable_if_t<isSignal<Signal>>>
void writeBinaryData(QIODevice& output, const Signal& signal, WaveformDataType dataType, double scaleFactor, std::function<void(int)> progressFunction)
{
    switch (dataType) {
    case WaveformDataType::INT8:
        writeBinaryData<Signal, int8_t>(output, signal, scaleFactor, progressFunction);
        break;
    case WaveformDataType::INT16:
        writeBinaryData<Signal, int16_t>(output, signal, scaleFactor, progressFunction);
        break;
    case WaveformDataType::INT32:
        writeBinaryData<Signal, int32_t>(output, signal, scaleFactor, progressFunction);
        break;
    case WaveformDataType::INT64:
        writeBinaryData<Signal, int64_t>(output, signal, scaleFactor, progressFunction);
        break;
    case WaveformDataType::FLOAT:
        writeBinaryData<Signal, float>(output, signal, 1, progressFunction);
        break;
    default: writeBinaryData<Signal, double>(output, signal, 1, progressFunction);
    }
}

template<class Signal, class T, class=std::enable_if_t< isSignal<Signal> &&
                                          (std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value ||
                                          std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value ||
                                          std::is_same<T, float>::value || std::is_same<T, double>::value)>>
Signal readBinaryData(QIODevice& input, int64_t samples, double factor, std::function<void(int)> progressFunction)
{
    int sampleSize = (isComplexSignal<Signal>?2:1)*sizeof (T);
    int64_t step = samples / (100);
    int64_t samplesLeft = samples;
    Signal result(samples);
    int64_t n = 0;
    int bufferSize = 1024*1024; // 1 Msample
    T* buffer = new T[(isComplexSignal<Signal>?2:1)*bufferSize];
    while (samplesLeft > 0) {
        int64_t bytesRead = input.read(reinterpret_cast<char*>(buffer),
                                      samplesLeft > bufferSize ? bufferSize * sampleSize : samplesLeft * sampleSize);
        if (bytesRead <=0) {
            throw std::runtime_error(msgReadError.toStdString());
        }
        for (int i = 0; i < bytesRead / sampleSize; ++i) {
            if constexpr (isComplexSignal<Signal>) {
                result[n] = QComplex(buffer[2*i], buffer[2*i+1])*factor;
            } else {
                result[n] = (double)buffer[i] * factor;
            }
            if (step != 0 && n % step == 0) {
                progressFunction((n*100) / samples);
            }
            ++n;
        }
        samplesLeft -= bytesRead / sampleSize;
    }
    delete [] buffer;
    progressFunction(100);
    return result;
}

template<class Signal, class=std::enable_if_t<isSignal<Signal>>>
Signal readBinaryData(QIODevice& input, int64_t samples, WaveformDataType dataType, double factor, std::function<void(int)> progressFunction)
{
    switch (dataType) {
    case WaveformDataType::INT8:
        return readBinaryData<Signal, int8_t>(input, samples, factor, progressFunction);
    case WaveformDataType::INT16:
        return readBinaryData<Signal, int16_t>(input, samples, factor, progressFunction);
    case WaveformDataType::INT32:
        return readBinaryData<Signal, int32_t>(input, samples, factor, progressFunction);
    case WaveformDataType::INT64:
        return readBinaryData<Signal, int64_t>(input, samples, factor, progressFunction);
    case WaveformDataType::FLOAT:
        return readBinaryData<Signal, float>(input, samples, 1, progressFunction);
    default:
        return readBinaryData<Signal, double>(input, samples, 1, progressFunction);
    }
}

template<class Signal, class=std::enable_if_t<isSignalContainer<Signal>>>
int getRefLevel(const Signal& signal, WaveformDataType dataType)
{
    switch (dataType) {
    case WaveformDataType::INT8:
        return std::floor(factorToDb((double)std::numeric_limits<int8_t>::max()/peak(signal)));
    case WaveformDataType::INT16:
        return std::floor(factorToDb((double)std::numeric_limits<int16_t>::max()/peak(signal)));
    case WaveformDataType::INT32:
        return std::floor(factorToDb((double)std::numeric_limits<int32_t>::max()/peak(signal)));
    case WaveformDataType::INT64:
        return std::floor(factorToDb((double)std::numeric_limits<int64_t>::max()/peak(signal)));
    default: return 1;
    }
}

template<class T, class = std::enable_if_t<isSignal<T>>>
class QWaveform: public T, public QWaveformOptions
{
public:
    QWaveform()
        : T(), QWaveformOptions()
    {}
    QWaveform(const T& signal)
        : T(signal), QWaveformOptions()
    {}
    QWaveform(T&& signal)
        : T(std::move(signal)), QWaveformOptions()
    {}
    QWaveform(const T& signal, const QString& comment)
        : T(signal), QWaveformOptions(comment)
    {}
    QWaveform(const T& signal, const QString& comment, QDateTime dateTime)
        : T(signal), QWaveformOptions(comment, dateTime)
    {}
    QWaveform(T&& signal, const QString& comment)
        : T(std::move(signal)), QWaveformOptions(comment)
    {}
    QWaveform(T&& signal, const QString& comment, QDateTime dateTime)
        : T(std::move(signal)), QWaveformOptions(comment, dateTime)
    {}
    QWaveform(T&& signal, QString&& comment)
        : T(std::move(signal)), QWaveformOptions(std::move(comment))
    {}
    QWaveform(T&& signal, QString&& comment, QDateTime dateTime)
        : T(std::move(signal)), QWaveformOptions(std::move(comment), dateTime)
    {}
    void saveToFile(const QString& fileName, std::function<void(int)> progressFunction, WaveformDataType dataType = WaveformDataType::DOUBLE) const
    {
        if (fileName == "") {
            throw std::invalid_argument(detail::msgInvalidFileName.toStdString());
        }
        QFile targetFile(fileName);
        targetFile.open(QIODevice::WriteOnly);

        if (!targetFile.isWritable()) {
            throw;
        }
        QWaveformFileInfo info;
        int refLevel = getRefLevel(static_cast<const QComplexSignal&>(*this),dataType);
        info.setRefLevel(refLevel);
        info.setSignalSize(this->size());
        info.setClock(this->clock());
        info.setComplex(std::is_same<T, QComplexSignal>::value);
        info.setDataType(dataType);
        info.setComment(this->comment());
        info.setDateTime(this->dateTime());
        detail::writeWaveformHeader(targetFile, info);
        writeBinaryData<T>(targetFile, *this, dataType, dbToFactor(refLevel), progressFunction);
        targetFile.close();
    }
    void saveToFile(const QString& fileName, WaveformDataType dataType = WaveformDataType::DOUBLE) const
    { this->saveToFile(fileName, detail::dummyProgressFunction, dataType); }

    void saveAsCsv(const QString& fileName, std::function<void(int)> progressFunction, char delimiter = ';') const
    {
        if (fileName == "") {
            throw std::invalid_argument(detail::msgInvalidFileName.toStdString());
        }
        QFile targetFile(fileName);
        targetFile.open(QIODevice::WriteOnly);

        if (!targetFile.isWritable()) {
            throw;
        }
        int step = this->size() / 100;
        int n = 0;
        int duration = this->duration();
        for (int i = 0; i < this->size(); ++i) {
            auto discrete = this->at(i);

            QString line = QString::number((double)i / this->size() * duration) + delimiter;
            if constexpr (std::is_same<typeof (discrete), QComplex>::value) {
                line += QString::number(this->at(i).real()) + delimiter +
                        QString::number(this->at(i).imag());
            } else {
                line += QString::number(this->at(i));
            }
            line += "\r\n";
            if (step != 0 && n % step == 0) {
                progressFunction((n*100) / this->size());
            }
            ++n;
            if (targetFile.write(line.toLocal8Bit()) == -1) {
                throw std::runtime_error(msgWriteError.toStdString());
            }
        }
        progressFunction(100);
        targetFile.close();
    }
    void saveAsCsv(const QString& fileName, char delimiter = ';') const
    { this->saveAsCsv(fileName, detail::dummyProgressFunction, delimiter); }

    void saveAsPlainText(const QString& fileName, std::function<void(int)> progressFunction) const
    {
        if (fileName == "") {
            throw std::invalid_argument(detail::msgInvalidFileName.toStdString());
        }
        QFile targetFile(fileName);
        targetFile.open(QIODevice::WriteOnly);

        if (!targetFile.isWritable()) {
            throw;
        }
        int step = this->size() / 100;
        int n = 0;
        for (int i = 0; i < this->size(); ++i) {
            auto discrete = this->at(i);
            QString line;
            if constexpr (std::is_same<typeof (discrete), QComplex>::value) {
                line = complexToString(this->at(i));
            } else {
                line = QString::number(this->at(i));
            }
            line += "\r\n";
            if (step != 0 && n % step == 0) {
                progressFunction((n*100) / this->size());
            }
            ++n;
            if (targetFile.write(line.toLocal8Bit()) == -1) {
                throw std::runtime_error(msgWriteError.toStdString());
            }
        }
        progressFunction(100);
        targetFile.close();
    }
    void saveAsPlainText(const QString& fileName) const
    { this->saveAsPlainText(fileName, detail::dummyProgressFunction); }

    static QWaveform<T> fromFile(const QString& fileName, std::function<void(int)> progressFunction)
    {
        QFile sourceFile(fileName);
        if (!sourceFile.exists()) {
            throw std::runtime_error(msgFileNotExist.toStdString());
        }
        sourceFile.open(QIODevice::ReadOnly);
        if (!sourceFile.isReadable()) {
            throw std::runtime_error(msgReadError.toStdString());
        }
        auto info = readWaveformHeader(sourceFile);
        double factor = 1/dbToFactor(info.refLevel());
        T signal;
        if constexpr (isComplexSignal<T>) {
            if (info.isComplex()) {
                signal = readBinaryData<QComplexSignal>(sourceFile, info.signalSize(), info.dataType(), factor, progressFunction);
            } else {
                signal = readBinaryData<QRealSignal>(sourceFile, info.signalSize(), info.dataType(), factor, progressFunction);
            }
        } else {
            if (info.isComplex()) {
                throw std::runtime_error("the signal is complex");
            }
            signal = readBinaryData<T>(sourceFile, info.signalSize(), info.dataType(), factor, progressFunction);
        }
        signal.setClock(info.clock());
        return QWaveform<T>(std::move(signal), info.comment(), info.dateTime());
    }
    static QWaveform<T> fromFile(const QString& fileName) {return fromFile(fileName, detail::dummyProgressFunction); }
};

} // namespace detail


using QRealWaveform = detail::QWaveform<QRealSignal>;
using QComplexWaveform = detail::QWaveform<QComplexSignal>;

template <typename T>
inline constexpr bool isWaveform = std::is_same<QRealWaveform, T>::value || std::is_same<QComplexWaveform, T>::value;

template <typename T>
inline constexpr bool isRealWaveform = std::is_same<QRealWaveform, T>::value;

template <typename T>
inline constexpr bool isComplexWaveform =  std::is_same<QComplexWaveform, T>::value;

template <typename T>
inline constexpr bool isWaveformBased = std::is_base_of<QRealWaveform, T>::value || std::is_base_of<QComplexWaveform, T>::value;

template <typename T>
inline constexpr bool isRealWaveformBased = std::is_base_of<QRealWaveform, T>::value;

template <typename T>
inline constexpr bool isComplexWaveformBased =  std::is_base_of<QComplexWaveform, T>::value;

template<class T, class = std::enable_if_t<isSignal<T>>>
QDebug operator<<(QDebug debug, const detail::QWaveform<T>& waveform)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Waveform:" << endl
                    << "    Date: " << waveform.dateTime().toString("dd.MM.yyyy ddd hh:mm:ss") << endl
                    << "    Comment: " << waveform.comment() << endl
                    << "    ";
    if constexpr (std::is_same<T, QRealSignal>::value) {
        debug.nospace() << static_cast<const QRealSignal&>(waveform);
    } else {
        debug.nospace() << static_cast<const QComplexSignal&>(waveform);
    }
    return debug;
}

template <class T, class = std::enable_if_t<isSignalContainer<T>>>
QRealSignal abs(const T& signal)
{
    QRealSignal result(signal.size(), signal.clock());
    std::transform(signal.begin(), signal.end(), result.begin(),
                   [](auto discrete) {
            return abs(discrete);
    });
    return result;
}

inline QRealSignal arg(const QComplexSignal& signal)
{
    QRealSignal result(signal.size(), signal.clock());
    std::transform(signal.begin(), signal.end(), result.begin(),
                   [](auto discrete) { return std::arg(discrete); });
    return result;
}

template<class ForwardIt>
double peak(ForwardIt start, ForwardIt stop)
{
    if(start == stop) {
        return 0;
    }
    double max = abs(*start);
    std::for_each(start, stop, [&max](auto discrete){ max = std::max(max, abs(discrete)); });
    return max;
}

template <class T, class = std::enable_if_t<isSignalContainer<T>>>
double peak(const T& signal)
{
    return peak(signal.begin(), signal.end());
}

template<class ForwardIt>
double rms(ForwardIt start, ForwardIt stop)
{
    if(start == stop) {
        return 0;
    }
    return sqrt((1.0/std::distance(start,stop)) *
                std::accumulate(start, stop, 0.0,
                                [](double sum, auto discrete)
    { return sum += abs(discrete)*abs(discrete); }
    ));
}

template <class T, class = std::enable_if_t<isSignalContainer<T>>>
double rms(const T& signal)
{
    return rms(signal.begin(), signal.end());
}

namespace detail {

template <class InputIt1, class InputIt2>
inline auto basicCorrelation(InputIt1 first1, InputIt1 last1, InputIt2 first2)
{
    if constexpr (std::is_same<decltype (*first1 + *first2), QComplex>::value) {
        return std::inner_product(first1, last1, first2, QComplex(),
                                  [](QComplex sum, auto x) { /*qDebug()<<"x ="<<x;*/ return sum += x; },
        [](auto lhs, auto rhs) {
            if constexpr (std::is_same<typeof (lhs), QComplex>::value) {
                // комплексно сопрягаем отсчёты ВТОРОГО сигнала
                // иначе получим результат с инвертированным
                // относительно нуля частоты спектром
                return lhs * std::conj(rhs);
            } else {
                return lhs * rhs;
            }
        }
        );
    } else {
        return std::inner_product(first1, last1, first2, 0.0);
    }
}

template <class InputIt1, class ReverseIntputIt2>
inline auto basicConvolution(InputIt1 first1, InputIt1 last1, ReverseIntputIt2 first2)
{
    if constexpr (std::is_same<decltype (*first1 + *first2), QComplex>::value) {
        return std::inner_product(first1, last1, first2, QComplex());
    } else {
        return std::inner_product(first1, last1, first2, 0.0);
    }
}

} // namespace detail

template <class T1, class T2, class = std::enable_if_t<hasSignalResult<T1, T2>>>
auto correlation(const T1& signal1, const T2& signal2)
{
    int x = detail::getResultSize(signal1.size(), signal2.size(), Q_FUNC_INFO);
    detail::getResultClock(signal1.clock(), signal2.clock(), Q_FUNC_INFO);
    return detail::basicCorrelation(signal1.begin(), signal1.begin() + x, signal2.begin());

}

template<class T1, class T2, class = std::enable_if_t<isSignalContainer<T1> && isSignalContainer<T2>>>
auto crossCorrelation(const T1& signal1, const T2& signal2, bool keepBeginTransientProcess = true, bool keepEndTransientProcess = false)
{
    int minSize = std::min(signal1.size(), signal2.size());
    int maxSize = std::max(signal1.size(), signal2.size());
    detail::SignalResult<T1, T2> result((keepBeginTransientProcess ? minSize - 1 : 0) +
                                        maxSize - minSize + 1 +
                                        (keepEndTransientProcess ? minSize - 1 : 0),
                                        detail::getResultClock(signal1.clock(), signal2.clock(), Q_FUNC_INFO));

    // Переходный процесс
    if (keepBeginTransientProcess) {
        for (int i = 0; i < minSize - 1; ++i) {
            result[i] = detail::basicCorrelation(signal1.begin(),
                                                  signal1.begin() + (i + 1),
                                                  signal2.end() - (i + 1));
        }
    }

    // Пересечение сигналов
    int start = (keepBeginTransientProcess ? minSize - 1 : 0);
    bool signal1Shorter = signal1.size() < signal2.size();
    for (int i = 0; i < maxSize - minSize + 1; ++i) {
        result[start + i] =
                detail::basicCorrelation(signal1.begin() + (signal1Shorter ? 0 : i),
                                          signal1.begin() + minSize + (signal1Shorter ? 0 : i),
                                          signal2.end() - minSize - (signal1Shorter ? i : 0));
    }

    // Переходный процесс
    if (keepEndTransientProcess) {
        start = maxSize;
        for (int i = 0; i < minSize - 1; ++i) {
            result[start + i] =
                    detail::basicCorrelation(signal1.end() - minSize + i + 1,
                                              signal1.end(),
                                              signal2.begin());
        }
    }
    return result;
}

template<class T, class = std::enable_if_t<isSignalContainer<T>>>
inline auto autoCorrelation(const T& signal)
{
    return crossCorrelation(signal, signal, true, true);
}

template<class T1, class T2, class = std::enable_if_t<isSignalContainer<T1> && isSignalContainer<T2>>>
auto convolution(const T1& signal1, const T2& signal2, bool keepBeginTransientProcess = true, bool keepEndTransientProcess = false)
{
    int minSize = std::min(signal1.size(), signal2.size());
    int maxSize = std::max(signal1.size(), signal2.size());
    detail::SignalResult<T1, T2> result((keepBeginTransientProcess ? minSize - 1 : 0) +
                                        maxSize - minSize + 1 +
                                        (keepEndTransientProcess ? minSize - 1 : 0),
                                        detail::getResultClock(signal1.clock(), signal2.clock(), Q_FUNC_INFO));

    // Переходный процесс
    if (keepBeginTransientProcess) {
        for (int i = 0; i < minSize - 1; ++i) {
            result[i] = detail::basicConvolution(signal1.begin(),
                                                  signal1.begin() + (i + 1),
                                                  signal2.rend() - (i + 1));
        }
    }

    // Пересечение сигналов
    int start = (keepBeginTransientProcess ? minSize - 1:0);
    bool signal1Shorter = signal1.size() < signal2.size();
    for (int i = 0; i < maxSize - minSize + 1; ++i) {
        result[start + i] =
                detail::basicConvolution(signal1.begin() + (signal1Shorter ? 0 : i),
                                          signal1.begin() + minSize + (signal1Shorter ? 0 : i),
                                          signal2.rend() - minSize - (signal1Shorter ? i : 0));
    }

    // Переходный процесс
    if (keepEndTransientProcess) {
        start = maxSize;
        for (int i = 0; i < minSize - 1; ++i) {
            result[start + i] =
                    detail::basicConvolution(signal1.end() - minSize + i + 1,
                                              signal1.end(),
                                              signal2.rbegin());
        }
    }
    return result;
}

template<class T1, class T2, class = std::enable_if_t<isSignalContainer<T1> && isSignalContainer<T2>>>
auto firFilter(const T1& coefficients, const T2& signal, bool keepBeginTransientProcess = true, bool keepEndTransientProcess = false)
{
    return convolution(coefficients, signal, keepBeginTransientProcess, keepEndTransientProcess);
}

template<class T, class = std::enable_if_t<isSignalContainer<T>>>
auto cicFilter(const T &signal, int order, int delay)
{
    if (order <= 0 || delay <= 0) {
        return detail::SignalResult<T, T>(signal.clock(), 0);
    }
    detail::SignalResult<T, T> result(signal.clock(), signal.size());
    std::copy(signal.begin(), signal.end(), result.begin());

    //Проходим N раз связку интегратор-гребенчатый фильтр
    for (int n = 0; n < order; ++n) {
        //Интегратор
        for (int i = 1; i < result.size(); ++i) {
            result[i] = result[i] + result[i-1];
        }
        //Гребенчатый фильтр
        for (int i = result.size()-1; i >= delay; --i) {
            result[i] = result[i] - result[i-delay];
        }
    }
    return result;
}

template<class T, class = std::enable_if_t<isSignalContainer<T>>>
auto decimate(const T &signal, int factor, bool accumulate = false, bool average = false)
{
    detail::SignalResult<T, T> result(signal.clock() / factor, signal.size() / factor);
    if (accumulate) {
        for (int i = 0; i < result.size(); ++i) {
            result[i] = std::accumulate(signal.begin()+factor*i,
                                        signal.begin()+factor*(i+1),
                                        0,
                                        [=](auto sum, auto discrete)
            {
                return sum += discrete ;
            }) / (average ? factor : 1);
        }
    } else {
        for (int i = 0; i < result.size(); ++i) {
            result[i] = signal[factor*i];
        }
    }
    return result;
}

// Дискретное преобразование Фурье является относительно медленным алгоритмом,
// поэтому для высокопроизводительного кода следует использовать
// быстрое преобразование Фурье (QFft)
template<class T, class = std::enable_if_t<isSignalContainer<T>>>
QComplexSignal dft(const T &signal, int size, bool inverted = false)
{
    int resultSize = (signal.size() > size) ? size : signal.size();
    QComplexSignal result(resultSize, signal.clock());
    for (int i = 0; i < resultSize; ++i) {
        QComplexSignal complexExponent = QComplexSignal::harmonic(resultSize, (double)(inverted ? i : -i) / resultSize);
        result[i] = (inverted ? 1 : (1.0/resultSize)) * detail::basicConvolution(signal.begin(), signal.begin() + resultSize, complexExponent.begin());
    }
    return result;
}

class QFft: public QObject {
public:
    explicit QFft(bool inverted = false)
        : inverted_(inverted)
    {}
    explicit QFft(int size, bool inverted = false);
    template<class T, class = std::enable_if_t<isSignalContainer<T>>>
    QComplexSignal compute(const T& signal) const
    {
        if (fftPower_ == 0) {
            return QComplexSignal(signal.clock());
        }
        // Расчёт обратного БПФ ведём путём перемены местами re и im на входе и выходе
        for (int i = 0; i < fftSize_; ++i) {
            if constexpr (isComplexSignalContainer<T>) {
                buffer_[i] = (i < signal.size()) ? (inverted_ ? QComplex(signal[i].imag(), signal[i].real()) : signal[i]) : 0;
            } else {
                buffer_[i] = (i < signal.size()) ? (inverted_ ? QComplex(0, signal[i]) : signal[i]) : 0;
            }
        }
        return this->process_(signal.clock());
    }

    void setPower(int power);
    int power() const { return fftPower_; }
    void setSize(int size){ this->setPower(floor(log2(size))); }
    int size() const { return fftSize_; }
    void setInverted(bool inverted){ inverted_ = inverted; }
    bool inverted() const { return inverted_; }
    template<class T, class = std::enable_if_t<isSignalContainer<T>>>
    QComplexSignal operator()(const T& signal) const
    { return this->compute(signal); }
private:
    static int reverseBits_(int number, int bitCount);
    void updateRotateMultiplers_();
    QComplexSignal process_(QFrequency resultBandwidth) const;

private:
    int fftPower_ = 0;
    int fftSize_ = 0;
    bool inverted_ = false;
    QComplexSignal rotateMultiplers_;
    mutable QComplexSignal buffer_;
};

template<class T, class = std::enable_if_t<isSignalContainer<T>>>
QComplexSignal fft(const T &signal, int size, bool inverted = false)
{
    QFft fft(size, inverted);
    return fft(signal);
}

class QAbstractWindow {
public:
    virtual ~QAbstractWindow() = default;
    virtual const QString& name() const = 0;
    virtual QRealSignal makeWindow(int size) const = 0;
};

class QAlphaParametric {
public:
    QAlphaParametric() = default;
    explicit QAlphaParametric(double alpha)
        :alpha_(alpha)
    {}
    virtual ~QAlphaParametric() = default;
    virtual void setAlpha(double alpha) { alpha_ = alpha; }
    double alpha() const { return alpha_; }

protected:
    double alpha_ = 0;
};

class QBartlettHannWindow: public QAbstractWindow {
public:
    QBartlettHannWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QBlackmanWindow: public QAbstractWindow {
public:
    QBlackmanWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QBlackmanHarrisWindow: public QAbstractWindow {
public:
    QBlackmanHarrisWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QBohmanWindow: public QAbstractWindow {
public:
    QBohmanWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QChebyshevWindow: public QAbstractWindow, public QAlphaParametric {
public:
    static constexpr double defaultAlphaValue = 4.0;

    explicit QChebyshevWindow(double alpha = defaultAlphaValue)
        : QAbstractWindow(), QAlphaParametric::QAlphaParametric(alpha)
    {}

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size, double alpha = defaultAlphaValue);
    QRealSignal makeWindow(int size) const override { return generate(size, alpha_); }
};

class QFlattopWindow: public QAbstractWindow {
public:
    QFlattopWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QGaussianWindow: public QAbstractWindow, public QAlphaParametric {
public:
    static constexpr double defaultAlphaValue = 3.0;

    explicit QGaussianWindow(double alpha = defaultAlphaValue)
        : QAbstractWindow(), QAlphaParametric::QAlphaParametric(alpha)
    {}

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size, double alpha = defaultAlphaValue);
    QRealSignal makeWindow(int size) const override { return generate(size, alpha_); }
};

class QHammingWindow: public QAbstractWindow, public QAlphaParametric {
public:
    static constexpr double defaultAlphaValue = 0.53856;

    explicit QHammingWindow(double alpha = defaultAlphaValue)
        : QAbstractWindow(), QAlphaParametric::QAlphaParametric(alpha)
    {}

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size, double alpha = defaultAlphaValue);
    QRealSignal makeWindow(int size) const override { return generate(size, alpha_); }
};

class QHannWindow: public QAbstractWindow {
public:
    QHannWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QKaiserWindow: public QAbstractWindow, public QAlphaParametric {
public:
    static constexpr double defaultAlphaValue = 3.0;

    explicit QKaiserWindow(double alpha = defaultAlphaValue)
        : QAbstractWindow(), QAlphaParametric::QAlphaParametric(alpha)
    {}

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size, double alpha = defaultAlphaValue);
    QRealSignal makeWindow(int size) const override { return generate(size, alpha_); }
};

class QNuttallWindow: public QAbstractWindow {
public:
    QNuttallWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QParzenWindow: public QAbstractWindow {
public:
    QParzenWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QTriangularWindow: public QAbstractWindow {
public:
    QTriangularWindow() = default;

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

class QTukeyWindow: public QAbstractWindow, public QAlphaParametric {
public:
    static constexpr double defaultAlphaValue = 0.5;

    explicit QTukeyWindow(double alpha = defaultAlphaValue)
        : QAbstractWindow(), QAlphaParametric::QAlphaParametric(alpha)
    {}

    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size, double alpha = defaultAlphaValue);
    QRealSignal makeWindow(int size) const override { return generate(size, alpha_); }
};

class QRectangleWindow: public QAbstractWindow {
public:
    QRectangleWindow() = default;
    static const QString windowName;
    const QString& name() const override { return windowName; }

    static QRealSignal generate(int size);
    QRealSignal makeWindow(int size) const override { return generate(size); }
};

namespace modulation {
QRealSignal amplitude(const QRealSignal& source, double M, double normFreq, double U = 1.0, double phase = 0);
QRealSignal frequency(const QRealSignal& source, double m, double normFreq, double U = 1.0, double phase = 0);
QRealSignal phase(const QRealSignal& source, double m, double normFreq, double U = 1.0, double phase = 0);
} // namespace modulation

} // namespace dsp

QDebug operator<<(QDebug debug, dsp::QComplex value);

inline constexpr dsp::QFrequency operator"" _Hz(long double value)
{
    return dsp::QFrequency(value);
}

inline constexpr dsp::QFrequency operator"" _Hz(unsigned long long value)
{
    return dsp::QFrequency(value);
}

inline constexpr dsp::QFrequency operator"" _kHz(long double value)
{
    return dsp::QFrequency(value*1e3);
}

inline constexpr dsp::QFrequency operator"" _kHz(unsigned long long value)
{
    return dsp::QFrequency(value*1e3);
}

inline constexpr dsp::QFrequency operator"" _MHz(long double value)
{
    return dsp::QFrequency(value*1e6);
}

inline constexpr dsp::QFrequency operator"" _MHz(unsigned long long value)
{
    return dsp::QFrequency(value*1e6);
}

inline constexpr dsp::QFrequency operator"" _GHz(long double value)
{
    return dsp::QFrequency(value*1e9);
}

inline constexpr dsp::QFrequency operator"" _GHz(unsigned long long value)
{
    return dsp::QFrequency(value*1e9);
}

Q_DECLARE_METATYPE(dsp::QComplex)
Q_DECLARE_METATYPE(dsp::QComplex*)
Q_DECLARE_METATYPE(dsp::QFrequency)
Q_DECLARE_METATYPE(dsp::QFrequency*)
Q_DECLARE_METATYPE(dsp::QRealSignal)
Q_DECLARE_METATYPE(dsp::QRealSignal*)
Q_DECLARE_METATYPE(dsp::QComplexSignal)
Q_DECLARE_METATYPE(dsp::QComplexSignal*)
Q_DECLARE_METATYPE(dsp::QRealWaveform)
Q_DECLARE_METATYPE(dsp::QRealWaveform*)
Q_DECLARE_METATYPE(dsp::QComplexWaveform)
Q_DECLARE_METATYPE(dsp::QComplexWaveform*)

#endif // QDSP_H

#ifndef RSWAVEFORM_H
#define RSWAVEFORM_H

#include "qdsp.h"

namespace dsp {

// Для форматов сигналов оборудования фирмы Rohde&Schwarz
class QRSWaveform{
public:
    QRSWaveform() = delete;

    static void toWvFile(const QComplexWaveform& waveform, const QString &fileName, std::function<void(int)> progressFunction);
    static void toWvFile(const QComplexWaveform& waveform, const QString &fileName)
    { toWvFile(waveform, fileName, detail::dummyProgressFunction); }
    static void toWvhFile(const QComplexWaveform& waveform, const QString &fileName, std::function<void(int)> progressFunction);
    static void toWvhFile(const QComplexWaveform& waveform, const QString &fileName)
    { toWvhFile(waveform, fileName, detail::dummyProgressFunction); }

    static QComplexWaveform fromFile(const QString& fileName, std::function<void(int)> progressFunction);
    static QComplexWaveform fromFile(const QString& fileName)
    { return fromFile(fileName, detail::dummyProgressFunction); }
    static QComplexWaveform fromWvFile(const QString& fileName, std::function<void(int)> progressFunction);
    static QComplexWaveform fromWvFile(const QString& fileName)
    { return fromWvFile(fileName, detail::dummyProgressFunction); }
    static QComplexWaveform fromTarFile(const QString& fileName, std::function<void(int)> progressFunction);
    static QComplexWaveform fromTarFile(const QString& fileName)
    { return fromTarFile(fileName, detail::dummyProgressFunction); }
    static QComplexWaveform fromWvhFile(const QString& fileName, std::function<void(int)> progressFunction, double loadResistance = 50);
    static QComplexWaveform fromWvhFile(const QString& fileName, double loadResistance = 50)
    { return fromWvhFile(fileName, detail::dummyProgressFunction, loadResistance); }
};

} // namespace dsp

#endif // RSWAVEFORM_H

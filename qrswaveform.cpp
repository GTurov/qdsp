#include "qrswaveform.h"

#include <QDataStream>
#include <QDateTime>
#include <QHash>
#include <QVariant>
#include <QDomDocument>

Q_DECLARE_METATYPE(QSharedPointer<dsp::QComplexSignal>)

QString readSectionName(QIODevice& input)
{
    QString result;
    while (!input.atEnd()) {
        char c;
        input.getChar(&c);
        if (c == ':') {
            return result;
        } else {
            result.append(c);
        }
        if (c == '}') {
            throw std::runtime_error(dsp::detail::msgInvalidFileFormat.toStdString());
        }
    }
    return result;
}

QString readString(QIODevice& input)
{
    QString result;
    char c;
    while (!input.atEnd()) {
        input.getChar(&c);
        if (c == '}') {
            return result;
        } else {
            if (result.isEmpty() && c == ' ') {
                continue;
            }
            result.append(c);
        }
    }
    return result;
}

QHash<QString, QVariant> readXmlTags(const QByteArray& data)
{
    QString str(data);
    QHash<QString, QVariant> result;
    QDomDocument doc("tags");
    if (!doc.setContent(data)) {
        qDebug() << "error";
        return result;
    }

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull()) {
            result[e.tagName()] = e.text();
        }
        n = n.nextSibling();
    }
    return result;
}

QHash<QString, QVariant> readRsTags(QIODevice &input, std::function<void(int)> progressFunction)
{
    QHash<QString, QVariant> result;
    while (!input.atEnd()) {
        char c;
        input.getChar(&c);
        if (c == '{') {
            QString section = readSectionName(input);
            if (section.startsWith("WAVEFORM-")) {
                int num = section.section('-', -1).toInt();
                input.getChar(&c); // ' ' or '#'
                if (c != '#') {
                    input.getChar(&c); // '#'
                }
                QSharedPointer<dsp::QComplexSignal> dataPtr(new dsp::QComplexSignal);
                *dataPtr = dsp::detail::readBinaryData<dsp::QComplexSignal, int16_t>(input, (num - 1) / (2*sizeof(int16_t)), 1, progressFunction);
                result["WAVEFORM"] = QVariant::fromValue(dataPtr);
            } else if (section.startsWith("WWAVEFORM-")) {
                qDebug()<<"Encrypted waveform format. No data available";
            } else {
                result[section] = readString(input);
            }
        }
    }
    return result;
}

namespace dsp {

void QRSWaveform::toWvFile(const QComplexWaveform &waveform, const QString &fileName, std::function<void (int)> progressFunction)
{
    if (fileName == "") {
        throw std::invalid_argument(detail::msgInvalidFileName.toStdString());
    }

    QFile file(fileName);
    file.open(QIODevice::WriteOnly);

    if (!file.isWritable()) {
        throw std::runtime_error(detail::msgWriteError.toStdString());
    }

    if (!waveform.hasClock()) {
        throw std::runtime_error(detail::msgNoClockError.toStdString());
    }

    QByteArray data;
    data.append("{TYPE: SMU-WV,0}");
    if (!waveform.comment().isEmpty()) {
        data.append(QString("{COMMENT:%1}").arg(waveform.comment()));
    }
    data.append(QString("{DATE:%1}").arg(waveform.dateTime().toString("yyyy-MM-dd;hh:mm:ss")));
    data.append(QString("{CLOCK:%1}").arg(waveform.clock().Hz()));
    data.append(QString("{CLOCK MARKER:%1}").arg(waveform.clock().Hz()));
    double wfRms = rms(waveform);
    double wfPeak = peak(waveform);
    data.append(QString("{LEVEL OFFS:%1,%2}").arg(factorToDb(1.0/wfRms)).arg(factorToDb(1.0/wfPeak)));
    data.append(QString("{SAMPLES:%1}").arg(waveform.size()));
    data.append(QString("{CONTROL LENGTH:%1}").arg(waveform.size()));
    data.append(QString("{WAVEFORM-%1:").arg(waveform.size() * 4 + 1));
    data.append(' ');
    data.append('#');
    file.write(data);
    detail::writeBinaryData<QComplexSignal, int16_t>(file, (QComplexSignal&)waveform, std::numeric_limits<int16_t>::max(), progressFunction);
    file.write("}",1);
    file.close();
}

void QRSWaveform::toWvhFile(const QComplexWaveform &waveform, const QString &fileName, std::function<void (int)> progressFunction)
{
    if (fileName == "") {
        throw std::invalid_argument(detail::msgInvalidFileName.toStdString());
    }

    QFile headerFile(fileName);
    headerFile.open(QIODevice::WriteOnly);
    if (!headerFile.isWritable()) {
        throw std::runtime_error(detail::msgWriteError.toStdString());
    }
    if (!waveform.hasClock()) {
        throw std::runtime_error(detail::msgNoClockError.toStdString());
    }

    QByteArray data;
    data.append("{TYPE:RAW16LE}{COMPONENTS:IQ}");
    if (!waveform.comment().isEmpty()) {
        data.append(QString("{COMMENT:%1}").arg(waveform.comment()));
    }
    data.append(QString("{DATE:%1}").arg(waveform.dateTime().toString("yyyy-MM-dd;hh:mm:ss")));
    data.append(QString("{CLOCK:%1}").arg(waveform.clock().Hz()));
    data.append("{FREQUENCY:0.000000}");
    double wfPeak = peak(waveform);
    data.append(QString("{REFLEVEL:%1}").arg(factorToDb(1.0/wfPeak)));
    headerFile.write(data);
    headerFile.close();

    QString dataFileName = fileName;
    dataFileName.replace(QRegExp("wvh$"), "wvd");
    QFile dataFile(dataFileName);
    dataFile.open(QIODevice::WriteOnly);
    if (!dataFile.isWritable()) {
        throw std::runtime_error(detail::msgWriteError.toStdString());
    }
    detail::writeBinaryData<QComplexSignal, int16_t>(dataFile, (QComplexSignal&)waveform, std::numeric_limits<int16_t>::max()/wfPeak, progressFunction);
    dataFile.close();
}

QComplexWaveform QRSWaveform::fromFile(const QString& fileName, std::function<void(int)> progressFunction)
{
    if (fileName.section('.',-2,-1) == "iq.tar") {
        return fromTarFile(fileName, progressFunction);
    } else if (fileName.section('.',-1,-1) == "wv") {
        return fromWvFile(fileName, progressFunction);
    } else if (fileName.section('.',-1,-1) == "wvh") {
        return fromWvhFile(fileName, progressFunction);
    } else {
        throw std::invalid_argument("unknown file extension");
    }
}

QComplexWaveform QRSWaveform::fromWvFile(const QString& fileName, std::function<void(int)> progressFunction)
{
    QFile file(fileName);
    if (!file.exists()) {
        throw std::runtime_error(detail::msgFileNotExist.toStdString());
    }
    file.open(QIODevice::ReadOnly);
    if (!file.isReadable()) {
        throw std::runtime_error(detail::msgReadError.toStdString());
    }
    auto tags = readRsTags(file, progressFunction);
    file.close();
    auto date = QDateTime::fromString(tags["DATE"].toString(),"yyyy-MM-dd;hh:mm:ss");
    auto dataPtr = tags["WAVEFORM"].value<QSharedPointer<dsp::QComplexSignal>>();
    auto comment = tags["COMMENT"].toString();
    dataPtr->setClock(QFrequency(tags["CLOCK"].toString().toDouble()));
    // В соответствии с мануалом R&S:
    // Пиковое значение
    double wfPeak =  dsp::dbToFactor(tags["LEVEL OFFS"].toString().section(',',1,1).toDouble());
    // Пиковое значение считанного сигнала
    double dataPeak = dsp::peak(*dataPtr);
    *dataPtr *= 1 / (wfPeak * dataPeak);
    return QComplexWaveform(std::move(*dataPtr), std::move(comment), date);
}

QComplexWaveform QRSWaveform::fromTarFile(const QString& fileName, std::function<void(int)> progressFunction)
{
    QFile file(fileName);
    if (!file.exists()) {
        throw std::runtime_error(detail::msgFileNotExist.toStdString());
    }
    file.open(QIODevice::ReadOnly);
    if (!file.isReadable()) {
        throw std::runtime_error(detail::msgReadError.toStdString());
    }
    double clock = 0;
    QString comment;
    QDateTime date;

    QString name;
    int position = 0;
    int size = 0;
    QComplexSignal data;
    while (!file.atEnd()) {
        QByteArray header = file.read(512);
        name = header.mid(0,100);
        bool ok;
        size = header.mid(124,11).toInt(&ok,8);
        if (name.endsWith(".complex.1ch.float32")) {
            data = detail::readBinaryData<QComplexSignal, float>(file, size / (2*sizeof(float)), 1, progressFunction);
        }
        if (name.endsWith("xml")) {
            auto tags = readXmlTags(file.read(size));
            clock = tags["Clock"].toDouble();
            comment = tags["Comment"].toString();
            date = QDateTime::fromString(tags["DateTime"].toString(),"yyyy-MM-ddThh:mm:ss");
        }
        position += 512 + ((size+512-1) / 512)*512;
        file.seek(position);
    }
    data.setClock(QFrequency(clock));
    return QComplexWaveform(std::move(data), std::move(comment), date);
}

QComplexWaveform QRSWaveform::fromWvhFile(const QString& fileName, std::function<void(int)> progressFunction, double loadResistance)
{
    QFile file(fileName);
    if (!file.exists()) {
        throw std::runtime_error(detail::msgFileNotExist.toStdString());
    }
    file.open(QIODevice::ReadOnly);
    if (!file.isReadable()) {
        throw std::runtime_error(detail::msgReadError.toStdString());
    }
    auto tags = readRsTags(file, progressFunction);
    file.close();
    auto comment = tags["COMMENT"].toString();
    auto date = QDateTime::fromString(tags["DATE"].toString(),"yyyy-MM-dd;hh:mm:ss");


    QString dataFileName = fileName;
    dataFileName.replace(QRegExp("wvh$"), "wvd");
    QFile dataFile(dataFileName);
    dataFile.open(QIODevice::ReadOnly);
    QComplexSignal data = detail::readBinaryData<QComplexSignal, int16_t>(dataFile, dataFile.size() / (2*sizeof(int16_t)), 1, progressFunction);
    dataFile.close();
    qreal factor = dsp::dbmToVolts(tags["REFLEVEL"].toString().toDouble(), loadResistance)
            / std::numeric_limits<int16_t>::max();
    data *= factor;
    data.setClock(QFrequency(tags["CLOCK"].toDouble()));
    return QComplexWaveform(std::move(data), std::move(comment), date);
}

} // namespace dsp

#include "maccapturethread.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioSource>
#include <QThread>
#include <QDebug>

/**
 * @class MacCaptureThread
 * @brief Implementierung eines Threads zur Audioaufnahme auf macOS mit Fokus auf das virtuelle Gerät "BlackHole".
 *
 * Diese Klasse erweitert CaptureThread und ermöglicht das Einlesen von Audiodaten über das virtuelle Audiogerät "BlackHole",
 * um z. B. Systemsound oder andere interne Audioquellen aufzuzeichnen.
 *
 * Der Thread liest kontinuierlich Audiodaten und stellt sie über das Signal `audioDataReady` bereit.
 *
 * @author Pakize Gökkaya
 */
MacCaptureThread::MacCaptureThread(QObject *parent)
    : CaptureThread(parent), audioSource(nullptr), inputDevice(nullptr), running(false)
{
}

/**
 * @brief Destruktor.
 * Beendet ggf. laufende Aufnahmen und gibt Ressourcen frei.
 */
MacCaptureThread::~MacCaptureThread()
{
    cleanupCapture();
}

/**
 * @brief Initialisiert die Audioaufnahme über das "BlackHole"-Gerät.
 *
 * Diese Methode sucht nach einem Audioeingabegerät, dessen Beschreibung "BlackHole" enthält (nicht case-sensitiv).
 * Wenn gefunden, wird ein QAudioSource mit 44.1 kHz, 1 Kanal, Float-Format erstellt und gestartet.
 *
 * Bei erfolgreicher Initialisierung wird ein Slot an readyRead des QIODevice gebunden, um kontinuierlich Daten zu lesen.
 *
 * @return true, wenn das Gerät erfolgreich gefunden und gestartet wurde; false andernfalls.
 */
bool MacCaptureThread::initializeCapture()
{
    QAudioDevice device;
    const auto devices = QMediaDevices::audioInputs();
    for (const auto &d : devices) {
        qDebug() << "Verfügbares Gerät:" << d.description();
        if (d.description().contains("BlackHole", Qt::CaseInsensitive)) {
            device = d;
            break;
        }
    }

    if (device.isNull()) {
        qWarning() << " Kein BlackHole-Gerät gefunden!";
        return false;
    }

    qDebug() << " efundenes Gerät:" << device.description();

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);

    audioSource = new QAudioSource(device, format);
    audioSource->moveToThread(QThread::currentThread());
    inputDevice = audioSource->start();

    if (inputDevice) {
        connect(inputDevice, &QIODevice::readyRead, this, [this]() {
            const qint64 len = inputDevice->bytesAvailable();
            if (len > 0) {
                QByteArray data = inputDevice->read(len);
                audioBuffer.append(data);
                emit audioDataReady(data);
                qDebug() << "🎙 Event: Gelesen " << data.size() << " Bytes";
            }
        });
    } else {
        qWarning() << "⚠️ inputDevice ist null!";
        return false;
    }

    running = true;
    return true;
}

/**
 * @brief Liest verfügbare Audiodaten und speichert sie im internen Puffer.
 *
 * Diese Methode kann z. B. regelmäßig aus einem Timer oder einer Schleife heraus aufgerufen werden.
 * Sie ergänzt das Verhalten von `initializeCapture`, falls `readyRead` nicht verwendet wird.
 */
void MacCaptureThread::captureLoopIteration()
{
    if (!inputDevice) return;

    const qint64 len = inputDevice->bytesAvailable();
    if (len > 0) {
        QByteArray data = inputDevice->read(len);
        audioBuffer.append(data);
        emit audioDataReady(data);
    }
}

/**
 * @brief (Deaktiviert) Hauptausführungsschleife des Threads.
 *
 * Diese Methode startet `initializeCapture()` und liest in einer Schleife weiter Daten, bis `running` oder `m_shutdown` false ergibt.
 */
/*
void MacCaptureThread::run()
{
    qDebug() << "MacCaptureThread::run gestartet";

    if (!initializeCapture()) {
        return;
    }

    while (running && !m_shutdown.load()) {
        QThread::msleep(10);
        captureLoopIteration();
    }

    cleanupCapture();
}
*/

/**
 * @brief Beendet die Audioaufnahme.
 *
 * Setzt das interne Flag `running` auf false und stoppt den QAudioSource, falls vorhanden.
 * Dies unterbricht die laufende Aufnahme.
 */
void MacCaptureThread::stop()
{
    running = false;

    if (audioSource) {
        audioSource->stop();
    }
}

/**
 * @brief Gibt belegte Ressourcen wieder frei.
 *
 * Stoppt und löscht ggf. den QAudioSource und setzt das Eingabegerät auf nullptr zurück.
 */
void MacCaptureThread::cleanupCapture()
{
    if (audioSource) {
        audioSource->stop();
        delete audioSource;
        audioSource = nullptr;
    }
    inputDevice = nullptr;
    running = false;
}

/**
 * @brief Gibt den internen Audiopuffer zurück.
 *
 * Der Puffer enthält alle bisher aufgezeichneten Audiodaten (ungefiltert).
 *
 * @return QByteArray mit den gesammelten Audiodaten.
 */
QByteArray MacCaptureThread::getBuffer() const
{
    return audioBuffer;
}

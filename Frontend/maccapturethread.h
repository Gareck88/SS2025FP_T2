#ifndef MACCAPTURETHREAD_H
#define MACCAPTURETHREAD_H

#include <QThread>
#include <QAudioSource>
#include <QIODevice>

#include "capturethread.h"

/**
 * @class MacCaptureThread
 * @brief Audioaufnahme-Thread für macOS mit Unterstützung für das virtuelle Gerät "BlackHole".
 *
 * Diese Klasse implementiert die Audioaufnahmefunktionalität für macOS, insbesondere für die Nutzung
 * von virtuellen Audiogeräten wie „BlackHole“. Sie erbt von CaptureThread und bietet Methoden zur Initialisierung,
 * Pufferung und Beendigung der Audioaufnahme.
 *
 * Audiodaten werden bei Verfügbarkeit aus dem Eingabegerät gelesen und per Signal `audioDataReady` weitergereicht.
 *
 * @author Pakize Gökkaya
 */
class MacCaptureThread : public CaptureThread
{
    Q_OBJECT

public:
    /**
     * @brief
     * @param
     */
    explicit MacCaptureThread(QObject *parent = nullptr);


    ~MacCaptureThread() override;

    /**
     * @brief Initialisiert die Audioaufnahme.
     * @return true, wenn erfolgreich initialisiert wurde, sonst false.
     */
    bool initializeCapture() override;

    /**
     * @brief Führt eine einzelne Iteration der Audioaufnahme durch.
     * Liest ggf. vorhandene Daten aus dem Eingabegerät.
     */
    void captureLoopIteration() override;

    /**
     * @brief Bereinigt alle Ressourcen, stoppt Audioquelle und setzt interne Zustände zurück.
     */
    void cleanupCapture() override;

    /**
     * @brief Stoppt die Aufnahme explizit.
     */
    void stop();

    /**
     * @brief Gibt den aufgezeichneten Audiopuffer zurück.
     * @return QByteArray mit gesammelten Audiodaten.
     */
    QByteArray getBuffer() const;


    /** void run() override; */

signals:
    /**
     * @brief Signal, das ausgelöst wird, wenn neue Audiodaten verfügbar sind.
     * @param data Die gelesenen Rohdaten aus dem Eingabegerät.
     */
    void audioDataReady(const QByteArray &data);

private:
    QAudioSource *audioSource;   ///< Pointer zur Audioquelle
    QIODevice *inputDevice;      ///< Pointer zum Eingabegerät (z. B. vom AudioSource erzeugt)
    QByteArray audioBuffer;      ///< Zwischenspeicher für gelesene Audiodaten
    bool running;                ///< Steuerung des Aufnahmezustands
};

#endif // MACCAPTURETHREAD_H

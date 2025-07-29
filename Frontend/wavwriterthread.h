/**
 * @file wavwriterthread.h
 * @brief Enthält die Deklaration des WavWriterThread zum Schreiben von Audio-Dateien.
 * @author Mike Wild
 */
#ifndef WAVWRITERTHREAD_H
#define WAVWRITERTHREAD_H

#include <QFile>
#include <QList>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

/**
 * @brief Ein dedizierter Thread, der Audio-Daten in WAV-Dateien schreibt.
 *
 * Diese Klasse wird verwendet, um das Schreiben von Dateien von anderen Threads
 * (insbesondere dem Haupt-Thread und dem Aufnahme-Thread) zu entkoppeln.
 * Sie nimmt über den `writeChunk`-Slot Audiodaten entgegen und schreibt diese in
 * ihrem eigenen Thread-Kontext auf die Festplatte. Sie erzeugt parallel zwei Dateien:
 * eine hochauflösende Stereo-Datei und eine für die Spracherkennung (ASR)
 * optimierte, heruntergesampelte Mono-Datei.
 */
class WavWriterThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Standard-Konstruktor.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit WavWriterThread (QObject *parent = nullptr);

    /**
     * @brief Destruktor. Ruft shutdown() auf, um den Thread sicher zu beenden.
     */
    ~WavWriterThread ();

    /**
     * @brief Startet eine neue Schreib-Session.
     *
     * Öffnet die beiden Zieldateien und bereitet den Thread auf das Empfangen
     * von Audio-Daten vor.
     * @param hqPath Pfad für die hochauflösende WAV-Datei.
     * @param asrPath Pfad für die ASR-optimierte WAV-Datei.
     */
    void startWriting (const QString &hqPath, const QString &asrPath);

    /**
     * @brief Beendet den Thread vollständig und wartet auf dessen Terminierung.
     */
    void shutdown ();

public slots:
    /**
     * @brief Nimmt einen Block von Audio-Daten entgegen und fügt ihn dem internen Puffer hinzu.
     * @note Dieser Slot ist thread-sicher und dazu gedacht, mit dem
     * `pcmChunkReady`-Signal des CaptureThread verbunden zu werden.
     * @param chunk Ein Block von Audio-Daten (2 Kanäle, 32-bit float).
     */
    void writeChunk (QList<float> chunk);

    /**
     * @brief Beendet die aktuelle Schreib-Session.
     *
     * Veranlasst den Thread, alle verbleibenden Daten aus dem Puffer zu schreiben,
     * die WAV-Header zu finalisieren und die Dateien zu schließen.
     */
    void stopWriting ();

signals:
    /**
     * @brief Wird gesendet, nachdem eine Schreib-Session vollständig abgeschlossen
     * und die Dateien geschlossen wurden.
     */
    void finishedWriting ();

protected:
    /**
     * @brief Die Hauptfunktion des Threads (der "Consumer"-Teil).
     *
     * Implementiert die Logik zum Warten auf Daten, Schreiben der Daten auf die
     * Festplatte und Finalisieren der Dateien.
     */
    void run () override;

private:
    /**
     * @brief Schreibt die finalen WAV-Header in die Dateien, nachdem alle Daten geschrieben wurden.
     * @param hqBytes Die Gesamtgröße der geschriebenen Audiodaten für die HQ-Datei.
     * @param asrBytes Die Gesamtgröße der geschriebenen Audiodaten für die ASR-Datei.
     */
    void writeHeaders (qint64 hqBytes, qint64 asrBytes);

    /**
     * @brief Schreibt den aktuellen Inhalt des internen Puffers auf die Festplatte.
     * @param buffer Der Puffer, dessen Inhalt geschrieben werden soll.
     */
    void writeCurrentBufferToDisk (QList<float> &buffer);

    QFile m_hqFile;  ///< Dateihandle für die High-Quality-WAV-Datei.
    QFile m_asrFile; ///< Dateihandle für die ASR-WAV-Datei.

    // Synchronisation und Zustand
    QMutex m_mutex;
    QWaitCondition m_mainLoopCond; ///< Weckt den Thread auf, wenn startWriting() gerufen wird.
    QWaitCondition
        m_dataAvailableCond;    ///< Weckt den Thread auf, wenn neue Daten in writeChunk() ankommen.
    std::atomic<bool> m_active; ///< Steuert die aktive Schreibschleife.
    std::atomic<bool> m_shutdown; ///< Signalisiert dem Thread, sich komplett zu beenden.

    // Puffer und Zähler
    QList<float> m_bufferFloat;   ///< Interner Puffer für ankommende Audio-Chunks.
    qint64 m_hqBytesWritten;      ///< Zähler für geschriebene Bytes (HQ).
    qint64 m_asrBytesWritten;     ///< Zähler für geschriebene Bytes (ASR).
    qint64 m_flushThresholdBytes; ///< Pufferschwelle in Bytes, bevor auf die Platte geschrieben wird.
    int m_downsampleOffset;       ///< Offset für das Downsampling zur ASR-Version.

    // Audio-Format-Konstanten
    const int m_sampleRateHQ;    ///< Sample-Rate für die HQ-Aufnahme (z.B. 48000 Hz).
    const int m_channelsHQ;      ///< Anzahl der Kanäle für die HQ-Aufnahme (z.B. 2 für Stereo).
    const int m_bitsPerSampleHQ; ///< Bittiefe für die HQ-Aufnahme (z.B. 32 für float).
    const int m_sampleRateASR;   ///< Ziel-Sample-Rate für die ASR-Aufnahme (z.B. 16000 Hz).
};

#endif // WAVWRITERTHREAD_H

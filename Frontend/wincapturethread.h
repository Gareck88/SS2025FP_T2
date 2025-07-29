/**
 * @file wincapturethread.h
 * @brief Enthält die Deklaration der WinCaptureThread-Klasse für die Audioaufnahme unter Windows.
 * @author Mike Wild
 */
#ifndef WINCAPTURETHREAD_H
#define WINCAPTURETHREAD_H

#include "capturethread.h"
#include "ringbuffer.h"
#include <audioclient.h>
#include <mmdeviceapi.h>

/**
 * @brief Eine konkrete Implementierung von CaptureThread für Windows-Systeme.
 *
 * Diese Klasse nutzt die Windows Audio Session API (WASAPI) zur Aufnahme von Audio.
 * Sie initialisiert zwei separate Audio-Clients: einen für das Standard-Ausgabegerät
 * im Loopback-Modus (um System-Sounds aufzunehmen) und einen für das Standard-Eingabegerät
 * (Mikrofon). Die Daten beider Streams werden in Ringpuffern zwischengespeichert,
 * resampelt, gemischt und dann zur weiteren Verarbeitung gesendet.
 */
class WinCaptureThread : public CaptureThread
{
    Q_OBJECT
public:
    /**
     * @brief Standard-Konstruktor.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit WinCaptureThread (QObject *parent = nullptr);

protected:
    /**
     * @brief Überschriebene Hauptfunktion des Threads zur Initialisierung von COM.
     * @note Da WASAPI auf COM basiert, muss für diesen Thread explizit die
     * COM-Bibliothek initialisiert (CoInitializeEx) und wieder freigegeben (CoUninitialize) werden.
     */
    void run() override;

    /**
     * @brief Initialisiert die WASAPI-Audio-Clients für System- und Mikrofon-Aufnahme.
     * @return true bei Erfolg, andernfalls false.
     */
    bool initializeCapture() override;

    /**
     * @brief Führt eine einzelne Iteration der Aufnahmeschleife aus.
     *
     * Liest verfügbare Datenpakete von beiden WASAPI-Clients, schreibt sie in die
     * Ringpuffer, führt ein Resampling auf 48kHz durch und mischt die Streams.
     */
    void captureLoopIteration() override;

    /**
     * @brief Gibt alle WASAPI- und COM-Ressourcen frei.
     *
     * Stoppt die Audio-Clients und gibt die Referenzen auf alle COM-Interfaces frei.
     */
    void cleanupCapture() override;

private:
    // --- COM-Interfaces für System-Audio (Loopback) ---
    IAudioClient *m_audioClientSys = nullptr; ///< Der WASAPI-Client für das Ausgabe-Gerät.
    IAudioCaptureClient *m_captureClientSys
        = nullptr;                    ///< Der Client zum Abgreifen der Loopback-Daten.
    IMMDevice *m_deviceSys = nullptr; ///< Repräsentiert das Standard-Ausgabegerät (z.B. Lautsprecher).
    IMMDeviceEnumerator *m_deviceEnumerator
        = nullptr; ///< Wird zur Auflistung und Auswahl von Audio-Geräten verwendet.

    // --- COM-Interfaces für Mikrofon-Audio ---
    IAudioClient *m_audioClientMic = nullptr; ///< Der WASAPI-Client für das Eingabe-Gerät.
    IAudioCaptureClient *m_captureClientMic
        = nullptr;                    ///< Der Client zum Abgreifen der Mikrofon-Daten.
    IMMDevice *m_deviceMic = nullptr; ///< Repräsentiert das Standard-Eingabegerät (Mikrofon).

    // --- Timing und Resampling ---
    LARGE_INTEGER m_perfCounterFreq; ///< Frequenz des High-Performance Timers für präzises Timing.
    LARGE_INTEGER m_lastTime;        ///< Letzter Zeitstempel für die Delta-Zeit-Berechnung.
    double m_sampleAccumulator
        = 0.0; ///< Akkumulator für eine fließkomma-genaue Frame-Zählung beim Resampling.
    double m_resampPosMic = 0.0; ///< Aktuelle Leseposition im Mikrofon-Ringpuffer.
    double m_resampPosSys = 0.0; ///< Aktuelle Leseposition im System-Audio-Ringpuffer.
    const int m_pollingIntervalMs
        = 10; ///< Das Intervall in ms, in dem neue Audiodaten abgefragt werden.

    // --- Ringpuffer ---
    RingBuffer m_fifoSysL; ///< Ringpuffer für den linken Kanal des System-Audios.
    RingBuffer m_fifoMicL; ///< Ringpuffer für den linken Kanal des Mikrofons.
    RingBuffer m_fifoSysR; ///< Ringpuffer für den rechten Kanal des System-Audios.
    RingBuffer m_fifoMicR; ///< Ringpuffer für den rechten Kanal des Mikrofons.

    // --- Geräte-Eigenschaften ---
    UINT32 m_nativeSampleRateSys = 0; ///< Native Abtastrate des System-Audio-Geräts.
    UINT32 m_nativeChannelsSys = 0;   ///< Native Kanalanzahl des System-Audio-Geräts.
    UINT32 m_nativeSampleRateMic = 0; ///< Native Abtastrate des Mikrofons.
    UINT32 m_nativeChannelsMic = 0;   ///< Native Kanalanzahl des Mikrofons.
};

#endif // WINCAPTURETHREAD_H

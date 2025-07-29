/**
 * @file pulsecapturethread.h
 * @brief Enthält die Deklaration der PulseCaptureThread-Klasse für die Audioaufnahme unter Linux.
 * @author Mike Wild
 */
#ifndef PULSECAPTURETHREAD_H
#define PULSECAPTURETHREAD_H

#include "capturethread.h"
#include <vector>

// Forward-Deklaration für die undurchsichtige PulseAudio-Struktur
struct pa_simple;

/**
 * @brief Eine konkrete Implementierung von CaptureThread für Linux-Systeme mit PulseAudio.
 *
 * Diese Klasse implementiert die Audio-Aufnahme durch die Interaktion mit dem
 * PulseAudio-Soundserver. Sie verwendet Kommandozeilen-Aufrufe von `pactl`, um
 * eine virtuelle Audio-Senke (null-sink) und ein Loopback-Gerät für das Mikrofon
 * zu erstellen. Dies ermöglicht das getrennte Abgreifen von System-Audio und
 * Mikrofon-Audio. Die eigentliche Aufnahme der Audiodaten erfolgt über die
 * "Simple API" von PulseAudio (libpulse-simple).
 */
class PulseCaptureThread : public CaptureThread
{
    Q_OBJECT
public:
    /**
     * @brief Standard-Konstruktor.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit PulseCaptureThread (QObject *parent = nullptr);

protected:
    /**
     * @brief Initialisiert die PulseAudio-Aufnahmeumgebung.
     *
     * Findet die Standard-Geräte, lädt die `module-null-sink` und `module-loopback`
     * Kernel-Module via `pactl` und öffnet zwei `pa_simple` Streams zum Mitschneiden.
     * @return true bei Erfolg, andernfalls false.
     */
    bool initializeCapture () override;

    /**
     * @brief Führt eine einzelne Iteration der Aufnahmeschleife aus.
     *
     * Liest Audio-Daten vom System- und Mikrofon-Stream, mischt diese unter
     * Berücksichtigung der Gain-Faktoren und sendet das Ergebnis über das
     * pcmChunkReady-Signal.
     */
    void captureLoopIteration () override;

    /**
     * @brief Gibt alle für die Aufnahme erstellten PulseAudio-Ressourcen frei.
     *
     * Schließt die `pa_simple` Streams und entlädt die zuvor per `pactl`
     * geladenen Kernel-Module.
     */
    void cleanupCapture () override;

private:
    pa_simple *m_paSys; ///< Handle für den PulseAudio-Stream der System-Sounds.
    pa_simple *m_paMic; ///< Handle für den PulseAudio-Stream des Mikrofons.
    int m_modNull;      ///< ID des geladenen module-null-sink.
    int m_modLoop;      ///< ID des geladenen module-loopback.

    std::vector<float> bufSys, bufMic, bufMix; ///< Puffer für die Audio-Samples.
    float m_sysGain, m_micGain; ///< Verstärkungsfaktoren für System- und Mikrofon-Audio.
};

#endif // PULSECAPTURETHREAD_H

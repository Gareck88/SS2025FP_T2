/**
 * @file capturethread.h
 * @brief Enthält die Deklaration der abstrakten Basisklasse CaptureThread.
 * @author Mike Wild
 */
#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

/**
 * @brief Eine abstrakte Basisklasse für Threads, die Audio in Echtzeit aufnehmen.
 *
 * Diese Klasse implementiert die allgemeine Logik und den Lebenszyklus eines
 * Audio-Aufnahme-Threads mithilfe des Template-Method-Patterns. Die run()-Methode
 * definiert das Grundgerüst des Ablaufs, während plattformspezifische Details
 * (Initialisierung, die eigentliche Aufnahmeschleife und das Aufräumen)
 * von abgeleiteten Klassen implementiert werden müssen.
 */
class CaptureThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief Standard-Konstruktor.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit CaptureThread (QObject *parent = nullptr);

    /**
     * @brief Virtueller Destruktor.
     */
    virtual ~CaptureThread () = default;

    /**
     * @brief Startet eine neue Aufnahme-Session.
     * Diese Methode ist thread-sicher und weckt den run()-Loop auf, um mit der Aufnahme zu beginnen.
     */
    void startCapture ();

    /**
     * @brief Fordert das Beenden der aktuellen Aufnahme-Session an.
     * Dies ist ein nicht-blockierender Aufruf. Der Thread beendet die Aufnahmeschleife
     * so bald wie möglich.
     */
    virtual void stopCapture ();

    /**
     * @brief Beendet den Thread vollständig und wartet auf dessen Terminierung.
     * Dies ist ein blockierender Aufruf, der sicherstellt, dass alle Ressourcen freigegeben werden.
     */
    void shutdown ();

signals:
    /**
     * @brief Wird gesendet, wenn ein neuer Block von Audio-Daten (PCM-Samples) bereitsteht.
     * @param chunk Eine Liste von float-Werten, die die Audio-Samples repräsentieren.
     */
    void pcmChunkReady (QList<float> chunk);

    /**
     * @brief Wird gesendet, unmittelbar nachdem die plattformspezifische Initialisierung
     * erfolgreich war und die Aufnahmeschleife beginnt.
     */
    void started ();

    /**
     * @brief Wird gesendet, nachdem die Aufnahmeschleife beendet und die Aufräumarbeiten
     * abgeschlossen sind.
     */
    void stopped ();

protected:
    /**
     * @brief Die Hauptfunktion des Threads, die von QThread aufgerufen wird.
     * Sie implementiert die Zustandsmaschine für den Aufnahme-Lebenszyklus.
     */
    void run () override;

    /**
     * @brief Rein virtuelle Methode zur Initialisierung der plattformspezifischen Audio-Ressourcen.
     * Muss von abgeleiteten Klassen implementiert werden.
     * @return true bei Erfolg, andernfalls false.
     */
    virtual bool initializeCapture () = 0;

    /**
     * @brief Rein virtuelle Methode, die eine einzelne Iteration der Aufnahmeschleife durchführt.
     * Hier werden die Audiodaten vom Gerät gelesen und verarbeitet.
     * Muss von abgeleiteten Klassen implementiert werden.
     */
    virtual void captureLoopIteration () = 0;

    /**
     * @brief Rein virtuelle Methode zum Aufräumen und Freigeben der plattformspezifischen Ressourcen.
     * Wird aufgerufen, nachdem die Aufnahmeschleife beendet wurde.
     * Muss von abgeleiteten Klassen implementiert werden.
     */
    virtual void cleanupCapture () = 0;

    // Synchronisationsobjekte für die Steuerung des Threads von außen
    QMutex m_mutex;                 ///< Schützt den Zugriff auf den Zustand des Threads.
    QWaitCondition m_waitCondition; ///< Lässt den Thread schlafen, wenn er inaktiv ist.
    std::atomic<bool> m_active;     ///< Steuert die innere Aufnahmeschleife (start/stop).
    std::atomic<bool> m_shutdown;   ///< Signalisiert dem Thread, sich komplett zu beenden.
};

#endif // CAPTURETHREAD_H

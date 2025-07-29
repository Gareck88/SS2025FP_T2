#include "capturethread.h"

#include <QDebug>

CaptureThread::CaptureThread (
    QObject *parent)
    : QThread (parent)
    , m_active (false)
    , m_shutdown (false)
{
}

//--------------------------------------------------------------------------------------------------

void CaptureThread::startCapture ()
{
    // QMutexLocker sperrt den Mutex im Konstruktor und gibt ihn im Destruktor
    // automatisch frei. Das garantiert Thread-Sicherheit für diese Methode.
    QMutexLocker locker (&m_mutex);
    m_active.store (true);

    // Weckt den run()-Loop auf, falls dieser gerade in m_waitCondition.wait() schläft.
    m_waitCondition.wakeAll ();
}

//--------------------------------------------------------------------------------------------------

void CaptureThread::stopCapture ()
{
    // Setzt nur das Flag, um die innere Aufnahmeschleife zu beenden.
    // Dies ist eine nicht-blockierende Operation.
    m_active.store (false);
}

//--------------------------------------------------------------------------------------------------

void CaptureThread::shutdown ()
{
    // Setzt die Flags, um beide Schleifen in run() zu beenden.
    m_shutdown.store (true);
    m_active.store (false);

    // Weckt den Thread auf, falls er wartet, damit er die neuen Flag-Werte erkennt.
    m_waitCondition.wakeAll ();

    // Blockiert und wartet, bis die run()-Methode vollständig ausgeführt wurde
    // und der Thread sich beendet hat. Dies stellt ein sauberes Herunterfahren sicher.
    wait ();
}

//--------------------------------------------------------------------------------------------------

void CaptureThread::run ()
{
    // Die äußere Schleife hält den Thread am Leben, bis shutdown() aufgerufen wird.
    while (!m_shutdown.load ())
    {
        m_mutex.lock ();
        if (!m_active.load ())
        {
            // --- Zustand: Wartend ---
            // Wenn keine Aufnahme aktiv ist, wird der Thread schlafen gelegt.
            // wait() gibt den Mutex atomar frei und legt den Thread schlafen.
            // Nach dem Aufwachen wird der Mutex automatisch wieder gesperrt.
            m_waitCondition.wait (&m_mutex);
        }
        m_mutex.unlock ();

        // Wenn der Thread aufgeweckt wurde, um sich zu beenden, wird die Schleife hier verlassen.
        if (m_shutdown.load ())
        {
            break;
        }

        // --- Zustand: Initialisierung ---
        if (!initializeCapture ())
        {
            // Wenn die plattformspezifische Initialisierung fehlschlägt,
            // wird die Aufnahme abgebrochen und der Thread geht zurück in den Wartezustand.
            m_active.store (false);
            continue;
        }

        emit started ();

        // --- Zustand: Aktive Aufnahme ---
        // Die innere Schleife läuft, solange die Aufnahme aktiv ist.
        while (m_active.load ())
        {
            captureLoopIteration (); // Die eigentliche, plattformspezifische Arbeit.
        }

        // --- Zustand: Aufräumen ---
        // Die Aufnahme wurde durch stopCapture() beendet.
        cleanupCapture ();
        emit stopped ();
    }
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

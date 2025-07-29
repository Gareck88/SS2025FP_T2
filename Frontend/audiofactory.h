/**
 * @file audiofactory.h
 * @brief Enthält die Deklaration der AudioFactory-Klasse.
 * @author Mike Wild
 */
#ifndef AUDIOFACTORY_H
#define AUDIOFACTORY_H

// Forward-Deklarationen
class QObject;
class CaptureThread;

/**
 * @brief Eine Factory-Klasse zur Erstellung von plattformspezifischen CaptureThread-Objekten.
 *
 * Diese Klasse nutzt den Factory-Pattern, um die Logik zur Auswahl der korrekten
 * CaptureThread-Implementierung (z.B. PulseCaptureThread für Linux, WinCaptureThread für Windows)
 * von der restlichen Anwendung zu entkoppeln.
 */
class AudioFactory
{
public:
    /**
     * @brief Standard-Konstruktor.
     */
    AudioFactory () = default;

    /**
     * @brief Erstellt eine Instanz der korrekten, plattformspezifischen CaptureThread-Klasse.
     *
     * Diese Methode verwendet Präprozessor-Direktiven, um zur Kompilierzeit die passende
     * Thread-Implementierung für das Zielbetriebssystem auszuwählen.
     * @param parent Das QObject-Elternteil für den neuen Thread, um die Speicherverwaltung zu gewährleisten.
     * @return Ein Zeiger auf den neu erstellten CaptureThread oder nullptr, wenn die Plattform nicht unterstützt wird.
     */
    static CaptureThread *createThread (QObject *parent = nullptr);
};

#endif // AUDIOFACTORY_H

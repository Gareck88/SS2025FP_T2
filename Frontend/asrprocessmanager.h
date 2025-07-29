/**
 * @file asrprocessmanager.h
 * @brief Enthält die Deklaration der AsrProcessManager-Klasse.
 * @author Mike Wild
 */
#ifndef ASRPROCESSMANAGER_H
#define ASRPROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include "transcription.h"

/**
 * @brief Steuert den externen Python-Prozess für die Spracherkennung (ASR).
 *
 * Diese Klasse kapselt die gesamte Logik für das Starten des ASR-Skripts,
 * die Kommunikation über Standard-I/O und die Fehlerbehandlung. Sie arbeitet
 * vollständig asynchron und kommuniziert ihren Zustand über Signale mit dem
 * Rest der Anwendung (z.B. dem MainWindow).
 */
class AsrProcessManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor. Lädt die notwendigen Pfade aus den QSettings.
     * @param parent Das QObject-Elternteil für die automatische Speicherverwaltung.
     */
    explicit AsrProcessManager (QObject *parent = nullptr);

    /**
     * @brief Destruktor. Stellt sicher, dass ein laufender Prozess beendet wird.
     */
    ~AsrProcessManager ();

public slots:
    /**
     * @brief Startet den ASR-Prozess für die angegebene WAV-Datei.
     *
     * Stellt sicher, dass nicht bereits ein Prozess läuft und übergibt den
     * Dateipfad als Argument an das Python-Skript.
     * @param wavFilePath Der absolute Pfad zur 16-kHz-Mono-WAV-Datei, die verarbeitet werden soll.
     * @note Dies ist ein öffentlicher Slot, der z.B. von der MainWindow aufgerufen wird.
     */
    void startTranscription (const QString &wavFilePath);

    /**
     * @brief Stoppt den laufenden ASR-Prozess, falls einer aktiv ist.
     *
     * Nützlich, wenn eine neue Aufnahme gestartet wird, während eine alte
     * Transkription noch läuft.
     */
    void stop ();

signals:
    /**
     * @brief Wird für jedes erkannte und geparste Textsegment gesendet.
     *
     * Das Python-Skript gibt die Segmente zeilenweise aus; dieses Signal wird
     * für jede erfolgreich geparste Zeile emittiert.
     * @param segment Das vollständig geparste Segment mit Zeitstempeln, Sprecher und Text.
     */
    void segmentReady (const MetaText &segment);

    /**
     * @brief Wird gesendet, wenn der ASR-Prozess (erfolgreich oder nicht) abgeschlossen ist.
     * @param success true, wenn der Prozess ohne Fehler (Exit-Code 0) beendet wurde.
     * @param errorMsg Eine Fehlermeldung, falls der Prozess fehlschlug. Enthält
     * typischerweise den Standard-Error-Stream des Prozesses für Debugging.
     */
    void finished (bool success, const QString &errorMsg);

private slots:
    // Interne Slots zur Behandlung der Signale von QProcess
    /** @brief Interner Slot, der aufgerufen wird, wenn der Prozess Daten auf stdout ausgibt. */
    void handleProcessOutput ();

    /** @brief Interner Slot, der aufgerufen wird, wenn der Prozess sich beendet. */
    void handleProcessFinished (int exitCode, QProcess::ExitStatus exitStatus);

    /** @brief Interner Slot, der aufgerufen wird, wenn beim Starten des Prozesses ein Fehler auftritt. */
    void handleProcessError (QProcess::ProcessError error);

private:
    /**
     * @brief Parst eine einzelne Ausgabezeile des Python-Skripts in ein MetaText-Objekt.
     * @param line Die zu parsende Zeile im Format "[start]s --> [end]s] SPEAKER: Text".
     * @return Ein gefülltes MetaText-Objekt. Wenn das Parsen fehlschlägt, ist das Objekt leer.
     */
    MetaText parseLine (const QString &line);

    /**
     * @brief Lädt den Python- und den Skript-Pfad aus den globalen Einstellungen
     */
    void loadPaths ();

    QProcess *m_process;  ///< Zeiger auf das QProcess-Objekt, das das Python-Skript ausführt.
    QString m_pythonPath; ///< Pfad zum Python-Interpreter der virtuellen Umgebung.
    QString m_scriptPath; ///< Pfad zum ASR-Python-Skript.
    int m_unknownCounter; ///< Zähler für die Benennung von unbekannten Sprechern (UNKNOWN_0, UNKNOWN_1, ...).
};

#endif // ASRPROCESSMANAGER_H

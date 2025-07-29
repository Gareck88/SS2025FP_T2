/**
 * @file filemanager.h
 * @brief Enthält die Deklaration der FileManager-Klasse.
 * @author Mike Wild
 */
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QJsonDocument>
#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @brief Kapselt alle direkten Dateisystem-Interaktionen der Anwendung.
 *
 * Diese Klasse dient als zentrale Anlaufstelle ("Single Source of Truth") für alle
 * Pfad-Konstruktionen und Dateioperationen. Sie abstrahiert Details wie temporäre
 * Verzeichnisse oder die Namenskonvention von Meeting-Dateien vom Rest der Anwendung.
 * Sie erbt von QObject, um die automatische Speicherverwaltung durch Qt zu nutzen.
 */
class FileManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor, der das Meeting-Verzeichnis initialisiert.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit FileManager (QObject *parent = nullptr);

    /**
     * @brief Gibt den vollständigen Pfad für die temporäre WAV-Aufnahmedatei zurück.
     * @param forAsr Wenn true, wird der Pfad für die heruntergesampelte ASR-Version zurückgegeben.
     * @return Der vollständige Dateipfad.
     */
    QString getTempWavPath (bool forAsr = false) const;

    /*
     * @brief Gibt den Pfad zum Verzeichnis zurück, in dem alle Meeting-Transkripte gespeichert werden.
     * @note Stellt bei der ersten Ausführung sicher, dass das Verzeichnis existiert.
     * @todo Diese Methode ist nur ein temporärer Platzhalter für die Demonstration
     * und wird entfernt, sobald die Anwendung mit der Datenbank verknüpft wird.
     * @return Der vollständige Verzeichnispfad.
    QString getMeetingsDirectory () const;



     * @brief Erstellt den vollständigen Dateipfad für eine bestimmte Meeting-JSON-Datei.
     * @todo Diese Methode ist nur ein temporärer Platzhalter für die Demonstration
     * und wird entfernt, sobald die Anwendung mit der Datenbank verknüpft wird.
     * @param meetingId Die eindeutige ID des Meetings (z.B. "Aufnahme - 2025-06-17_09-30").
     * @param isEdited Wenn true, wird der Pfad zur "_bearbeitet.json"-Version zurückgegeben,
     * andernfalls zur "_original.json".
     * @return Der vollständige Dateipfad.

    QString getMeetingJsonPath (const QString &meetingId, bool isEdited) const;


     * @brief Durchsucht das Meeting-Verzeichnis und gibt eine Liste aller gefundenen Meeting-IDs zurück.
     * @todo Diese Methode ist nur ein temporärer Platzhalter für die Demonstration
     * und wird entfernt, sobald die Anwendung mit der Datenbank verknüpft wird.
     * @return Eine QStringList mit den Basisnamen der Meetings (ohne Suffix).
    QStringList findExistingMeetings () const;
     */

    /**
     * @brief Lädt eine JSON-Datei vom angegebenen Pfad.
     * @todo Diese Methode ist nur ein temporärer Platzhalter für die Demonstration
     * bis die Anwendung mit der Datenbank verknüpft wird. Danach kann sie aber evtl. erhalten bleiben.
     * @param filePath Der Pfad zur zu ladenden Datei.
     * @param ok [out] Wird auf true gesetzt, wenn das Laden und Parsen erfolgreich war, sonst false.
     * @return Das geladene QJsonDocument. Bei einem Fehler ist das Dokument leer.
     */
    QJsonDocument loadJson (const QString &filePath, bool &ok) const;

    /**
     * @brief Speichert ein QJsonDocument in der angegebenen Datei.
     * @todo Diese Methode ist nur ein temporärer Platzhalter für die Demonstration
     * bis die Anwendung mit der Datenbank verknüpft wird. Danach kann sie aber evtl. erhalten bleiben.
     * @param filePath Der Ziel-Dateipfad.
     * @param doc Das zu speichernde JSON-Dokument.
     * @return true bei Erfolg, andernfalls false.
     */
    bool saveJson (const QString &filePath, const QJsonDocument &doc) const;
};

#endif // FILEMANAGER_H

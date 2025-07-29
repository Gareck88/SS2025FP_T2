/**
 * @file databasemanager.h
 * @brief Enthält die Deklaration der Databasemanager-Klasse.
 * @author Yolanda Fiska
 */
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QStringList>
#include <QObject>
#include <QSqlDatabase>
#include "transcription.h"

/**
 * @brief The DatabaseManager class kapselt die Datenbankverbindung, das Laden von Daten aus der Datenbank
 * sowie das Speichern von Daten zu der Datenbank.
 */
class DatabaseManager:public QObject{
    Q_OBJECT
public:
    /**
     * @brief Standard-Konstruktor.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit DatabaseManager (QObject *parent = nullptr);

    /**
    * @brief Stellt eine Verbindung zur Supabase-Datenbank her.
    *
    * Diese Methode initialisiert die Supabase-Client-Komponente mit den
    * notwendigen Zugangsdaten aus einer Konfigurationsquelle. Sie überprüft,
    * ob die Verbindung erfolgreich hergestellt werden kann, bevor weitere Datenbankaktionen
    * ausgeführt werden.
    *
    * @return true, wenn die Verbindung erfolgreich hergestellt wurde, andernfalls false.
    */
    bool connectToSupabase();

    /**
    * @brief Gibt die aktive Datenbankverbindung zur Supabase zurück.
    * @return QSqlDatabase-Instanz mit dem Connection-Name "supabase".
    *
    * Diese Methode stellt sicher, dass überall dieselbe Datenbankinstanz
    * verwendet wird, um doppelte Verbindungen zu vermeiden.
    */
    static QSqlDatabase getDatabase();

    /**
    * @brief Liefert den Namen eines Sprechers anhand seiner ID und der Besprechungs-ID.
    * @param speakerId Die ID des Sprechers.
    * @param meetingId Die ID der zugehörigen Besprechung.
    * @param db Die zu verwendende Datenbankverbindung.
    * @return Der Name des Sprechers oder "Unbekannt", falls nicht gefunden.
    */
    QString getSpeakerName(int speakerId, int meetingId, const QSqlDatabase &db);


    /**
    * @brief Lädt alle Transkriptionen aus der Datenbank.
    * @return Map mit Besprechungstitel und zugehöriger Transkription.
    */
    QMap<QString, Transcription*> loadAllTranscriptions();

    /** @brief Lädt alle Transkriptionname und sie in einer Liste speichern. */
    QStringList loadAllTranscriptionsName();

    /**
    * @brief Gibt die ID einer Besprechung anhand des Titels zurück.
    * @param title Titel der Besprechung.
    * @return Meeting-ID.
    */
    static int getMeetingIdByTitle(const QString &title);

    /**
    * @brief Lädt das Transkript für ein Meeting.
    * @param meetingTitle Titel der Besprechung.
    * @param textColumn Spalte mit dem Text (z. B. "roher_text").
    * @param m_script Zeiger auf das Ziel-Transkriptionsobjekt.
    ‚*/
    void loadMeetingTranscriptions(const QString &meetingTitle,
                                   const QString &textColumn,
                                   Transcription *m_script);

    /**
     *  @brief Aktualisiert das Transkript in der Datenbank.
     *  @param m_script Zeigeer auf das das Ziel-Transkriptionsobjekt.
     *  @return true, wenn der Prozess erfolgreich abgeschlossen wird, ansonsten false.
     */
    bool updateTranscription(Transcription *m_script);

    /**
     * @brief Speichert das Neue Transkription in der Datenbank.
     * @param m_script das neue erstellte Transkription.
     * @param newTitle Meetingsname.
     * @return true, wenn der Prozess erfolgreich abgeschlossen wird, ansonsten false.
     */
    bool saveNewTranscription(Transcription *m_script, const QString &newTitle);

    /**
     * @brief Gibt den Status der letzten Datenbankverbindung zurück.
     *
     * Diese Methode liefert `true`, wenn die Verbindung zur Supabase-Datenbank
     * erfolgreich hergestellt wurde, andernfalls `false`.
     *
     * @return bool Verbindung erfolgreich oder nicht.
     */
    bool isConnected() const { return m_connected; };

    /**
     * @brief Hilfsfunktion zum Parsen von PostgreSQL text[] string in QStringList
     * @param pgArrayString Die Tags des Transkripts
     * @return Tags als QStringList
     */
    QStringList parsePgTextArray(const QString &pgArrayString);


private:
    bool m_connected = false; // Flag, um anzuzeigen, ob die Datenbankverbindung funktioniert
};

#endif // DATABASEMANAGER_H

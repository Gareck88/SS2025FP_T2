/**
 * @file databasemanager.h
 * @brief Enthält die Deklaration der Databasemanager-Klasse.
 * @author Yolanda Fiska
 */
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QStringList>

/**
 * @brief The DatabaseManager class kapselt die Datenbankverbindung, das Laden von Daten aus der Datenbank
 * sowie das Speichern von Daten zu der Datenbank.
 */
class DatabaseManager : public QObject
{
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
    bool connectToSupabase ();

    /**
    * @brief Lädt alle verfügbaren Transkriptionstitel aus der Supabase-Datenbank.
    *
    * Diese Methode stellt eine Verbindung zur Supabase her und ruft alle Einträge aus der
    * Tabelle "besprechungen" ab. Es wird eine Liste von Titeln zurückgegeben, die für die
    * Darstellung oder weitere Verarbeitung genutzt werden kann.
    *
    * @return Eine QStringList mit den Titeln der gespeicherten Besprechungen.
    */
    QStringList loadAllTranscriptions ();
};

#endif // DATABASEMANAGER_H

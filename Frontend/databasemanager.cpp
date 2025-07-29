#include "databasemanager.h"

#include <QDebug>
#include <QProcessEnvironment>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

DatabaseManager::DatabaseManager (
    QObject *parent)
{
}

bool DatabaseManager::connectToSupabase ()
{
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    settings.beginGroup ("Database");
    QSqlDatabase db = QSqlDatabase::addDatabase ("QPSQL", "supabase");
    db.setHostName (settings.value ("host").toString ());
    db.setPort (settings.value ("port").toInt ());
    db.setDatabaseName (settings.value ("name").toString ());
    db.setUserName (settings.value ("user").toString ());
    db.setPassword (settings.value ("password").toString ());
    settings.endGroup ();

    if (!db.open ())
    {
        qWarning () << "❌ Verbindung konnte nicht hergestellt werden:" << db.lastError ().text ();
        return false;
    }
    qDebug () << "✅ Mit Supabase PostgreSQL verbunden!";
    return true;
}

QStringList DatabaseManager::loadAllTranscriptions ()
{
    QStringList titles;

    QSqlDatabase db = QSqlDatabase::database ("supabase");
    QSqlQuery query (db);

    if (!query.exec ("SELECT titel FROM besprechungen"))
    {
        qWarning () << "❌ Abfrage fehlgeschlagen:" << query.lastError ().text ();
        return titles;
    }

    while (query.next ())
    {
        titles << query.value (0).toString ();
    }

    return titles;
}

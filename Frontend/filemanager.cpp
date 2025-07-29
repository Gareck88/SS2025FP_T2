#include "filemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonParseError>
#include <QSettings>

FileManager::FileManager (
    QObject *parent)
    : QObject (parent)
{
}

//--------------------------------------------------------------------------------------------------

QString FileManager::getTempWavPath (
    bool forAsr) const
{
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    QString stdWav = QDir::temp ().filePath ("meeting_recording.wav");
    QString stdAsr = QDir::temp ().filePath ("meeting_recording_asr.wav");

    QString ergWav = settings.value ("wavPath", stdWav).toString ();
    QString ergAsr = settings.value ("asrWavPath", stdAsr).toString ();

    //  Je nach Anwendungsfall wird der Pfad zur hochauflösenden (HQ)
    //  oder zur für die Spracherkennung (ASR) optimierten Datei zurückgegeben.
    if (forAsr)
    {
        return ergAsr;
    }
    return ergWav;
}

//--------------------------------------------------------------------------------------------------
/*
QString FileManager::getMeetingsDirectory () const
{
    //  Der Speicherort für die gespeicherten Meetings ist betriebssystemabhängig.
#ifdef Q_OS_WIN
    QString stdpath = QDir::tempPath () + "/meetings";
#else
    //  Unter Linux/macOS wird einheitlich /tmp/meetings verwendet.
    QString stdpath = "/tmp/meetings";
#endif

    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    QString path = settings.value ("meetingPath", stdpath).toString ();

    QDir dir (path);
    if (!dir.exists ())
    {
        //  Erstellt das Verzeichnis rekursiv, falls es noch nicht vorhanden ist.
        dir.mkpath (".");
    }
    return path;
}
//--------------------------------------------------------------------------------------------------

QString FileManager::getMeetingJsonPath (
    const QString &meetingId, bool isEdited) const
{
    QString suffix = isEdited ? "_bearbeitet.json" : "_original.json";
    return getMeetingsDirectory () + "/" + meetingId + suffix;
}

//--------------------------------------------------------------------------------------------------

QStringList FileManager::findExistingMeetings () const
{
    QDir dir (getMeetingsDirectory ());
    QStringList files = dir.entryList (QStringList () << "*_original.json", QDir::Files);

    QStringList meetingIds;
    for (const QString &file : files)
    {
        QString baseName = QFileInfo (file).fileName ();
        //  Der Suffix "_original.json" wird vom Dateinamen entfernt,
        //  um die reine Meeting-ID für die Anzeige in der Liste zu extrahieren.
        baseName.remove ("_original.json");
        meetingIds.append (baseName);
    }
    return meetingIds;
}
*/

//--------------------------------------------------------------------------------------------------

QJsonDocument FileManager::loadJson (
    const QString &filePath, bool &ok) const
{
    //  'ok' ist ein out-Parameter, der dem Aufrufer den Erfolg der Operation signalisiert.
    ok = false;
    QFile file (filePath);
    if (!file.open (QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning () << "FileManager: Konnte Datei nicht öffnen:" << filePath;
        return QJsonDocument ();
    }

    QByteArray data = file.readAll ();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson (data, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning () << "FileManager: JSON-Parse-Fehler in Datei" << filePath << ":"
                    << parseError.errorString ();
        return QJsonDocument ();
    }

    ok = true;
    return doc;
}

//--------------------------------------------------------------------------------------------------

bool FileManager::saveJson (
    const QString &filePath, const QJsonDocument &doc) const
{
    QFile file (filePath);
    if (!file.open (QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning () << "FileManager: Konnte Datei zum Schreiben nicht öffnen:" << filePath
                    << file.errorString ();
        return false;
    }

    //  Wir speichern das JSON lesbar (Indented), um das manuelle Debuggen der Dateien zu erleichtern.
    file.write (doc.toJson (QJsonDocument::Indented));
    return true;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

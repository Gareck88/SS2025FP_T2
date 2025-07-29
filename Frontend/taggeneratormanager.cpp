#include "taggeneratormanager.h"

#include <QCoreApplication>
#include <QSettings>

//--------------------------------------------------------------------------------------------------

TagGeneratorManager::TagGeneratorManager (
    QObject *parent)
    : QObject (parent)
    , m_process (new QProcess (this))
{
    QSettings settings;
    m_pythonPath = settings.value ("pythonPath").toString ();

    //  Der Pfad zum Tag-Generator-Skript wird relativ zum Anwendungsverzeichnis konstruiert.
    //  Dies stellt sicher, dass es auch nach der Installation noch gefunden wird.
    m_scriptPath = QCoreApplication::applicationDirPath () + "/python/generate_tags.py";

    connect (m_process, &QProcess::finished, this, &TagGeneratorManager::onProcessFinished);
}

//--------------------------------------------------------------------------------------------------

void TagGeneratorManager::generateTagsFor (
    const QString &fullText)
{
    //  Sicherheitsprüfung, um zu verhindern, dass mehrere Analyse-Prozesse gleichzeitig laufen.
    if (m_process->state () != QProcess::NotRunning)
    {
        emit tagsReady ({}, false, "Ein anderer Prozess zur Tag-Generierung läuft bereits.");
        return;
    }

    //  Startet das Python-Skript.
    m_process->start (m_pythonPath, {m_scriptPath});

    //  Schreibt den gesamten Transkript-Text in die Standardeingabe des Python-Prozesses.
    m_process->write (fullText.toUtf8 ());

    //  WICHTIG: Schließt den Schreibkanal. Dies sendet ein "End-of-File" (EOF) an das
    //  Python-Skript. Dessen `sys.stdin.read()`-Befehl wird erst dann beendet
    //  und mit der Verarbeitung beginnen, wenn dieser Kanal geschlossen ist.
    m_process->closeWriteChannel ();
}

//--------------------------------------------------------------------------------------------------

void TagGeneratorManager::onProcessFinished (
    int exitCode, QProcess::ExitStatus exitStatus)
{
    //  Prüft, ob der Prozess normal beendet wurde und einen Erfolgs-Code (0) zurückgegeben hat.
    if (exitStatus != QProcess::NormalExit || exitCode != 0)
    {
        //  Im Fehlerfall wird der Standard-Fehlerkanal ausgelesen, um eine aussagekräftige
        //  Fehlermeldung an die UI weiterzugeben.
        QString error = QString::fromUtf8 (m_process->readAllStandardError ());
        emit tagsReady ({},
                        false,
                        "Fehler im Python-Skript zur Tag-Erstellung: " + error.trimmed ());
        return;
    }

    //  Liest die gesamte Standardausgabe des erfolgreichen Prozesses.
    QString output = QString::fromUtf8 (m_process->readAllStandardOutput ());

    //  Das Python-Skript gibt jedes Tag in einer neuen Zeile aus.
    //  Wir zerlegen die Ausgabe anhand der Zeilenumbrüche in eine String-Liste.
    QStringList tags = output.split ('\n', Qt::SkipEmptyParts);

    emit tagsReady (tags, true);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

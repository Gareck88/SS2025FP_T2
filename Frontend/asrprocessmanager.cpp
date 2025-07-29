#include "asrprocessmanager.h"

#include <QRegularExpression>
#include <QSettings>

AsrProcessManager::AsrProcessManager (
    QObject *parent)
    : QObject (parent)
    , m_process (new QProcess (this))
    , m_unknownCounter (0)
{
    //  Die Signale des internen QProcess-Objekts mit den Slots dieser Klasse verbinden.
    connect (m_process,
             &QProcess::readyReadStandardOutput,
             this,
             &AsrProcessManager::handleProcessOutput);
    connect (m_process, &QProcess::finished, this, &AsrProcessManager::handleProcessFinished);
    connect (m_process, &QProcess::errorOccurred, this, &AsrProcessManager::handleProcessError);
}

//--------------------------------------------------------------------------------------------------

AsrProcessManager::~AsrProcessManager ()
{
    //  Stellt sicher, dass der Kind-Prozess beendet wird, wenn der Manager zerstört wird.
    if (m_process->state () != QProcess::NotRunning)
    {
        m_process->terminate ();
        m_process->waitForFinished (
            1000); // Kurzes Warten, um dem Prozess Zeit zum Beenden zu geben.
    }
}

//--------------------------------------------------------------------------------------------------

void AsrProcessManager::startTranscription (
    const QString &wavFilePath)
{
    //  hier immer neu laden, falls Nutzer die Einstellung ändert.
    loadPaths ();

    if (m_pythonPath.isEmpty () || m_scriptPath.isEmpty ())
    {
        emit finished (false, "Python- oder Skript-Pfad ist nicht konfiguriert.");
        return;
    }

    if (m_process->state () != QProcess::NotRunning)
    {
        emit finished (false, "Ein anderer Transkriptionsprozess läuft bereits.");
        return;
    }

    //  Setze den Zähler für jeden neuen Lauf zurück, um wieder bei UNKNOWN_0 zu beginnen.
    m_unknownCounter = 0;

    QStringList args = {m_scriptPath, wavFilePath};
    m_process->start (m_pythonPath, args);
}

//--------------------------------------------------------------------------------------------------

void AsrProcessManager::stop ()
{
    if (m_process->state () != QProcess::NotRunning)
    {
        m_process->terminate ();
    }
}

//--------------------------------------------------------------------------------------------------

void AsrProcessManager::handleProcessOutput ()
{
    //  Solange der Prozess lesbare Zeilen hat, verarbeiten wir sie.
    while (m_process->canReadLine ())
    {
        QByteArray lineBytes = m_process->readLine ().trimmed ();
        if (!lineBytes.isEmpty ())
        {
            QString line = QString::fromUtf8 (lineBytes);
            MetaText segment = parseLine (line);

            //  Nur erfolgreich geparste Segmente werden als gültig betrachtet und weitergeleitet.
            if (!segment.Speaker.isEmpty ())
            {
                emit segmentReady (segment);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------

void AsrProcessManager::handleProcessFinished (
    int exitCode, QProcess::ExitStatus exitStatus)
{
    //  exitStatus prüft, ob der Prozess normal beendet oder abgestürzt ist.
    //  exitCode prüft den von Python zurückgegebenen Fehlercode (Konvention: 0 = Erfolg).
    if (exitStatus == QProcess::NormalExit && exitCode == 0)
    {
        emit finished (true, "");
    }
    else
    {
        // Bei einem Fehler wird der Standard-Error-Stream ausgelesen, um eine
        // nützliche Debug-Information an die UI weiterzugeben.
        QString errorDetails = QString::fromUtf8 (m_process->readAllStandardError ());
        QString errorMsg = QString ("Prozess fehlgeschlagen mit Exit-Code %1.\nDetails: %2")
                               .arg (exitCode)
                               .arg (errorDetails.trimmed ());
        emit finished (false, errorMsg);
    }
}

//--------------------------------------------------------------------------------------------------

void AsrProcessManager::handleProcessError (
    QProcess::ProcessError error)
{
    //  Dieser Slot wird für Fehler beim Starten des Prozesses aufgerufen (z.B. "Programm nicht gefunden").
    QString errorMsg = QString ("Ein Fehler ist beim Starten des Prozesses aufgetreten: %1")
                           .arg (m_process->errorString ());
    emit finished (false, errorMsg);
}

//--------------------------------------------------------------------------------------------------

MetaText AsrProcessManager::parseLine (
    const QString &line)
{
    //  Dieser reguläre Ausdruck zerlegt die Ausgabezeile des Python-Skripts.
    //  Beispiel: "[0.02s --> 1.55s] SPEAKER_00: Hallo Welt"
    //  - Capture-Gruppe 1 (\d+\.\d+): Startzeit (z.B. "0.02")
    //  - Capture-Gruppe 2 (\d+\.\d+): Endzeit (z.B. "1.55")
    //  - Capture-Gruppe 3 ([A-Z0-9_]+): Sprecher-ID (z.B. "SPEAKER_00")
    //  - Capture-Gruppe 4 (.*): Der eigentliche Text (z.B. "Hallo Welt")
    QRegularExpression re (R"(\[(\d+\.\d+)s\s*-->\s*(\d+\.\d+)s\]\s*([A-Z0-9_]+):\s*(.*))");
    QRegularExpressionMatch match = re.match (line);

    MetaText result;
    if (match.hasMatch ())
    {
        result.Start = match.captured (1);
        result.End = match.captured (2);
        result.Speaker = match.captured (3);
        result.Text = match.captured (4);

        //  Alle unbekannten Sprecher sollen einen eindeutigen Namen erhalten.
        if (result.Speaker == "UNKNOWN")
        {
            result.Speaker = QString ("UNKNOWN_%1").arg (m_unknownCounter++);
        }
    }
    else
    {
        qWarning () << "AsrProcessManager: Konnte Zeile nicht parsen:" << line;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------

void AsrProcessManager::loadPaths ()
{
    //  Lade die Pfad-Konfiguration aus den globalen Anwendungseinstellungen.
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    m_pythonPath = settings.value ("pythonPath").toString ();
    m_scriptPath = settings.value ("scriptPath").toString ();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

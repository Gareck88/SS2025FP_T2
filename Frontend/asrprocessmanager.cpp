// asrprocessmanager.cpp
#include "asrprocessmanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <QSettings>
#include <QFile>
#include <QProcessEnvironment> // Für Umgebungsvariablen
#include <QCoreApplication>

AsrProcessManager::AsrProcessManager (QObject *parent)
    : QObject (parent)
    , m_asrProcess (new QProcess (this))
    , m_stdoutStream (nullptr) // Wichtig: nullptr initialisieren
{
    startProcess();

    // Verbinden der Slots
    connect (m_asrProcess, &QProcess::readyReadStandardOutput,
            this, &AsrProcessManager::readyReadStandardOutput);
    connect (m_asrProcess, &QProcess::readyReadStandardError,
            this, &AsrProcessManager::readyReadStandardError);
    connect (m_asrProcess, QOverload<int, QProcess::ExitStatus>::of (&QProcess::finished),
            this, &AsrProcessManager::processFinished);
    connect (m_asrProcess, &QProcess::errorOccurred,
            this, &AsrProcessManager::processErrorOccurred);

    // Initialisieren des QTextStream für die Standardausgabe
    m_stdoutStream = new QTextStream(m_asrProcess);

    qDebug() << "ASR-Backend-Prozess gestartet mit PID:" << m_asrProcess->processId();
}

AsrProcessManager::~AsrProcessManager ()
{
    stopProcess(); // Sicherstellen, dass der Prozess beendet wird
    delete m_stdoutStream; // QTextStream löschen
}

// Slot zum Senden von Audiodaten an das Python-Backend (via Standardeingabe)
void AsrProcessManager::sendAudioChunk (const QByteArray &chunk)
{

    if (m_asrProcess->state () == QProcess::Running) {
        //qDebug() << "ASR-Process-Manager: Chunk größe: " << chunk.size();
        m_asrProcess->write (chunk);
    } else {
        qWarning () << "Kann Audio-Chunk nicht senden: ASR-Backend-Prozess läuft nicht.";
    }
}

bool AsrProcessManager::startProcess()
{

    // Überprüfen, ob der Prozess bereits läuft
    if (m_asrProcess->state () != QProcess::NotRunning) {
        qDebug () << "ASR-Backend-Prozess läuft bereits.";
        return true; // Oder false, je nach gewünschtem Verhalten
    }


    QSettings settings;
    QString pythonExecutable = settings.value ("pythonPath").toString (); // Oder der volle Pfad zu Ihrer venv-Python-Exe
    QString asrScriptPath = QCoreApplication::applicationDirPath () + "/python/asr_backend.py"; // Pfad zu Ihrem asr_backend.py

    // Prüfen, ob das Python-Skript existiert
    if (!QFile::exists(asrScriptPath)) {
        qWarning() << "Fehler: ASR-Backend-Skript nicht gefunden unter:" << asrScriptPath;
        emit backendError("ASR-Backend-Skript nicht gefunden.");
        return false;
    }

    // Argumente für den Python-Prozess: Das Skript und eventuelle Parameter
    QStringList arguments;
    arguments << asrScriptPath;
    // HINWEIS: `--input-pipe` und `--output-pipe` werden NICHT mehr benötigt,
    // da wir auf stdin/stdout umstellen.
    // Wenn das Python-Skript weitere Initialisierungsargumente benötigt, fügen Sie diese hier hinzu.

    // Umgebungsvariablen setzen (z.B. für Hugging Face Caches)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
    // Beispiel: env.insert("HF_HOME", "/path/to/huggingface_cache");
    // Diese sollten idealerweise bereits im System oder durch PythonEnvironmentManager gesetzt sein.
    m_asrProcess->setProcessEnvironment (env);

    // Wichtig: Trennen von Standardausgabe und Standardfehlerausgabe
    // Standardeingabe (für Audiodaten) und Standardausgabe (für JSON-Nachrichten)
    // werden direkt über die QProcess-Methoden read/write verwendet.
    m_asrProcess->setProcessChannelMode (QProcess::SeparateChannels);

    // Den Prozess starten
    qDebug () << "Starte ASR-Backend-Prozess:" << pythonExecutable << arguments.join (" ");
    m_asrProcess->start (pythonExecutable, arguments);

    // Initialisieren des QTextStream für die Standardausgabe, NACHDEM der Prozess gestartet ist
    // und somit die stdout-Pipe verfügbar ist.
    if (m_asrProcess->waitForStarted ()) {
        m_stdoutStream = new QTextStream (m_asrProcess);
        qDebug () << "ASR-Backend-Prozess gestartet mit PID:" << m_asrProcess->processId ();
        return true;
    } else {
        qWarning () << "Fehler beim Starten des ASR-Backend-Prozesses:"
                   << m_asrProcess->errorString ();
        emit backendError ("ASR-Backend konnte nicht gestartet werden: " + m_asrProcess->errorString ());
        return false;
    }

}

void AsrProcessManager::stopProcess()
{
    if (m_asrProcess && m_asrProcess->state () == QProcess::Running)
    {
        m_asrProcess->terminate (); // Versuche, den Prozess zu beenden
        if (!m_asrProcess->waitForFinished (3000)) // Warte max. 3 Sekunden
        {
            m_asrProcess->kill (); // Wenn terminate nicht reicht, zwingend beenden
            qWarning() << "ASR-Backend-Prozess musste getötet werden.";
        }
    }
}


// Liest die Standardausgabe des Python-Prozesses (erwartet JSON-Nachrichten)
void AsrProcessManager::readyReadStandardOutput ()
{
    // Sicherstellen, dass der Stream initialisiert ist
    if (!m_stdoutStream) {
        qWarning() << "m_stdoutStream ist nullptr in readyReadStandardOutput!";
        return;
    }

    // Zeilenweise lesen, um JSON-Objekte zu verarbeiten
    while (!m_stdoutStream->atEnd ()) {
        QString line = m_stdoutStream->readLine ();
        // qDebug() << "ASR-Backend (stdout):" << line; // Debugging-Ausgabe

        QJsonDocument doc = QJsonDocument::fromJson (line.toUtf8 ());
        if (doc.isObject ()) {
            QJsonObject obj = doc.object ();
            QString type = obj["type"].toString ();

            if (type == "ready") {
                qDebug() << "ASR-Backend meldet: READY!";
                emit backendReady ();
            } else if (type == "transcription") {
                qDebug() << "transcription" + obj["speaker"].toString () + "  " + obj["text"].toString ();
                QString speaker = obj["speaker"].toString ();
                QString text = obj["text"].toString ();

                emit transcriptionReady (speaker, text);
            } else if (type == "error") {
                QString message = obj["message"].toString ();
                emit backendError ("ASR-Backend-Fehler: " + message);
            } else {
                qWarning () << "Unbekannte Nachricht vom ASR-Backend:" << line;
            }
        } else {
            // Dies könnten auch einfache Debug-Ausgaben sein, die nicht JSON sind
            qDebug () << "ASR-Backend (stdout - Non-JSON):" << line;
        }
    }
}

void AsrProcessManager::readyReadStandardError ()
{
    QByteArray errorOutput = m_asrProcess->readAllStandardError ();
    qWarning () << "ASR-Backend (Stderr):" << errorOutput.constData ();
    //emit backendError("ASR-Backend-Fehler (stderr): " + QString::fromUtf8(errorOutput));
}

void AsrProcessManager::processFinished (int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug () << "ASR-Backend-Prozess beendet mit Code:" << exitCode
             << "Status:" << exitStatus;
    if (exitCode != 0) {
        emit backendError(QString("ASR-Backend-Prozess beendet mit Fehlercode: %1").arg(exitCode));
    } else {
        emit backendError("ASR-Backend-Prozess normal beendet."); // Oder ein anderes Signal für normalen Exit
    }
}

void AsrProcessManager::processErrorOccurred(QProcess::ProcessError error)
{
    qWarning() << "ASR-Backend-Prozessfehler aufgetreten:" << error;
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = "Prozess konnte nicht gestartet werden. Prüfen Sie, ob Python und das Skript existieren und ausführbar sind.";
        break;
    case QProcess::Crashed:
        errorMessage = "Prozess ist abgestürzt.";
        break;
    case QProcess::Timedout:
        errorMessage = "Prozess-Timeout.";
        break;
    case QProcess::ReadError:
        errorMessage = "Fehler beim Lesen der Prozessausgabe.";
        break;
    case QProcess::WriteError:
        errorMessage = "Fehler beim Schreiben an den Prozess.";
        break;
    case QProcess::UnknownError:
    default:
        errorMessage = "Unbekannter Prozessfehler.";
        break;
    }
    emit backendError("ASR-Backend-Prozessfehler: " + errorMessage);
}

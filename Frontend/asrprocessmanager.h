// asrprocessmanager.h
//@author Fabian Scherer
#ifndef ASRPROCESSMANAGER_H
#define ASRPROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTextStream>

class AsrProcessManager : public QObject
{
    Q_OBJECT
public:
    // NEU: Konstruktor akzeptiert jetzt die Pfade
    explicit AsrProcessManager (QObject *parent = nullptr);
    ~AsrProcessManager ();

    // Slot zum Senden von Audiodaten an das Python-Backend
    void sendAudioChunk (const QByteArray &chunk);
    // NEU: Methode zum Starten des Prozesses, nachdem die Pfade gesetzt sind.
    // Dies ist sinnvoll, wenn der Manager erst instanziiert und dann konfiguriert wird.
    bool startProcess();

    // NEU: Methode zum Stoppen des Python-Prozesses
    void stopProcess();

signals:
    // ... (bestehende Signale) ...
    void transcriptionReady (const QString &speaker, const QString &text);
    void backendReady ();
    void backendError (const QString &message);
    // NEU: Signal, das gesendet wird, wenn der Prozess erfolgreich gestartet wurde
    void processStarted();

private slots:
    void readyReadStandardOutput ();
    void readyReadStandardError ();
    void processFinished (int exitCode, QProcess::ExitStatus exitStatus);
    void processErrorOccurred(QProcess::ProcessError error);


private:
    QProcess *m_asrProcess;
    QTextStream *m_stdoutStream;

    // NEU: Private Member, um die Pfade zu speichern
    QString m_pythonPath;
    QString m_asrScriptPath;
};

#endif // ASRPROCESSMANAGER_H

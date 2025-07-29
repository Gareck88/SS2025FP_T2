#include "installationdialog.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

InstallationDialog::InstallationDialog (
    QWidget *parent)
    : QDialog (parent)
    , m_outputDisplay (new QTextEdit (this))
    , m_closeButton (new QPushButton (tr ("Abbrechen"), this))
    , m_setupProcess (new QProcess (this))
{
    setWindowTitle (tr ("Python Environment Setup"));
    setFixedSize (1200, 800);

    m_outputDisplay->setReadOnly (true);

    QVBoxLayout *mainLayout = new QVBoxLayout (this);
    mainLayout->addWidget (m_outputDisplay);
    mainLayout->addWidget (m_closeButton);
    setLayout (mainLayout);

    //  Verbindet die Signale des QProcess-Objekts mit den Slots dieses Dialogs.
    connect (m_setupProcess,
             &QProcess::readyReadStandardOutput,
             this,
             &InstallationDialog::appendOutput);
    connect (m_setupProcess,
             &QProcess::readyReadStandardError,
             this,
             &InstallationDialog::appendOutput);
    connect (m_setupProcess,
             qOverload<int, QProcess::ExitStatus> (&QProcess::finished),
             this,
             &InstallationDialog::handleProcessFinished);
    connect (m_setupProcess,
             &QProcess::errorOccurred,
             this,
             &InstallationDialog::handleProcessError);
    connect (m_closeButton,
             &QPushButton::clicked,
             this,
             &InstallationDialog::handleCancelButtonClicked);
}

//--------------------------------------------------------------------------------------------------

InstallationDialog::~InstallationDialog ()
{
    //  Stellt sicher, dass der Kind-Prozess beendet wird, falls der Dialog zerstört wird,
    //  während der Prozess noch läuft.
    if (m_setupProcess->state () == QProcess::Running)
    {
        m_setupProcess->terminate ();
        m_setupProcess->waitForFinished (1000);
        if (m_setupProcess->state () == QProcess::Running)
        {
            m_setupProcess->kill (); //  Erzwingt das Beenden als letzte Maßnahme.
        }
    }
}

//--------------------------------------------------------------------------------------------------

void InstallationDialog::startPythonSetup ()
{
    m_outputDisplay->clear ();
    m_closeButton->setText (tr ("Abbrechen")); //  Der Button-Text ändert sich je nach Zustand.

    QString scriptPath;
    QString program;
    QStringList args;

//  Wählt das korrekte Skript und den Interpreter für das jeweilige Betriebssystem.
#if defined(Q_OS_WIN)
    program = "cmd.exe";
    scriptPath = QCoreApplication::applicationDirPath () + "/python/setup_env.bat";
    args << "/c" << scriptPath;
#else
    program = "bash";
    scriptPath = QCoreApplication::applicationDirPath () + "/python/setup_env.sh";
    args << scriptPath;
#endif

    m_outputDisplay->append (tr ("Starte Setup-Skript: %1 %2").arg (program).arg (args.join (" ")));
    m_setupProcess->start (program, args);

    if (!m_setupProcess->waitForStarted (1000))
    {
        handleProcessError (m_setupProcess->error ());
    }
}

//--------------------------------------------------------------------------------------------------

void InstallationDialog::appendOutput ()
{
    //  Hängt sowohl Standard- als auch Fehlerausgaben an das Textfeld an,
    //  damit der Benutzer den gesamten Log sieht.
    m_outputDisplay->append (QString::fromLocal8Bit (m_setupProcess->readAllStandardOutput ()));
    m_outputDisplay->append (QString::fromLocal8Bit (m_setupProcess->readAllStandardError ()));
}

//--------------------------------------------------------------------------------------------------

void InstallationDialog::handleProcessFinished (
    int exitCode, QProcess::ExitStatus exitStatus)
{
    appendOutput (); //  Stellt sicher, dass auch die letzten Ausgaben erfasst werden.
    m_closeButton->setText (
        tr ("Schließen")); //  Der Prozess ist beendet, der Button schließt nur noch.

    if (exitStatus == QProcess::NormalExit && exitCode == 0)
    {
        m_outputDisplay->append (tr ("\n<b>Setup erfolgreich abgeschlossen.</b>"));
        emit installationFinished (true);
        //  accept() schließt den Dialog und sorgt dafür, dass exec() einen positiven Wert zurückgibt.
        accept ();
    }
    else
    {
        QString errorMsg = tr ("Setup fehlgeschlagen mit Exit-Code %1.").arg (exitCode);
        if (exitStatus == QProcess::CrashExit)
        {
            errorMsg = tr ("Setup-Prozess ist abgestürzt.");
        }
        m_outputDisplay->append (QString ("\n<b><font color='red'>%1</font></b>").arg (errorMsg));
        emit installationFinished (false, errorMsg);
    }
}

//--------------------------------------------------------------------------------------------------

void InstallationDialog::handleProcessError (
    QProcess::ProcessError error)
{
    m_closeButton->setText (tr ("Schließen"));
    QString errorMsg = tr ("Ein Fehler ist beim Ausführen des Prozesses aufgetreten: %1")
                           .arg (m_setupProcess->errorString ());
    m_outputDisplay->append (QString ("\n<b><font color='red'>%1</font></b>").arg (errorMsg));
    emit installationFinished (false, errorMsg);
}

//--------------------------------------------------------------------------------------------------

void InstallationDialog::handleCancelButtonClicked ()
{
    if (m_setupProcess->state () == QProcess::Running)
    {
        m_outputDisplay->append (tr ("\n<b>Breche den Setup-Prozess ab...</b>"));
        m_setupProcess->terminate ();
        if (!m_setupProcess->waitForFinished (5000))
        {
            m_outputDisplay->append (
                tr ("Prozess wurde nicht ordnungsgemäß beendet, wird gekillt..."));
            m_setupProcess->kill ();
        }
    }
    //  reject() schließt den Dialog und sorgt dafür, dass exec() einen negativen Wert zurückgibt.
    reject ();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

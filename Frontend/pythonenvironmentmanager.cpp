#include "pythonenvironmentmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#include "installationdialog.h"

PythonEnvironmentManager::PythonEnvironmentManager (
    QObject *parent)
    : QObject{parent}
    , m_dialogSuccess (false)
{
}

//--------------------------------------------------------------------------------------------------

bool PythonEnvironmentManager::checkAndSetup (
    bool forceReinstall, QWidget *parentWidget)
{
    QSettings settings (QSettings::NativeFormat, QSettings::UserScope,
        "SS2025FP_T2", "AudioTranskriptor");
    QString pythonPath = settings.value ("pythonPath").toString();
    settings.sync();

    //  Wenn eine Neuinstallation nicht erzwungen wird und der Pfad gültig ist, sind wir fertig.
    if (!forceReinstall && QFile::exists (pythonPath))
    {
        return true;
    }

    //  Der Pfad zur virtuellen Umgebung wird relativ zum Anwendungsverzeichnis konstruiert.
    QString venvPath = QCoreApplication::applicationDirPath () + "/python/venv";

    //  Wenn eine Neuinstallation erzwungen wird, wird die alte Umgebung zuerst gelöscht.
    if (forceReinstall && QDir (venvPath).exists ())
    {
        if (!removeVirtualEnvironment (venvPath))
        {
            //  Wenn das Löschen fehlschlägt, wird der Vorgang abgebrochen.
            return false;
        }
    }

    //  Ein modaler Dialog wird erstellt und angezeigt, um den Fortschritt darzustellen.
    InstallationDialog installationDialog (parentWidget);
    connect (&installationDialog,
             &InstallationDialog::installationFinished,
             this,
             &PythonEnvironmentManager::handleInstallationDialogFinished);

    //  Startet den Setup-Prozess, sobald der Dialog angezeigt wird.
    QTimer::singleShot (0, &installationDialog, &InstallationDialog::startPythonSetup);

    //  Die `exec()`-Methode blockiert, bis der Dialog geschlossen wird.
    installationDialog.exec ();

    //  Das Ergebnis (m_dialogSuccess) wird im Slot `handleInstallationDialogFinished` gesetzt.
    return m_dialogSuccess;
}

//--------------------------------------------------------------------------------------------------

void PythonEnvironmentManager::handleInstallationDialogFinished (
    bool success, const QString &errorMessage)
{
    //  Dieser Slot puffert das Ergebnis aus dem Dialog, damit es in `checkAndSetup` ausgewertet werden kann.
    m_dialogSuccess = success;
    m_dialogErrorMessage = errorMessage;

    if (success)
    {
        //  Wenn das Skript erfolgreich war, lese den Python-Pfad aus der generierten Textdatei.
        QSettings settings;
        QFile pythonPathFile (QCoreApplication::applicationDirPath () + "/python/python_path.txt");
        if (pythonPathFile.open (QIODevice::ReadOnly | QIODevice::Text))
        {
            QString pathFromScript = QString::fromUtf8 (pythonPathFile.readLine ()).trimmed ();
            pythonPathFile.close ();
            if (QFile::exists (pathFromScript))
            {
                settings.setValue ("pythonPath", pathFromScript);
            }
            else
            {
                //  Sollte dieser Fall eintreten, ist etwas im Setup-Skript schiefgelaufen.
                m_dialogSuccess = false;
                m_dialogErrorMessage = tr ("Konnte den Python-Pfad nach dem Setup nicht finden.");
            }
        }
        else
        {
            m_dialogSuccess = false;
            m_dialogErrorMessage = tr ("Konnte python_path.txt nicht öffnen.");
        }
    }
}

//--------------------------------------------------------------------------------------------------

bool PythonEnvironmentManager::removeVirtualEnvironment (
    const QString &venvPath)
{
    QDir venvDir (venvPath);
    if (venvDir.exists ())
    {
        //  Informiert den Benutzer, bevor das Verzeichnis gelöscht wird.
        QMessageBox::information (
            nullptr,
            QCoreApplication::translate ("main", "Virtuelle Umgebung wird gelöscht"),
            QCoreApplication::translate ("main", "Die alte Python-Umgebung wird entfernt..."));

        if (!venvDir.removeRecursively ())
        {
            //  Gibt eine kritische Fehlermeldung aus, wenn das Löschen fehlschlägt.
            QMessageBox::critical (
                nullptr,
                QCoreApplication::translate ("main", "Fehler beim Löschen"),
                QCoreApplication::translate ("main",
                                             "Konnte die alte Python-Umgebung nicht vollständig "
                                             "entfernen. Bitte manuell löschen: %1")
                    .arg (venvPath));
            return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

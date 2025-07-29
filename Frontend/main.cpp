/**
 * @file main.cpp
 * @brief Der Haupteinstiegspunkt der Anwendung.
 * @author Mike Wild
 */
#include "installationdialog.h"
#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include "pythonenvironmentmanager.h"
#include "settingswizard.h"

/*
 * ToDo:
 *
 * Audio abgreifen auf Mac => Paki
 * ASR-Verarbeitung auf Echtzeit umstellen => sobald Echtzeit-Modell fertig ist
 *
 * In Zusammenarbeit mit Yolanda:
 * Meetings-Liste aus Datenbank abrufen
 * Meetings-Liste nach Tags durchsuchbar machen
 * fertiges Meeting mit in die Datenbank aufnehmen
 * */

int main (
    int argc, char *argv[])
{
    QApplication app (argc, argv);
    app.setApplicationName ("AudioTranskriptor");
    app.setOrganizationName ("SS2025FP_T2");

    // Instanziiere den PythonEnvironmentManager
    PythonEnvironmentManager pythonManager;

    // Initialer Check beim Start
    if (!pythonManager.checkAndSetup (
            false, nullptr)) // Kein forceReinstall, kein Parent für QMessageBox im Main-Thread
    {
        // Wenn das automatische Setup fehlschlägt oder abgebrochen wird
        SettingsWizard wizard;
        if (wizard.exec () != QDialog::Accepted)
        {
            QMessageBox::critical (
                nullptr,
                QCoreApplication::translate ("main", "Abbruch"),
                QCoreApplication::translate (
                    "main", "Kein gültiger Python-Pfad konfiguriert. Das Programm wird beendet."));
            return 1;
        }

        // Nach dem Wizard prüfen wir erneut.
        QSettings settings;
        if (!QFile::exists (settings.value ("pythonPath").toString ()))
        {
            QMessageBox::critical (
                nullptr,
                QCoreApplication::translate ("main", "Abbruch"),
                QCoreApplication::translate (
                    "main", "Der manuell gesetzte Pfad ist ungültig. Das Programm wird beendet."));
            return 1;
        }
    }

    MainWindow w;
    w.show ();
    return app.exec ();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

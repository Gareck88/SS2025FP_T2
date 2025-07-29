/**
 * @file main.cpp
 * @brief Hauptanwendungsdatei mit Datenbankinitialisierung
 */

#include "installationdialog.h"
#include "mainwindow.h"
#include "pythonenvironmentmanager.h"
#include "settingswizard.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QPluginLoader>
#include <QLibrary>
#include <QDir>

bool initializeDatabaseConnection()
{
    // 1. Laden der libpq-Bibliothek
    QLibrary libpq("/usr/local/lib/libpq.dylib"); // Intel Mac
    // QLibrary libpq("/opt/homebrew/lib/libpq.dylib"); // Apple Silicon
    if(!libpq.load()) {
        qCritical() << "Fehler beim Laden von libpq:" << libpq.errorString();
        return false;
    }

    // 2. Setzen der Plugin-Pfade
    QCoreApplication::addLibraryPath("/usr/local/lib/qt/plugins");
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/../PlugIns");

    // 3. Manuelles Laden des QPSQL-Treibers
    QPluginLoader loader;
    QString driverPath = "/usr/local/lib/qt/plugins/sqldrivers/libqsqlpsql.dylib"; // Intel
    // QString driverPath = "/opt/homebrew/lib/qt/plugins/sqldrivers/libqsqlpsql.dylib"; // Apple Silicon

    loader.setFileName(driverPath);
    if(!loader.load()) {
        qCritical() << "Fehler beim Laden des QPSQL-Treibers:";
        qCritical() << "Fehlermeldung:" << loader.errorString();
        qCritical() << "Versuchter Pfad:" << driverPath;
        return false;
    }

    // 4. Datenbankverbindung herstellen
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("postgres");
    db.setUserName(QDir::home().dirName()); // Aktueller Systembenutzer

    if(!db.open()) {
        qCritical() << "Datenbankverbindung fehlgeschlagen:";
        qCritical() << "Fehler:" << db.lastError().text();
        qCritical() << "Benutzer:" << db.userName();
        qCritical() << "Datenbank:" << db.databaseName();
        return false;
    }

    qDebug() << "Erfolgreich mit PostgreSQL verbunden!";
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("AudioTranskriptor");
    app.setOrganizationName("SS2025FP_T2");

    // Debug-Informationen aktivieren
    qputenv("QT_DEBUG_PLUGINS", "1");

    // Datenbankverbindung initialisieren
    if(!initializeDatabaseConnection()) {
        QMessageBox::warning(nullptr,
                             "Datenbankverbindung",
                             "Konnte keine Verbindung zur Datenbank herstellen.\n"
                             "Bitte überprüfen Sie:\n"
                             "1. PostgreSQL ist installiert (brew install postgresql@14)\n"
                             "2. Der Server läuft (brew services start postgresql@14)\n"
                             "3. Die libpq-Bibliothek ist vorhanden\n\n"
                             "Einige Funktionen sind möglicherweise eingeschränkt.");
    }

    // Python-Umgebung prüfen
    PythonEnvironmentManager pythonManager;
    if(!pythonManager.checkAndSetup(false, nullptr)) {
        SettingsWizard wizard;
        if(wizard.exec() != QDialog::Accepted) {
            QMessageBox::critical(nullptr,
                                  "Python-Umgebung",
                                  "Keine gültige Python-Umgebung gefunden.\n"
                                  "Das Programm wird beendet.");
            return 1;
        }
    }

    // Hauptfenster anzeigen
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

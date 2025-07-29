/**
 * @file pythonenvironmentmanager.h
 * @brief Enthält die Deklaration der PythonEnvironmentManager-Klasse.
 * @author Mike Wild
 */
#ifndef PYTHONENVIRONMENTMANAGER_H
#define PYTHONENVIRONMENTMANAGER_H

#include <QObject>

class QWidget; // Forward-Deklaration

/**
 * @brief Verwaltet die Python-Umgebung und deren Installation/Prüfung.
 *
 * Diese Klasse kapselt die gesamte Logik, die für die Überprüfung,
 * Einrichtung, und optionale Neuinstallation der Python-Umgebung
 * (insbesondere einer virtuellen Umgebung und ihrer Abhängigkeiten)
 * erforderlich ist. Sie interagiert mit dem InstallationDialog, um den
 * Fortschritt anzuzeigen und Rückmeldungen zu geben.
 */
class PythonEnvironmentManager : public QObject
{
    Q_OBJECT
public:
    explicit PythonEnvironmentManager (QObject *parent = nullptr);

    /**
     * @brief Überprüft die Python-Umgebung und führt bei Bedarf eine Installation durch.
     *
     * Diese Methode ist der zentrale Einstiegspunkt. Sie prüft, ob ein gültiger Python-Pfad
     * vorhanden ist. Falls nicht oder wenn `forceReinstall` true ist, wird eine
     * Neuinstallation initiiert. Ein modaler `InstallationDialog` wird angezeigt,
     * um den Fortschritt darzustellen.
     *
     * @param forceReinstall Wenn `true`, wird die virtuelle Python-Umgebung gelöscht
     * und neu aufgebaut, auch wenn bereits ein Pfad konfiguriert ist.
     * @param parentWidget Ein optionales Eltern-Widget für den `InstallationDialog`.
     * Dies ist wichtig für die korrekte Positionierung und Modalität des Dialogs.
     * @return `true`, wenn die Umgebung erfolgreich konfiguriert wurde, `false` bei Fehler oder Abbruch.
     */
    bool checkAndSetup (bool forceReinstall = false, QWidget *parentWidget = nullptr);


private slots:
    /**
     * @brief Slot zur Verarbeitung des `installationFinished`-Signals vom `InstallationDialog`.
     *
     * Dieser Slot wird aufgerufen, wenn der `InstallationDialog` seine Arbeit beendet hat.
     * Er puffert das Ergebnis intern, um es nach dem Schließen des Dialogs auszuwerten.
     * @param success `true`, wenn der Prozess im Dialog erfolgreich war.
     * @param errorMessage Eine eventuelle Fehlermeldung vom Dialog.
     */
    void handleInstallationDialogFinished (bool success, const QString &errorMessage);

private:
    /**
     * @brief Löscht rekursiv eine vorhandene virtuelle Python-Umgebung.
     * @param venvPath Der absolute Pfad zum Verzeichnis der virtuellen Umgebung.
     * @return `true` bei erfolgreichem Löschen oder wenn das Verzeichnis nicht existiert.
     */
    bool removeVirtualEnvironment (const QString &venvPath);

    bool m_dialogSuccess;         ///< Speichert das Erfolgsergebnis des `InstallationDialog`.
    QString m_dialogErrorMessage; ///< Speichert die Fehlermeldung des `InstallationDialog`.
};

#endif // PYTHONENVIRONMENTMANAGER_H

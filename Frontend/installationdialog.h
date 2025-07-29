/**
 * @file installationdialog.h
 * @brief Enthält die Deklaration des InstallationDialog.
 * @author Mike Wild
 */
#ifndef INSTALLATIONDIALOG_H
#define INSTALLATIONDIALOG_H

#include <QDialog>
#include <QProcess>

// Forward-Deklarationen
class QTextEdit;
class QPushButton;
class QVBoxLayout;

/**
 * @brief Ein modaler Dialog, der die Ausgabe des Python-Setup-Skripts anzeigt.
 *
 * Dieser Dialog startet das Setup-Skript in einem eigenen QProcess und leitet
 * dessen Standard- und Fehlerausgabe in ein Textfeld um. Er meldet das
 * Ergebnis des Prozesses über ein Signal zurück.
 */
class InstallationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InstallationDialog (QWidget *parent = nullptr);
    ~InstallationDialog ();

public slots:
    /** @brief Startet die Ausführung des plattformspezifischen Python-Setup-Skripts. */
    void startPythonSetup ();

signals:
    /**
     * @brief Wird emittiert, wenn der Installationsprozess beendet ist.
     * @param success `true`, wenn der Prozess erfolgreich war.
     * @param errorMessage Eine Fehlermeldung bei Misserfolg.
     */
    void installationFinished (bool success, const QString &errorMessage = "");

private slots:
    /** @brief Hängt die Ausgaben (stdout/stderr) des Prozesses an das Textfeld an. */
    void appendOutput ();

    /** @brief Behandelt das `finished`-Signal des QProcess. */
    void handleProcessFinished (int exitCode, QProcess::ExitStatus exitStatus);

    /** @brief Behandelt das `errorOccurred`-Signal des QProcess. */
    void handleProcessError (QProcess::ProcessError error);

    /** @brief Behandelt den Klick auf den Abbrechen/Schließen-Button. */
    void handleCancelButtonClicked ();

private:
    QTextEdit *m_outputDisplay; ///< Zeigt die Konsolenausgabe des Setup-Skripts an.
    QPushButton *m_closeButton; ///< Button zum Abbrechen oder Schließen des Dialogs.
    QProcess *m_setupProcess;   ///< Der Prozess, der das Setup-Skript ausführt.
};

#endif // INSTALLATIONDIALOG_H

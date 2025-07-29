/**
 * @file mainwindow.h
 * @brief Enthält die Deklaration der MainWindow-Klasse, dem Hauptfenster der Anwendung.
 * @author Mike Wild
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QElapsedTimer>
#include <QJsonDocument>
#include <QListWidget>
#include <QMainWindow>
#include <QStack>

// Eigene Klassen
#include "asrprocessmanager.h"
#include "filemanager.h"
#include "transcription.h"

// Forward-Deklarationen
class TagGeneratorManager;
class SpeakerEditorDialog;
class TextEditorDialog;
class WavWriterThread;
class CaptureThread;
class SettingsWizard;
class QAction;
class QCloseEvent;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QMenuBar;
class QProcess;
class QPushButton;
class QSplitter;
class QTextEdit;
class QTimer;
class QVBoxLayout;

/**
 * @brief Das Hauptfenster und die zentrale Steuerungseinheit der Anwendung.
 *
 * Die MainWindow-Klasse ist verantwortlich für die Darstellung der Benutzeroberfläche
 * und die Koordination der verschiedenen Hintergrundprozesse und Manager-Klassen.
 * Sie nimmt Benutzerinteraktionen entgegen und delegiert die Aufgaben an die
 * zuständigen Komponenten.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow (QWidget *parent = nullptr);
    ~MainWindow () override;

protected:
    /**
     * @brief Wird aufgerufen, wenn der Benutzer versucht, das Fenster zu schließen.
     * @param event Das Close-Event, das ggf. ignoriert werden kann, um das Schließen zu verhindern.
     * @todo Die Logik zum Speichern bei ungesicherten Änderungen soll zukünftig ebenfalls
     * die **Datenbank-Speicherfunktion** aufrufen.
     */
    void closeEvent (QCloseEvent *event) override;

private slots:
    /** @brief Startet eine neue Audio-Aufnahmesession. */
    void onStartClicked ();

    /** @brief Beendet die laufende Audio-Aufnahmesession. */
    void onStopClicked ();

    /**
     * @brief Temporärer Platzhalter zur Simulation von Echtzeit-Transkripten.
     * @todo Diese Methode ist nur für Demonstrationszwecke und wird entfernt,
     * sobald die Echtzeit-Verarbeitung via ASR-Manager implementiert ist.
     */
    void onPollTranscripts ();

    /** @brief Öffnet einen Dialog zum Speichern der hochqualitativen WAV-Audiodatei. */
    void onSaveAudio ();

    /** @brief Delegiert die Erstellung und Anzeige einer PDF-Version des Transkripts. */
    void onSavePDF ();

    /** @brief Öffnet den Dialog zur Bearbeitung und Zuordnung von Sprechern. */
    void onEditSpeakers ();

    /** @brief Startet den Prozess zur automatischen Generierung von Tags für das Transkript. */
    void onGenerateTags ();

    /**
     * @brief Lädt das ausgewählte Meeting aus der Liste bei einem Doppelklick.
     * @param item Das angeklickte Listenelement, dessen Text die Meeting-ID enthält.
     * @todo Die Methode verwendet derzeit den FileManager, um eine JSON-Datei zu laden.
     * Dies soll durch eine **Datenbankabfrage** ersetzt werden, die das Meeting anhand seiner ID lädt.
     */
    void onMeetingSelected (QListWidgetItem *item);

    /** @brief Filtert die Meeting-Liste basierend auf der Eingabe im Suchfeld. */
    void onSearchTextChanged (const QString &text);

    /** @brief Startet den ASR-Prozess für die zuletzt aufgenommene Audiodatei. */
    void processAudio ();

    /** @brief Öffnet den Einstellungsdialog. */
    void openSettingsWizard ();

    /** @brief Zeigt eine Nachricht in der Statusleiste an. */
    void setStatus (const QString &text, bool keep = false);

    /** @brief Öffnet den Dialog zur Bearbeitung des Transkript-Textes. */
    void onEditTranscript ();

    /** @brief Macht die letzte Änderung am Transkript rückgängig. */
    void onUndo ();

    /** @brief Wiederholt die letzte rückgängig gemachte Änderung. */
    void onRedo ();

    /**
     * @brief Öffnet einen Dateidialog, um ein Transkript im JSON-Format zu laden.
     * @todo Diese Funktion zum Laden einer beliebigen Datei bleibt bestehen, aber die primäre
     * Ladefunktion wird der Datenbankzugriff über onMeetingSelected().
     */
    void loadTranscriptionFromJson ();

    /**
     * @brief Speichert das aktuell bearbeitete Transkript.
     * @todo Ersetzt aktuell die zugehörige JSON-Datei. Dies soll in Zukunft
     * einen 'UPDATE'-Befehl an die **Datenbank** senden, um den bestehenden Eintrag zu aktualisieren.
     */
    void saveTranscriptionToJson ();

    /**
     * @brief Öffnet einen Dateidialog, um das Transkript unter einem neuen Namen zu speichern.
     * @todo Statt als neue Datei zu speichern, soll dies zukünftig einen
     * 'INSERT'-Befehl an die **Datenbank** senden, um einen neuen Eintrag zu erzeugen.
     */
    void saveTranscriptionToJsonAs ();

    /**
     * @brief Setzt das aktuelle Transkript auf die ursprünglich generierte Version zurück.
     * @todo Lädt aktuell das `_original.json`-File. Zukünftig soll dies den
     * Originalzustand des Transkripts aus der **Datenbank** wiederherstellen.
     */
    void restoreOriginalTranscription ();

    /** @brief Aktualisiert den Zustand der Undo/Redo-Buttons. */
    void updateUndoRedoState ();

    /** @brief Setzt den Namen für das aktuelle Meeting. */
    void setMeetingName (const QString &name);

    /** @brief Öffnet einen Dialog, um den Meeting-Namen manuell einzugeben. */
    void onSetMeetingName ();

    /**
   * @brief Startet den Prozess zur Neuinstallation der Python-Umgebung.
   * @note Zeigt einen Bestätigungsdialog vor der Ausführung.
   */
    void onReinstallPython ();

    /**
   * @brief Behandelt das backendReady-Signal vom ASR-Backend
   * @author Fabian Scherer
   */
    void handleBackendReady ();
    /**
   * @brief Behandelt empfangene Transkriptionen vom ASR-Backend
   * @author Fabian Scherer
   */
    void handleTranscriptionReady(const QString &speaker, const QString &text);
    // NEUER SLOT: 
    /**
   * @brief Behandelt Fehler- und Statusmeldungen vom ASR-Backend
   * @author Fabian Scherer
   */
    void handleBackendError(const QString &message);

private:
    /** @brief Erstellt und arrangiert alle UI-Widgets. */
    void setupUI ();

    /** @brief Bündelt alle Signal-Slot-Verbindungen. */
    void doConnects ();

    /**
     * @brief Lädt die Liste der verfügbaren Meetings in die Seitenleiste.
     * @todo Aktuell wird das Dateisystem durchsucht. Zukünftig soll diese Methode
     * eine **Datenbankabfrage** ausführen, um alle gespeicherten Meetings abzurufen.
     */
    void loadMeetings ();

    /** @brief Aktualisiert den Zustand der UI (Buttons, Labels) basierend auf dem aktuellen Meeting-Status. */
    void updateUiForCurrentMeeting ();

    /** @brief Wendet einen Filter auf die sichtbaren Elemente der Meeting-Liste an. */
    void filterMeetings (const QString &filter);


    /** @brief Konstruiert den Anzeige-Namen für das aktuelle Meeting aus Name und Datum. */
    QString currentName () const;

    // Undo/Redo-Logik
    QStack<QJsonDocument> m_undoStack; ///< Stapel für die Undo-Zustände.
    QStack<QJsonDocument> m_redoStack; ///< Stapel für die Redo-Zustände.
    QAction *m_undoAction;             ///< Menü-Aktion für Undo.
    QAction *m_redoAction;             ///< Menü-Aktion für Redo.

    //  Menü-Aktionen
    QAction *m_actionOpen;
    QAction *m_actionSave;
    QAction *m_actionSaveAs;
    QAction *m_actionRestoreOriginal;
    QAction *m_actionClose;
    QAction *m_actionSetMeetingName;
    QAction *m_settingsAction;
    QAction *m_reinstallPythonAction;

    // Threads und Manager
    CaptureThread *m_captureThread;      ///< Thread für die plattformspezifische Audio-Aufnahme.
    WavWriterThread *m_wavWriter;        ///< Thread zum Schreiben der WAV-Dateien.
    Transcription *m_script;             ///< Das zentrale Datenmodell für das Transkript.
    FileManager *m_fileManager;          ///< Manager für alle Dateizugriffe.
    AsrProcessManager *m_asrManager;     ///< Manager für den ASR-Python-Prozess.
    TagGeneratorManager *m_tagGenerator; ///< Manager für den Tag-Generator-Python-Prozess.

    // UI-Widgets
    QSplitter *splitter;
    QWidget *leftPanel;
    QLineEdit *searchBox;
    QListWidget *meetingList;
    QWidget *rightPanel;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonLayout;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *saveAudioButton;
    QPushButton *savePDFButton;
    QPushButton *assignNamesButton;
    QPushButton *generateTagsButton;
    QPushButton *editTextButton;
    QLabel *timeLabel;
    QLabel *nameLabel;
    QLabel *statusLabel;
    QTextEdit *transcriptView;
    QTimer *pollTimer;
    QTimer *timeUpdateTimer;
    QTimer *statusTimer;
    QElapsedTimer elapsedTime;
    SpeakerEditorDialog *m_speakerEditorDialog;
    TextEditorDialog *m_textEditorDialog;

    // Zustandsvariablen
    QString m_currentAudioPath;   ///< Pfad zur zuletzt gespeicherten Audiodatei.
    QString m_currentMeetingName; ///< Name des aktuellen Meetings (wird bei Aufnahme/Laden gesetzt).
    QString m_currentMeetingDateTime; ///< Zeitstempel des aktuellen Meetings.

    QProcess *pluginProcess; ///< Platzhalter für einen möglichen IPC-Prozess.
};

#endif // MAINWINDOW_H

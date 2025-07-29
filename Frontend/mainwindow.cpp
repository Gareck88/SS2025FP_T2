#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextEdit>
#include <QTimer>
#include <QtConcurrent> //  Fürs parallele Kopieren der Wav-Datei

#include "audiofactory.h"
#include "capturethread.h"
#include "databasemanager.h"
#include "multisearchdialog.h"
#include "pythonenvironmentmanager.h"
#include "searchdialog.h"
#include "settingswizard.h"
#include "speakereditordialog.h"
#include "taggeneratormanager.h"
#include "texteditordialog.h"
#include "transcriptpdfexporter.h"
#include "wavwriterthread.h"

//--------------------------------------------------------------------------------------------------

MainWindow::MainWindow (QWidget *parent)
    : QMainWindow (parent)
    , splitter (new QSplitter (this))
    , leftPanel (new QWidget (this))
    , searchBox (new QLineEdit (this))
    , meetingList (new QListWidget (this))
    , rightPanel (new QWidget (this))
    , mainLayout (new QVBoxLayout)
    , buttonLayout (new QHBoxLayout)
    , startButton (new QPushButton (tr ("Start"), this))
    , stopButton (new QPushButton (tr ("Stop"), this))
    , saveAudioButton (new QPushButton (tr ("Audio speichern"), this))
    , savePDFButton (new QPushButton (tr ("Transkript als PDF"), this))
    , assignNamesButton (new QPushButton (tr ("Sprecher zuordnen"), this))
    , editTextButton (new QPushButton (tr ("Text bearbeiten"), this))
    , generateTagsButton (new QPushButton (tr ("Tags generieren"), this))
    , searchButton (new QPushButton (tr ("Suche im Transkript"), this))
    , toggleButton (new QPushButton (tr ("Transkript umschalten"), this))
    , multiSearchButton (new QPushButton (tr ("Suche in allen Transkripten"), this))
    , transcriptView (new QTextEdit (this))
    , pollTimer (new QTimer (this))
    , statusTimer (new QTimer (this))
    , pluginProcess (new QProcess (this))
    , timeUpdateTimer (new QTimer (this))
    , m_script (new Transcription (this))
    , m_speakerEditorDialog (nullptr)
    , m_captureThread (AudioFactory::createThread (this))
    , m_wavWriter (new WavWriterThread (this))
    , m_fileManager (new FileManager (this))
    , m_asrManager (new AsrProcessManager (this))
    , m_tagGenerator (new TagGeneratorManager (this))
    , m_textEditorDialog (nullptr)
    , m_databaseManager (new DatabaseManager (this))
    , m_searchDialog (new SearchDialog (this))
    , m_multiSearchDialog (new MultiSearchDialog (this))
{
    //  Konfiguriert den Timer, der Statusnachrichten nach 3 Sekunden automatisch ausblendet.
    statusTimer->setSingleShot (true);
    statusTimer->setInterval (3000);

    //  Initialisiert die Benutzeroberfläche und lädt gespeicherte Meetings.
    setupUI ();

    // Datenbank-Verbindung versuchen
    if (!m_databaseManager->connectToSupabase ())
    {
        // Warnung anzeigen, aber App läuft weiter
        QMessageBox::warning (
            this,
            "Datenbankfehler",
            "Konnte keine Verbindung zur Supabase-Datenbank herstellen.\n"
            "Bitte überprüfen Sie die Einstellungen unter 'Einstellungen'.\n"
            "Einige Funktionen sind deaktiviert, bis die Verbindung hergestellt ist.");
    }
    // Nur laden, wenn DB-Verbindung erfolgreich war
    if (m_databaseManager->isConnected ())
    {
        loadMeetings ();
    }
    else
    {
        qDebug () << "Meetings werden nicht geladen, da keine DB-Verbindung besteht.";
    }

    //  Alle Signal-Slot-Verbindungen werden in einer separaten Methode gekapselt,
    //  um den Konstruktor übersichtlich zu halten.
    doConnects ();

    //  Setzt den initialen Zustand der UI-Elemente (meist deaktiviert).
    transcriptView->setReadOnly (true);
    stopButton->setEnabled (false);
    saveAudioButton->setEnabled (false);
    savePDFButton->setEnabled (false);
    generateTagsButton->setEnabled (false);
    assignNamesButton->setEnabled (false);
    editTextButton->setEnabled (false);

    //  Die Hintergrund-Threads für die Aufnahme werden direkt gestartet.
    //  Sie gehen sofort in einen Wartezustand, bis startCapture() bzw. startWriting() aufgerufen wird.
    m_captureThread->start ();
    m_wavWriter->start ();

    //  Setzt den finalen, sauberen Anfangszustand der UI.
    updateUiForCurrentMeeting ();

    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    restoreGeometry (settings.value ("geometry").toByteArray ());
}

//--------------------------------------------------------------------------------------------------

MainWindow::~MainWindow ()
{
    //  Stellt sicher, dass alle Hintergrund-Threads explizit und sauber heruntergefahren werden,
    //  bevor das Hauptfenster zerstört wird. Dies verhindert Abstürze beim Beenden.
    if (m_captureThread)
    {
        m_captureThread->shutdown ();
    }

    if (m_wavWriter)
    {
        m_wavWriter->shutdown ();
    }

    if (pluginProcess->state () != QProcess::NotRunning)
    {
        pluginProcess->terminate ();
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::closeEvent (
    QCloseEvent *event)
{
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    settings.setValue ("geometry", saveGeometry ());

    // Wenn kein Transkript geladen ist, einfach schließen
    if (m_script->name ().isEmpty ())
    {
        event->accept ();
        return;
    }
    // Hole das Original-Transkript aus der Map
    Transcription *original = m_transcriptions.value (m_script->name (), nullptr);

    // Wenn das Original nicht existiert oder keine Änderungen vorliegen, einfach schließen
    if (!original || m_script->isContentEqual (original))
    {
        event->accept ();
        return;
    }

    // Zeige Dialog zur Bestätigung des Speicherns
    QMessageBox::StandardButton reply
        = QMessageBox::question (this,
                                 tr ("Änderungen speichern"),
                                 tr ("Sie haben Änderungen vorgenommen. Möchten Sie speichern?"),
                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    switch (reply)
    {
    case QMessageBox::Yes:
        updateTranscriptionInDatabase ();
        event->accept ();
        break;
    case QMessageBox::No:
        event->accept ();
        break;
    case QMessageBox::Cancel:
    default:
        event->ignore ();
        break;
    }

    //  Sollte nie erreicht werden
    //  Akzeptiere das Schließen-Event, damit das Fenster geschlossen wird.
    event->accept ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::setupUI ()
{
    //  Grundeinstellungen für das Hauptfenster.
    setWindowTitle (tr ("AudioTranskriptor"));
    resize (1000, 700);

    //  Linkes Panel: Meeting-Liste mit Suche
    QVBoxLayout *leftLayout = new QVBoxLayout ();
    searchBox->setPlaceholderText (tr ("Suchen..."));
    leftLayout->addWidget (searchBox);
    leftLayout->addWidget (meetingList);
    leftPanel->setLayout (leftLayout);

    //  Rechtes Panel: Steuerelemente und Transkript-Anzeige
    QHBoxLayout *buttonLayout = new QHBoxLayout ();
    buttonLayout->addWidget (startButton);
    buttonLayout->addWidget (stopButton);
    buttonLayout->addWidget (saveAudioButton);
    buttonLayout->addWidget (savePDFButton);
    buttonLayout->addWidget (assignNamesButton);
    buttonLayout->addWidget (editTextButton);
    buttonLayout->addWidget (generateTagsButton);
    buttonLayout->addWidget (multiSearchButton);

    QHBoxLayout *labelLayout = new QHBoxLayout ();
    statusLabel = new QLabel (this);
    statusLabel->setVisible (false);
    nameLabel = new QLabel (this);
    timeLabel = new QLabel (this);

    transkriptStatusLabel = new QLabel (this);
    QFont f = nameLabel->font ();
    f.setPointSize (10); //  Einheitliche, kleinere Schrift für die Statuszeilen-Labels.
    nameLabel->setFont (f);
    timeLabel->setFont (f);
    transkriptStatusLabel->setFont (f);
    nameLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);
    timeLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);
    transkriptStatusLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);

    labelLayout->setContentsMargins (0, 0, 0, 0);
    labelLayout->setSpacing (8);
    labelLayout->addWidget (statusLabel, 0, Qt::AlignVCenter);
    labelLayout->addStretch ();
    labelLayout->addWidget (nameLabel, 0, Qt::AlignVCenter);
    labelLayout->addWidget (timeLabel, 0, Qt::AlignVCenter);
    labelLayout->addWidget (transkriptStatusLabel, 0, Qt::AlignVCenter);
    labelLayout->addWidget (toggleButton, 0, Qt::AlignVCenter);
    labelLayout->addWidget (searchButton, 0, Qt::AlignVCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout ();
    mainLayout->addLayout (buttonLayout);
    mainLayout->addLayout (labelLayout);
    mainLayout->addWidget (transcriptView);
    rightPanel->setLayout (mainLayout);

    //  Splitter zur größenveränderlichen Aufteilung der Panels
    splitter = new QSplitter (this);
    splitter->addWidget (leftPanel);
    splitter->addWidget (rightPanel);
    //  Sorgt dafür, dass das rechte Panel (Index 1) initial dreimal so breit
    //  ist wie das linke Panel (Index 0, impliziter Faktor 1).
    splitter->setStretchFactor (1, 3);

    //  Menüleiste und die einzelnen Menüs erstellen
    QMenuBar *menuBar = new QMenuBar (this);
    setMenuBar (menuBar);
    QMenu *menuDatei = new QMenu (tr ("Datei"), this);
    QMenu *menuBearbeiten = new QMenu (tr ("Bearbeiten"), this);
    QMenu *menuExtras = new QMenu (tr ("Extras"), this);
    menuBar->addMenu (menuDatei);
    menuBar->addMenu (menuBearbeiten);
    menuBar->addMenu (menuExtras);

    //  Datei-Menüeinträge erstellen und den Member-Variablen zuweisen
    m_actionOpen = new QAction (tr ("Transkript aus Json laden..."), this);
    //m_actionSave = new QAction (tr ("Transkript speichern"), this);
    m_actionSaveAs = new QAction (tr ("Transkript speichern unter..."), this);
    m_actionSaveToDB = new QAction (tr ("Transkript in Datenbank speichern"), this);
    m_actionRestoreOriginal = new QAction (tr ("Original wiederherstellen"), this);
    m_actionClose = new QAction (tr ("Beenden"), this);
    m_actionSetMeetingName = new QAction (tr ("Meetingname setzen..."), this);
    m_reinstallPythonAction = new QAction (tr ("Python neu-installieren"), this);

    //  Standard-Tastenkürzel für die Aktionen festlegen
    m_actionOpen->setShortcut (QKeySequence::Open);
    m_actionSaveToDB->setShortcut (QKeySequence::Save);
    //m_actionSave->setShortcut (QKeySequence::Save);
    m_actionSaveAs->setShortcut (QKeySequence::SaveAs);
    m_actionClose->setShortcut (QKeySequence::Quit);

    //  Aktionen dem "Datei"-Menü hinzufügen
    menuDatei->addAction (m_actionOpen);
    //menuDatei->addAction (m_actionSave);
    menuDatei->addAction (m_actionSaveAs);
    menuDatei->addSeparator ();
    menuDatei->addAction (m_actionSaveToDB);
    menuDatei->addSeparator ();
    menuDatei->addAction (m_actionRestoreOriginal);
    menuDatei->addSeparator ();
    menuDatei->addAction (m_actionClose);

    //  Bearbeiten-Menüeinträge erstellen (waren bereits Member-Variablen)
    m_undoAction = new QAction (tr ("Rückgängig"), this);
    m_redoAction = new QAction (tr ("Wiederholen"), this);

    m_undoAction->setShortcut (QKeySequence::Undo);
    m_redoAction->setShortcut (QKeySequence::Redo);
    m_undoAction->setEnabled (false);
    m_redoAction->setEnabled (false);

    menuBearbeiten->addAction (m_undoAction);
    menuBearbeiten->addAction (m_redoAction);

    //  Extras-Menüeinträge erstellen
    m_settingsAction = new QAction (tr ("Einstellungen..."), this);
    menuExtras->addAction (m_actionSetMeetingName);
    menuExtras->addAction (m_settingsAction);
    menuExtras->addAction (m_reinstallPythonAction);

    //  Die Aktionen der Menüeinträge mit den entsprechenden Slots verbinden.
    //  Diese Verbindungen werden in der doConnects()-Methode gebündelt.
    transcriptView->setObjectName ("transcriptView");

    setCentralWidget (splitter);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::doConnects ()
{
    //  --- 1. Direkte UI-Interaktionen ---
    //  Verbindet die Klicks auf die Haupt-Buttons mit ihren jeweiligen Aktionen.
    connect (startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect (stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect (saveAudioButton, &QPushButton::clicked, this, &MainWindow::onSaveAudio);
    connect (savePDFButton, &QPushButton::clicked, this, &MainWindow::onSavePDF);
    connect (assignNamesButton, &QPushButton::clicked, this, &MainWindow::onEditSpeakers);
    connect (editTextButton, &QPushButton::clicked, this, &MainWindow::onEditTranscript);
    connect (generateTagsButton, &QPushButton::clicked, this, &MainWindow::onGenerateTags);
    connect (meetingList, &QListWidget::itemDoubleClicked, this, &MainWindow::onMeetingSelected);
    connect (searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect (searchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect (multiSearchButton, &QPushButton::clicked, this, &MainWindow::openMultiSearchDialog);
    connect (toggleButton, &QPushButton::clicked, this, &MainWindow::toggleTranscriptionVersion);

    //  Menü "Datei"
    connect (m_actionOpen, &QAction::triggered, this, &MainWindow::loadTranscriptionFromJson);
    //connect (m_actionSave, &QAction::triggered, this, &MainWindow::saveTranscriptionToJson);
    connect (m_actionSaveAs, &QAction::triggered, this, &MainWindow::saveTranscriptionToJsonAs);
    connect (m_actionSaveToDB,
             &QAction::triggered,
             this,
             &MainWindow::updateTranscriptionInDatabase);
    connect (m_actionRestoreOriginal,
             &QAction::triggered,
             this,
             &MainWindow::restoreOriginalTranscription);
    connect (m_actionClose, &QAction::triggered, this, &MainWindow::close);

    //  Menü "Bearbeiten"
    connect (m_undoAction, &QAction::triggered, this, &MainWindow::onUndo);
    connect (m_redoAction, &QAction::triggered, this, &MainWindow::onRedo);

    //  Menü "Extras"
    connect (m_actionSetMeetingName, &QAction::triggered, this, &MainWindow::onSetMeetingName);
    connect (m_settingsAction, &QAction::triggered, this, &MainWindow::openSettingsWizard);
    connect (m_reinstallPythonAction, &QAction::triggered, this, &MainWindow::onReinstallPython);

    //  --- 2. Asynchrone Logik-Ketten (State Machine via Signals & Slots) ---

    //  A. Audio-Daten-Pipeline: CaptureThread -> WavWriterThread
    //  Der CaptureThread sendet Daten, der Writer-Thread empfängt und schreibt sie.
    //  Qt::QueuedConnection ist wichtig für die Thread-sichere Kommunikation.
    connect (m_captureThread,
             &CaptureThread::pcmChunkReady,
             m_wavWriter,
             &WavWriterThread::writeChunk,
             Qt::QueuedConnection);

    //  B. Aufnahme-Start-Logik: UI-Reaktion auf den Start des CaptureThreads.
    connect (m_captureThread,
             &CaptureThread::started,
             this,
             [this] ()
             {
                 elapsedTime.restart ();
                 timeUpdateTimer->start (100); //  Intervall für die Zeitanzeige: 100ms
                 setStatus ("es wird aufgezeichnet", true);
             });

    //  C. Aufnahme-Stopp-Logik (Kette 1): CaptureThread -> WavWriterThread
    //  Wenn die Aufnahme stoppt, wird der WavWriter angewiesen, seine Arbeit zu beenden.
    connect (m_captureThread,
             &CaptureThread::stopped,
             this,
             [this] ()
             {
                 m_wavWriter->stopWriting ();
                 setStatus ("Aufnahme beendet, speichere und verarbeite...", true);
             });

    //  D. Verarbeitungs-Logik (Kette 2): WavWriterThread -> ASR-Prozess
    //  Wenn der WavWriter fertig ist, wird der ASR-Prozess angestoßen.
    connect (m_wavWriter, &WavWriterThread::finishedWriting, this, &MainWindow::processAudio);
    connect (m_wavWriter,
             &WavWriterThread::finishedWriting,
             this,
             [=] () { saveAudioButton->setEnabled (true); });

    //  --- 3. Timer und Datenmodell-Synchronisation ---

    //  E. Timer für die laufende Zeitanzeige während der Aufnahme.
    connect (timeUpdateTimer,
             &QTimer::timeout,
             this,
             [this]
             {
                 qint64 ms = elapsedTime.elapsed ();
                 //  Umrechnung von Millisekunden in Stunden, Minuten und Sekunden.
                 int h = ms / 3600000;
                 int m = (ms % 3600000) / 60000;
                 int s = (ms % 60000) / 1000;
                 timeLabel->setText (QString ("%1:%2:%3")
                                         .arg (h, 2, 10, QLatin1Char ('0'))
                                         .arg (m, 2, 10, QLatin1Char ('0'))
                                         .arg (s, 2, 10, QLatin1Char ('0')));
             });

    //  F. Timer zum automatischen Ausblenden von Statusnachrichten.
    connect (statusTimer, &QTimer::timeout, this, [this] () { statusLabel->setVisible (false); });

    //  G. Synchronisation zwischen Datenmodell (m_script) und der Benutzeroberfläche.
    //  Aktualisiert die Textansicht bei jeder Änderung am Transkript.
    connect (m_script,
             &Transcription::changed,
             this,
             [=] () { transcriptView->setText (m_script->script ()); });

    //  --- 4. Manager-Verbindungen ---

    //  H. ASR-Manager: Verarbeitet die Ergebnisse des ASR-Prozesses.
    connect (m_asrManager, &AsrProcessManager::segmentReady, m_script, &Transcription::add);
    connect (m_asrManager,
             &AsrProcessManager::finished,
             this,
             [this] (bool success, const QString &errorMsg)
             {
                 if (success)
                 {
                     setStatus ("Verarbeitung beendet");
                     updateUiForCurrentMeeting ();
                 }
                 else
                 {
                     setStatus ("Verarbeitung fehlgeschlagen");
                     QMessageBox::warning (this, tr ("ASR-Fehler"), errorMsg);
                 }
             });

    //  I. Tag-Generator: Verarbeitet die Ergebnisse der Tag-Analyse.
    connect (m_tagGenerator,
             &TagGeneratorManager::tagsReady,
             this,
             [this] (const QStringList &tags, bool success, const QString &errorMsg)
             {
                 if (success)
                 {
                     m_script->setTags (tags);
                     QMessageBox::information (this,
                                               "Generierte Tags",
                                               "Folgende Tags wurden gefunden:\n\n"
                                                   + tags.join ("\n"));
                 }
                 else
                 {
                     QMessageBox::warning (this, "Fehler bei der Tag-Erstellung", errorMsg);
                 }
                 generateTagsButton->setEnabled (true);
             });
}

//--------------------------------------------------------------------------------------------------

void MainWindow::loadMeetings ()
{
    //  Leert die aktuelle Liste in der UI.
    meetingList->clear ();
    // Attempt to connect to the database
    if (!m_databaseManager->connectToSupabase ())
    {
        QMessageBox::critical (this, "Database Error", "❌ Could not connect to Supabase.");
        return;
    }

    //  Beauftragt den FileManager, alle existierenden Meeting-Dateien zu finden.
    m_transcriptions = m_databaseManager->loadAllTranscriptions ();

    if (m_transcriptions.isEmpty ())
    {
        meetingList->addItem ("⚠️ Keine Besprechungen gefunden");
        return;
    }
    //  Fügt die gefundenen Meeting-IDs der Liste in der UI hinzu.
    meetingList->addItems (m_transcriptions.keys ());
}

//--------------------------------------------------------------------------------------------------

void MainWindow::updateUiForCurrentMeeting ()
{
    //  Prüft, ob ein gültiges Transkript mit Inhalt geladen ist.
    bool meetingLoaded = m_script && !m_script->getMetaTexts ().isEmpty ();

    //  Aktiviert oder deaktiviert die Bearbeitungs-Buttons basierend darauf, ob ein Meeting geladen ist.
    savePDFButton->setEnabled (meetingLoaded);
    assignNamesButton->setEnabled (meetingLoaded);
    editTextButton->setEnabled (meetingLoaded);
    generateTagsButton->setEnabled (meetingLoaded);

    //  Der Button zum Speichern der .wav-Datei wird hier pauschal deaktiviert,
    //  da für ein aus einer Datei geladenes Meeting die temporäre .wav-Datei nicht mehr existiert.
    //  Er wird nur nach einer Neuaufnahme explizit aktiviert.
    saveAudioButton->setEnabled (false);

    if (meetingLoaded)
    {
        //  Füllt die Labels mit den Metadaten des geladenen Transkripts.
        searchButton->setVisible (true);
        toggleButton->setVisible (true);
        nameLabel->setText (currentName ());
        timeLabel->setText (m_script->getDurationAsString ());
    }
    else
    {
        //  Setzt die UI in den "Kein Meeting geladen"-Zustand zurück.
        nameLabel->setText ("Keine Aufnahme geladen");
        timeLabel->setText ("00:00:00.0");
        transcriptView->clear ();
        searchButton->setVisible (false);
        toggleButton->setVisible (false);
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::filterMeetings (
    const QString &filter)
{
    //  Iteriert durch alle Einträge der Meeting-Liste.
    for (int i = 0; i < meetingList->count (); ++i)
    {
        QListWidgetItem *item = meetingList->item (i);
        //  Versteckt den Eintrag, wenn sein Text nicht den Filter-String enthält (unabhängig von Groß-/Kleinschreibung).
        item->setHidden (!item->text ().contains (filter, Qt::CaseInsensitive));
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onSearchTextChanged (
    const QString &text)
{
    //  Leitet den neuen Suchtext einfach an die Filter-Funktion weiter.
    filterMeetings (text);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onSearchButtonClicked ()
{
    // Den gefundenen Text in der Abschrift markieren
    m_searchDialog->setTranscription (m_script);
    connect (m_searchDialog,
             &SearchDialog::searchResultSelected,
             this,
             [=] (const QString &text)
             {
                 QTextCursor cursor = transcriptView->document ()->find (text);
                 if (!cursor.isNull ())
                 {
                     transcriptView->setTextCursor (cursor);
                     setStatus (tr ("Gefundener Treffer: \"%1\"").arg (text));
                 }
             });
    m_searchDialog->exec ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::processAudio ()
{
    setStatus ("wird verarbeitet … - bitte warten", true);

    //  Bewahrt die Metadaten der aktuellen Aufnahme (Name, Datum), bevor das
    //  Transkript-Objekt für die neuen ASR-Ergebnisse geleert wird.
    auto nam = m_script->name ();
    auto dat = m_script->dateTime ();
    m_script->clear ();
    m_script->setName (nam);
    m_script->setDateTime (dat);

    //  Gibt den Startschuss an den ASR-Manager, die Verarbeitung zu beginnen.
    QString asrWavPath = m_fileManager->getTempWavPath (true);
    m_asrManager->startTranscription (asrWavPath);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::openSettingsWizard ()
{
    SettingsWizard dlg (this);
    //  dlg.exec() öffnet den Dialog modal, d.h. der Code pausiert hier,
    //  bis der Benutzer den Dialog schließt.
    dlg.exec ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::setStatus (
    const QString &text, bool keep)
{
    statusLabel->setText (text);
    statusLabel->setVisible (true);

    //  Wenn 'keep' false ist, wird ein Timer gestartet, der die Nachricht nach 3 Sekunden ausblendet.
    if (!keep)
    {
        statusTimer->start ();
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onEditTranscript ()
{
    if (!m_script)
    {
        QMessageBox::warning (this, tr ("Fehler"), tr ("Kein Transkriptionsobjekt vorhanden."));
        return;
    }

    //  Stellt sicher, dass nur eine Instanz des Dialogs gleichzeitig existiert (Singleton-Pattern).
    if (!m_textEditorDialog)
    {
        //  Wenn der Dialog noch nicht existiert, wird er erstellt.
        m_textEditorDialog = new TextEditorDialog (m_script, this);

        //  Verbindet das finished-Signal des Dialogs mit einem Lambda, das den Dialog
        //  sicher löscht und den Zeiger zurücksetzt, um Speicherlecks zu vermeiden.
        connect (m_textEditorDialog,
                 &QDialog::finished,
                 this,
                 [this] (int)
                 {
                     m_textEditorDialog->deleteLater ();
                     m_textEditorDialog = nullptr;
                 });
        m_textEditorDialog->show ();
    }
    else
    {
        //  Wenn der Dialog bereits existiert, wird er nur in den Vordergrund geholt.
        m_textEditorDialog->raise ();
        m_textEditorDialog->activateWindow ();
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onUndo ()
{
    if (m_undoStack.isEmpty ())
    {
        return;
    }

    //  1. Den aktuellen Zustand auf den Redo-Stapel legen, um die Aktion wiederherstellen zu können.
    m_redoStack.push (m_script->toJson ());

    //  2. Den letzten Zustand vom Undo-Stapel holen.
    QJsonDocument doc = m_undoStack.pop ();

    //  3. Das Datenmodell mit dem wiederhergestellten Zustand überschreiben.
    m_script->fromJson (doc.toJson ());

    //  4. Die UI-Ansicht aktualisieren.
    transcriptView->setText (m_script->script ());

    updateUndoRedoState ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onRedo ()
{
    if (m_redoStack.isEmpty ())
    {
        return;
    }

    //  1. Den aktuellen Zustand auf den Undo-Stapel legen.
    m_undoStack.push (m_script->toJson ());

    //  2. Den wiederherzustellenden Zustand vom Redo-Stapel holen.
    QJsonDocument doc = m_redoStack.pop ();

    //  3. Das Datenmodell aktualisieren.
    m_script->fromJson (doc.toJson ());

    //  4. Die UI-Ansicht aktualisieren.
    transcriptView->setText (m_script->script ());

    updateUndoRedoState ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::updateTranscriptionInDatabase ()
{
    QSqlDatabase db = QSqlDatabase::database ("supabase");
    if (!db.isOpen ())
    {
        qWarning () << "❌ Datenbank nicht verbunden";
        return;
    }

    QString name = m_script->name ();
    QDateTime startTime = m_script->dateTime ();

    // Schritt 1: Meeting-ID abrufen
    QSqlQuery lookupQuery (db);
    lookupQuery.prepare ("SELECT id FROM besprechungen WHERE titel = :titel");
    lookupQuery.bindValue (":titel", name);

    if (!lookupQuery.exec () || !lookupQuery.next ())
    {
        saveTranscription ();
        return;
    }

    int meetingId = lookupQuery.value (0).toInt ();

    // Schritt 2: Metadaten der Besprechung aktualisieren
    QSqlQuery updateMeeting (db);
    updateMeeting.prepare ("UPDATE besprechungen SET created_at = :created_at WHERE id = :id");
    updateMeeting.bindValue (":created_at", startTime);
    updateMeeting.bindValue (":id", meetingId);
    if (!updateMeeting.exec ())
    {
        qWarning ()
            << "❌ Fehler beim Aktualisieren des Meetings:"
            << updateMeeting.lastError ().text ();
        return;
    }

    // Schritt 3: Alte Aussagen für diese Besprechung löschen
    QSqlQuery deleteQuery (db);
    deleteQuery.prepare ("DELETE FROM aussagen WHERE besprechungen_id = :id");
    deleteQuery.bindValue (":id", meetingId);
    if (!deleteQuery.exec ())
    {
        qWarning () << "❌ Fehler beim Löschen alter Aussagen:" << deleteQuery.lastError ().text ();
        return;
    }

    // Schritt 4: Vorhandene Sprechernamen und IDs laden
    QMap<QString, int> speakerCache;
    QSqlQuery speakerLoad (db);
    if (speakerLoad.exec ("SELECT id, name FROM sprecher"))
    {
        while (speakerLoad.next ())
        {
            int id = speakerLoad.value ("id").toInt ();
            QString name = speakerLoad.value ("name").toString ();
            speakerCache[name] = id;
        }
    }

    // Schritt 5: Aktualisierte Anweisungen erneut einfügen
    QSqlQuery insertQuery (db);
    QString targetColumn = m_script->isEdited () ? "verarbeiteter_text" : "roher_text";

    insertQuery.prepare (QString (R"(
        INSERT INTO aussagen (
            besprechungen_id,
            zeit_start,
            zeit_ende,
            %1,
            sprecher_id
        ) VALUES (
            :besprechungen_id,
            :zeit_start,
            :zeit_ende,
            :text,
            :sprecher_id
        )
    )")
                             .arg (targetColumn));

    for (const MetaText &segment : m_script->getMetaTexts ())
    {
        QString speakerName = segment.Speaker.trimmed ();
        int speakerId = -1;

        // Vorhandenen Sprechernamen aktualisieren oder neuen einfügen
        if (speakerCache.contains (speakerName))
        {
            speakerId = speakerCache[speakerName];
        }
        else if (!speakerName.isEmpty ())
        {
            // Try to insert new speaker
            QSqlQuery insertSpeaker (db);
            insertSpeaker.prepare (
                R"(INSERT INTO sprecher (name, besprechungen_id) VALUES (:name, :besprechungen_id) RETURNING id)");
            insertSpeaker.bindValue (":name", speakerName);
            insertSpeaker.bindValue (":besprechungen_id", meetingId);
            if (insertSpeaker.exec () && insertSpeaker.next ())
            {
                speakerId = insertSpeaker.value (0).toInt ();
                speakerCache[speakerName] = speakerId;
            }
            else
            {
                qWarning ()
                    << "Sprecher konnte nicht hinzugefügt werden:"
                    << insertSpeaker.lastError ().text ();
            }
        }

        QDateTime start = QDateTime::fromSecsSinceEpoch (segment.Start.toLongLong ());
        QDateTime end = QDateTime::fromSecsSinceEpoch (segment.End.toLongLong ());

        insertQuery.bindValue (":besprechungen_id", meetingId);
        insertQuery.bindValue (":zeit_start", start);
        insertQuery.bindValue (":zeit_ende", end);
        insertQuery.bindValue (":text", segment.Text);
        if (speakerId != -1)
        {
            insertQuery.bindValue (":sprecher_id", speakerId);
        }
        else
        {
            insertQuery.bindValue (":sprecher_id", QVariant ());
        }

        if (!insertQuery.exec ())
        {
            qWarning () << "INSERT fehlgeschlagen:" << insertQuery.lastError ().text ();
        }
    }

    QMessageBox::information (this, tr ("Erfolg"), tr ("Transkript erfolgreich aktualisiert."));
}

//--------------------------------------------------------------------------------------------------

void MainWindow::saveTranscription ()
{
    QSqlDatabase db = QSqlDatabase::database ("supabase");
    if (!db.isOpen ())
    {
        qWarning () << "Datenbank ist nicht geöffnet!";
        return;
    }

    QString newTitle = QInputDialog::getText (this,
                                              tr ("Neuer Titel"),
                                              tr ("Geben Sie einen neuen Meeting-Titel ein:"));
    if (newTitle.trimmed ().isEmpty ())
    {
        return;
    }

    if (!m_databaseManager->saveNewTranscription (m_script, newTitle))
    {
        QMessageBox::warning (this, "Fehler", "Transkript konnte nicht gespeichert werden.");
        return;
    }

    m_script->setName (newTitle);
    meetingList->addItem (newTitle);
    m_transcriptions[newTitle] = m_script;

    QMessageBox::information (this, tr ("Gespeichert"), tr ("Neues Transkript gespeichert."));
}

//--------------------------------------------------------------------------------------------------

int MainWindow::getOrInsertSpeakerId (
    const QString &name, QSqlDatabase &db)
{
    QSqlQuery query (db);
    query.prepare ("SELECT id FROM sprecher WHERE name = :name");
    query.bindValue (":name", name);

    if (query.exec () && query.next ())
    {
        return query.value (0).toInt ();
    }

    QSqlQuery insert (db);
    insert.prepare ("INSERT INTO sprecher (name) VALUES (:name) RETURNING id");
    insert.bindValue (":name", name);
    if (insert.exec () && insert.next ())
    {
        return insert.value (0).toInt ();
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------

void MainWindow::loadTranscriptionFromJson ()
{
    //  Öffnet einen Standard-Dateidialog, um eine JSON-Datei auszuwählen.
    QString path = QFileDialog::getOpenFileName (this,
                                                 tr ("Transkript laden"),
                                                 QDir::homePath (),
                                                 "*.json");
    if (path.isEmpty ())
    {
        return; //  Benutzer hat den Dialog abgebrochen.
    }

    bool ok;
    QJsonDocument doc = m_fileManager->loadJson (path, ok);
    if (!ok)
    {
        QMessageBox::warning (this,
                              tr ("Fehler"),
                              tr ("Datei konnte nicht gelesen oder geparst werden."));
        return;
    }

    if (m_script->fromJson (doc.toJson ()))
    {
        //  Versucht, Name und Datum aus dem Dateinamen zu extrahieren,
        //  falls dieser dem Standardformat "Name - Datum" entspricht.
        QString baseName = QFileInfo (path).completeBaseName ();
        QStringList baseNameList = baseName.split (" - ");
        if (baseNameList.size () > 1)
        {
            m_currentMeetingName = baseNameList[0];
            m_currentMeetingDateTime = baseNameList[1];
        }
        else
        {
            m_currentMeetingName = baseName;
        }

        //  Aktualisiert die UI mit den neu geladenen Daten.
        updateUiForCurrentMeeting ();
    }
}

//--------------------------------------------------------------------------------------------------

/*
void MainWindow::saveTranscriptionToJson ()
{
    //  ToDo: speichern, falls Pfad vorhanden, ansonsten saveTranscriptionToJsonAs
    return;
}
*/

//--------------------------------------------------------------------------------------------------

void MainWindow::saveTranscriptionToJsonAs ()
{
    //  Öffnet einen Standard-Dateidialog, um einen Speicherort und Dateinamen zu wählen.
    QString path = QFileDialog::getSaveFileName (this,
                                                 tr ("Transkript speichern unter"),
                                                 QDir::homePath (),
                                                 "*.json");
    if (path.isEmpty ())
    {
        return; //  Benutzer hat den Dialog abgebrochen.
    }

    m_fileManager->saveJson (path, m_script->toJson ());
}

//--------------------------------------------------------------------------------------------------

void MainWindow::restoreOriginalTranscription ()
{
    QListWidgetItem *item = meetingList->currentItem ();
    if (!item)
    {
        return;
    }

    QString meetingTitle = item->text ();
    loadMeetingTranscription (meetingTitle, "roher_text");
    updateTranscriptStatusAnzeige (m_script->getViewMode ());
}

//--------------------------------------------------------------------------------------------------

void MainWindow::loadMeetingTranscription (
    const QString &meetingTitle, const QString &textColumn)
{
    // Transkript aus der Datenbank laden
    QSqlDatabase db = QSqlDatabase::database ("supabase");
    if (!db.isOpen ())
    {
        qWarning () << "Supabase-Datenbank ist nicht geöffnet!";
        return;
    }

    QSqlQuery query (db);
    query.prepare ("SELECT id, created_at FROM besprechungen WHERE titel = :titel");
    query.bindValue (":titel", meetingTitle);

    if (!query.exec () || !query.next ())
    {
        QMessageBox::warning (this, tr ("Fehler"), tr ("Meeting konnte nicht gefunden werden."));
        return;
    }

    int meetingId = query.value ("id").toInt ();
    QDateTime startTime = query.value ("created_at").toDateTime ();

    m_script->clear ();
    m_script->setName (meetingTitle);
    m_script->setDateTime (startTime);

    QSqlQuery stmtQuery (db);
    stmtQuery.prepare (QString (R"(
        SELECT id, zeit_start, zeit_ende, %1, sprecher_id
        FROM aussagen
        WHERE besprechungen_id = :id
        ORDER BY zeit_start
    )")
                           .arg (textColumn));
    stmtQuery.bindValue (":id", meetingId);

    if (!stmtQuery.exec ())
    {
        QMessageBox::warning (this, tr ("Fehler"), tr ("Aussagen konnten nicht geladen werden."));
        return;
    }
    // Flag to check whether any valid text was found
    bool hasText = false;
    while (stmtQuery.next ())
    {
        QString text = stmtQuery.value (textColumn).toString ();
        if (!text.isEmpty ())
        {
            hasText = true;
        }

        QDateTime start = stmtQuery.value ("zeit_start").toDateTime ();
        QDateTime end = stmtQuery.value ("zeit_ende").toDateTime ();
        int speakerId = stmtQuery.value ("sprecher_id").toInt ();

        QString speakerName;
        QSqlQuery speakerQuery (db);
        speakerQuery.prepare (R"(
            SELECT name FROM sprecher
            WHERE id = :id AND besprechungen_id = :besprechungen_id
        )");
        speakerQuery.bindValue (":id", speakerId);
        speakerQuery.bindValue (":besprechungen_id", meetingId);

        if (speakerQuery.exec () && speakerQuery.next ())
        {
            speakerName = speakerQuery.value (0).toString ();
        }
        else
        {
            speakerName = tr ("Unbekannt");
        }

        QString startSec = QString::number (start.toSecsSinceEpoch ());
        QString endSec = QString::number (end.toSecsSinceEpoch ());

        MetaText segment (startSec, endSec, speakerName, text);
        m_script->add (segment);
    }
    // Handle fallback after loop
    if (!hasText && textColumn == "verarbeiteter_text")
    {
        qWarning () << "⚠️ Kein bearbeiteter Text gefunden, lade Original.";
        loadMeetingTranscription (meetingTitle, "roher_text");
        return;
    }

    //  Setzt den Undo/Redo-Verlauf zurück, da wir einen neuen Ausgangszustand haben.
    m_undoStack.clear ();
    m_redoStack.clear ();
    m_undoStack.push (m_script->toJson ());
    updateUndoRedoState ();
    updateUiForCurrentMeeting ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::updateUndoRedoState ()
{
    //  Aktiviert oder deaktiviert die Menü-Aktionen basierend darauf,
    //  ob die jeweiligen Stapel leer sind.
    m_undoAction->setEnabled (!m_undoStack.isEmpty ());
    m_redoAction->setEnabled (!m_redoStack.isEmpty ());
}

//--------------------------------------------------------------------------------------------------

void MainWindow::setMeetingName (
    const QString &name)
{
    //  Aktualisiert den Meeting-Namen an allen relevanten Stellen:
    //  1. Im internen Zustand der MainWindow.
    m_currentMeetingName = name;

    //  2. Im Datenmodell.
    m_script->setName (name);

    //  3. In der UI-Anzeige.
    nameLabel->setText (currentName ());
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onSetMeetingName ()
{
    //  Öffnet einen einfachen Text-Eingabedialog, um den Benutzer nach einem Namen zu fragen.
    QString currentName = m_script->name ();
    QString name = QInputDialog::getText (this,
                                          tr ("Meetingname setzen"),
                                          tr ("Name des Meetings:"),
                                          QLineEdit::Normal,
                                          currentName);

    //  Nur wenn der Benutzer eine nicht-leere Eingabe gemacht hat, wird der Name gesetzt.
    if (!name.trimmed ().isEmpty ())
    {
        setMeetingName (name.trimmed ());
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onReinstallPython ()
{
    //  Fragt den Benutzer zur Sicherheit nach, bevor die potenziell
    //  langwierige und destruktive Operation gestartet wird.
    QMessageBox::StandardButton reply;
    reply
        = QMessageBox::question (this,
                                 tr ("Python Neu-Installation"),
                                 tr ("Möchten Sie die Python-Umgebung wirklich neu installieren? "
                                     "Dies wird die aktuelle Umgebung löschen und neu aufsetzen."),
                                 QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        //  Eine temporäre Instanz des Managers wird erstellt, um die Aufgabe auszuführen.
        PythonEnvironmentManager pythonManager;

        //  Ruft die Setup-Methode mit `forceReinstall = true` auf.
        //  `this` wird als Eltern-Widget übergeben, damit der Installationsdialog
        //  korrekt über dem Hauptfenster zentriert wird.
        if (pythonManager.checkAndSetup (true, this))
        {
            QMessageBox::information (
                this,
                tr ("Erfolgreich"),
                tr ("Die Python-Umgebung wurde erfolgreich neu installiert."));
        }
        else
        {
            QMessageBox::warning (
                this,
                tr ("Fehler"),
                tr ("Die Python-Neuinstallation konnte nicht abgeschlossen werden. Bitte "
                    "überprüfen Sie die Ausgabe im Installationsfenster."));
        }
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::toggleTranscriptionVersion ()
{
    if (!meetingList->currentItem ())
    {
        return;
    }

    QString meetingTitle = meetingList->currentItem ()->text ();

    // Umschalten zwischen Original und Bearbeitet
    TranscriptionViewMode newMode;
    QString labelText, column, color;

    if (m_script->getViewMode () == TranscriptionViewMode::Original)
    {
        newMode = TranscriptionViewMode::Edited;
        labelText = "Anzeige: Bearbeitetes Transkript";
        column = "verarbeiteter_text";
        color = "orange";
    }
    else
    {
        newMode = TranscriptionViewMode::Original;
        labelText = "Anzeige: Originales Transkript";
        column = "roher_text";
        color = "green";
    }

    // Modus setzen
    m_script->setViewMode (newMode);
    transkriptStatusLabel->setText (labelText);
    transkriptStatusLabel->setStyleSheet ("font-weight: bold; color: " + color + ";");

    // Transkript laden mit Fallback bei leeren bearbeiteten Inhalten
    loadMeetingTranscription (meetingTitle, column);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::openMultiSearchDialog ()
{
    // Dialog bei Bedarf erstellen
    if (!m_multiSearchDialog)
    {
        m_multiSearchDialog = new MultiSearchDialog (this);
    }
    // Alle geladenen Transkripte dem Dialog übergeben
    m_multiSearchDialog->setTranscriptionsMap (m_transcriptions);
    // Mehrere Verbindungen vermeiden
    disconnect (m_multiSearchDialog, nullptr, this, nullptr);
    // Verbindung: Wenn ein Suchtreffer gewählt wurde
    connect (m_multiSearchDialog,
             &MultiSearchDialog::searchResultSelected,
             this,
             [=] (const QString &matchedText, const QString &meetingName)
             {
                 selectMeetingInList (meetingName);
                 loadMeetingTranscription (meetingName, "verarbeiteter_text");
                 highlightMatchedText (matchedText);
             });
    // Dialog modal anzeigen
    m_multiSearchDialog->exec ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::updateTranscriptStatusAnzeige (
    TranscriptionViewMode newMode)
{
    QString labelText, color;
    // Anzeigemodus prüfen und Beschriftung sowie Button-Text entsprechend setzen
    if (m_script->getViewMode () == TranscriptionViewMode::Edited)
    {
        labelText = "Anzeige: Bearbeitetes Transkript";
        toggleButton->setText ("Original anzeigen");
        color = "orange";
    }
    else
    {
        labelText = "Anzeige: Originales Transkript";
        toggleButton->setText ("Bearbeitet anzeigen");
        color = "green";
    }
    // Statuslabel im UI aktualisieren
    transkriptStatusLabel->setText (labelText);
    transkriptStatusLabel->setStyleSheet ("font-weight: bold; color: " + color + ";");
}

//--------------------------------------------------------------------------------------------------

void MainWindow::highlightMatchedText (
    const QString &text)
{
    // Keine Aktion, wenn View oder Text leer
    if (!transcriptView || text.trimmed ().isEmpty ())
    {
        return;
    }

    QTextDocument *doc = transcriptView->document ();
    QTextCursor highlightCursor (doc);

    // Formatierung für Markierung definieren
    QTextCharFormat highlightFormat;

    //  Nutze Systemfarben anstatt selbst definierte
    //highlightFormat.setBackground (QColor (0x44, 0x44, 0xFF)); // hell blau
    //highlightFormat.setForeground (Qt::white);
    auto pal = QApplication::palette ();
    highlightFormat.setBackground (pal.accent ());
    highlightFormat.setForeground (pal.highlightedText ());

    bool found = false;
    // Alle Vorkommen des Texts markieren
    while (!(highlightCursor = doc->find (text, highlightCursor)).isNull ())
    {
        highlightCursor.mergeCharFormat (highlightFormat);
        found = true;
    }
    // Cursor zur ersten Fundstelle setzen und sichtbar machen
    if (found)
    {
        QTextCursor firstCursor = doc->find (text);
        if (!firstCursor.isNull ())
        {
            transcriptView->setTextCursor (firstCursor);
            transcriptView->ensureCursorVisible ();
            transcriptView->setFocus ();
        }
    }
    else
    {
        qDebug () << "Keine Treffer:" << text;
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::selectMeetingInList (
    const QString &meetingName)
{
    // Durchlaufe die Meetingliste und wähle den passenden Eintrag aus
    for (int i = 0; i < meetingList->count (); ++i)
    {
        QListWidgetItem *item = meetingList->item (i);
        if (item && item->text () == meetingName)
        {
            meetingList->setCurrentItem (item); // Auswahl im UI setzen
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onMeetingSelected (
    QListWidgetItem *item)
{
    if (!item)
    {
        return;
    }

    QString meetingTitle = item->text ();

    // Modus automatisch festlegen basierend auf Bearbeitungsstatus
    TranscriptionViewMode viewMode = m_script->isEdited () ? TranscriptionViewMode::Edited
                                                           : TranscriptionViewMode::Original;

    m_script->setViewMode (viewMode);

    QString column = (viewMode == TranscriptionViewMode::Edited) ? "verarbeiteter_text"
                                                                 : "roher_text";

    loadMeetingTranscription (meetingTitle, column);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onStartClicked ()
{
    //  1. UI für den Aufnahme-Zustand vorbereiten (Buttons de-/aktivieren).
    startButton->setEnabled (false);
    stopButton->setEnabled (true);
    saveAudioButton->setEnabled (false);
    savePDFButton->setEnabled (false);
    assignNamesButton->setEnabled (false);
    editTextButton->setEnabled (false);
    generateTagsButton->setEnabled (false);

    //  2. Eventuell laufende Hintergrundprozesse der vorherigen Aufnahme beenden.
    m_asrManager->stop ();

    //  3. Datenmodell für die neue Aufnahme zurücksetzen.
    m_script->clear ();

    //  4. Threads für das Schreiben der .wav-Dateien und die Audio-Aufnahme starten.
    m_wavWriter->startWriting (m_fileManager->getTempWavPath (false),
                               m_fileManager->getTempWavPath (true));
    m_captureThread->startCapture ();

    //  5. Metadaten für das neue Meeting setzen.
    QDateTime dt = QDateTime::currentDateTime ();
    m_currentMeetingDateTime = dt.toString ("yyyy-MM-dd_HH-mm");
    if (m_currentMeetingName.isEmpty ())
    {
        m_currentMeetingName = "Aufnahme";
    }
    m_script->setName (m_currentMeetingName);
    m_script->setDateTime (dt);
    nameLabel->setText (currentName ());

    //  Poll-Timer starten, damit onPollTranscripts() regelmäßig aufgerufen wird
    //  Das ist nur zu Demonstrationszwecken bis die Echtzeit-Transkribierung erfolgt
    pollTimer->start (500);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onStopClicked ()
{
    //  Beendet alle Prozesse, die mit der aktiven Aufnahme zusammenhängen.
    pollTimer->stop ();
    m_captureThread->stopCapture ();
    timeUpdateTimer->stop ();

    //  Setzt die UI in den "gestoppt"-Zustand.
    stopButton->setEnabled (false);
    startButton->setEnabled (true);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onPollTranscripts ()
{
    static int count;
    QList<MetaText> bsp = {{timeLabel->text (), " ", "Sprecher 1", "Beispieltext"},
                           {timeLabel->text (), " ", "Sprecher 2", "Weiterer Text"},
                           {timeLabel->text (), " ", "Sprecher 3", "Bla bla bla."}};
    count = ((count + 1) % bsp.length ());

    m_script->add (bsp[count]);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onSaveAudio ()
{
    QString path = QFileDialog::getSaveFileName (this,
                                                 tr ("Audio speichern"),
                                                 QString (),
                                                 tr ("WAV-Datei (*.wav)"));
    if (path.isEmpty ())
    {
        return;
    }
    m_currentAudioPath = path;

    //  Das Kopieren der potenziell großen WAV-Datei wird in einen separaten Thread ausgelagert,
    //  um ein Einfrieren der Benutzeroberfläche zu verhindern.
    auto future = QtConcurrent::run (
        [=] ()
        {
            QFile::remove (path);
            return QFile::copy (m_fileManager->getTempWavPath (), path);
        });

    //  Ein QFutureWatcher wird verwendet, um auf das Ergebnis der asynchronen Operation zu reagieren.
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool> (this);
    connect (watcher,
             &QFutureWatcher<bool>::finished,
             this,
             [=] ()
             {
                 watcher->deleteLater ();
                 if (future.result ())
                 {
                     QMessageBox::information (this,
                                               tr ("Gespeichert"),
                                               tr ("Audio unter %1 gespeichert.").arg (path));
                     setStatus ("Audiodatei gespeichert");
                 }
                 else
                 {
                     QMessageBox::warning (this,
                                           tr ("Fehler"),
                                           tr ("Audio konnte nicht kopiert werden."));
                     setStatus ("Audiodatei konnte nicht gespeichert werden");
                 }
             });
    watcher->setFuture (future);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onSavePDF ()
{
    if (!m_script || m_script->getMetaTexts ().isEmpty ())
    {
        QMessageBox::information (this,
                                  tr ("Export nicht möglich"),
                                  tr ("Es gibt kein Transkript zum Exportieren."));
        return;
    }

    QString defaultFileName = QString ("%1.pdf").arg (currentName ());
    QString filePath = QFileDialog::getSaveFileName (this,
                                                     tr ("Transkript als PDF speichern"),
                                                     QDir::homePath () + "/" + defaultFileName,
                                                     tr ("PDF-Dateien (*.pdf)"));

    if (filePath.isEmpty ())
    {
        return; // Nutzer hat abgebrochen
    }

    TranscriptPdfExporter exporter (*m_script);

    setStatus (tr ("PDF wird erstellt..."), true);
    if (exporter.exportToPdf (filePath))
    {
        setStatus (tr ("PDF erfolgreich gespeichert."), false);

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question (
            this,
            tr ("Export erfolgreich"),
            tr ("Die PDF-Datei wurde erfolgreich gespeichert.\nMöchten Sie sie jetzt öffnen?"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            QDesktopServices::openUrl (QUrl::fromLocalFile (filePath));
        }
    }
    else
    {
        QMessageBox::warning (this,
                              tr ("Fehler"),
                              tr ("Die PDF-Datei konnte nicht erstellt oder gespeichert werden."));
        setStatus (tr ("PDF-Export fehlgeschlagen."), false);
    }
}

//--------------------------------------------------------------------------------------------------

QString MainWindow::currentName () const
{
    if (!m_script)
    {
        return "Kein Skript geladen";
    }

    QString name = m_script->name ();
    if (name.isEmpty ())
    {
        name = "Unbenannte Aufnahme";
    }

    auto dateTime = m_script->dateTime ();
    QString dt;
    if (dateTime.isValid ())
    {
        dt = dateTime.toString ("yyyy-MM-dd_HH-mm");
    }
    else
    {
        dt = "1919-09-19_19-19";
    }

    return QString ("%1 - %2").arg (name, dt);
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onEditSpeakers ()
{
    //  Dieser Block implementiert ein "Lazy Instantiation"-Pattern. Der Dialog wird
    //  nur dann erstellt, wenn er zum ersten Mal benötigt wird.
    if (!m_speakerEditorDialog)
    {
        //  1. Erstelle eine neue Instanz des Dialogs.
        m_speakerEditorDialog = new SpeakerEditorDialog (m_script, this);

        //  2. Verbinde das `finished`-Signal des Dialogs. Dies ist entscheidend für
        //     sauberes Speichermanagement bei nicht-modalen Dialogen.
        connect (m_speakerEditorDialog,
                 &QDialog::finished,
                 this,
                 [this] (int result)
                 {
                     //  `deleteLater()` stellt sicher, dass das Objekt sicher gelöscht wird,
                     //  nachdem alle ausstehenden Events verarbeitet wurden.
                     m_speakerEditorDialog->deleteLater ();

                     //  Der Zeiger wird auf nullptr zurückgesetzt, damit beim nächsten Klick
                     //  auf den Button eine neue Instanz erstellt werden kann.
                     m_speakerEditorDialog = nullptr;
                 });
    }

    //  3. Zeige den Dialog an. `show()` öffnet den Dialog, ohne den Codefluss zu blockieren.
    m_speakerEditorDialog->show ();

    //  4. Bringe das Fenster in den Vordergrund, falls es von anderen Fenstern verdeckt ist.
    m_speakerEditorDialog->raise ();

    //  5. Setze den Tastatur-Fokus auf das Dialogfenster.
    m_speakerEditorDialog->activateWindow ();
}

//--------------------------------------------------------------------------------------------------

void MainWindow::onGenerateTags ()
{
    if (!m_script || m_script->text ().isEmpty ())
    {
        QMessageBox::warning (this, "Fehler", "Es gibt keinen Text zum Analysieren.");
        return;
    }

    // Deaktiviere den Button, während die Analyse läuft
    generateTagsButton->setEnabled (false);
    setStatus ("Generiere Tags, bitte warten...", true);

    // Starte die Analyse
    m_tagGenerator->generateTagsFor (m_script->text ());
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

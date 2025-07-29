#include "speakereditordialog.h"

#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

SpeakerEditorDialog::SpeakerEditorDialog (
    Transcription* transcription, QWidget* parent)
    : QDialog (parent)
    , m_transcription (transcription)
    , m_tabWidget (new QTabWidget (this))
    , m_globalSpeakerTable (new QTableWidget (this))
    , m_segmentTable (new QTableWidget (this))
    , m_applyButton (new QPushButton (tr ("Anwenden"), this))
    , m_okButton (new QPushButton (tr ("OK"), this))
    , m_cancelButton (new QPushButton (tr ("Abbrechen"), this))
    , m_statusLabel (new QLabel (this))
    , m_statusTimer (new QTimer (this))
    , m_mergeNameEdit (new QLineEdit (this))
    , m_mergeSpeakersButton (new QPushButton (tr ("Sprecher zusammenfassen"), this))
{
    //  Konfiguriert den Dialog als nicht-modal (blockiert nicht das Hauptfenster).
    setWindowModality (Qt::NonModal);
    //  Sorgt dafür, dass der Dialog beim Schließen automatisch gelöscht wird, um Speicherlecks zu vermeiden.
    setAttribute (Qt::WA_DeleteOnClose);

    //  Konfiguration des Timers, der Status-Nachrichten nach 3 Sekunden ausblendet.
    m_statusTimer->setSingleShot (true);
    m_statusTimer->setInterval (3000);
    connect (m_statusTimer, &QTimer::timeout, m_statusLabel, &QLabel::hide);
    m_statusLabel->setAlignment (Qt::AlignCenter);
    m_statusLabel->setStyleSheet ("QLabel { color: blue; font-weight: bold; }");
    m_statusLabel->hide ();

    setupUI ();
    resize (1000, 700);

    //  Verbindet den Dialog mit dem Datenmodell, um auf Änderungen zu reagieren.
    if (m_transcription)
    {
        connect (m_transcription,
                 &Transcription::changed,
                 this,
                 &SpeakerEditorDialog::onTranscriptionChanged);
        //  Füllt den Dialog initial mit den Daten aus dem Transkript.
        onTranscriptionChanged ();
    }
    else
    {
        setDialogStatus (tr ("Fehler: Kein gültiges Transkriptions-Objekt übergeben."), false);
    }
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::setupUI ()
{
    setWindowTitle (tr ("Sprecher bearbeiten"));
    QVBoxLayout* mainLayout = new QVBoxLayout (this);

    //  Tab für die globale Bearbeitung von Sprechernamen.
    QWidget* globalTab = new QWidget (this);
    QVBoxLayout* globalLayout = new QVBoxLayout (globalTab);
    m_globalSpeakerTable->setColumnCount (2);
    m_globalSpeakerTable->setHorizontalHeaderLabels ({tr ("Erkannter Sprecher"), tr ("Neuer Name")});
    m_globalSpeakerTable->horizontalHeader ()->setStretchLastSection (true);
    globalLayout->addWidget (m_globalSpeakerTable);

    //  Layout für die "Sprecher zusammenfassen"-Funktion.
    QHBoxLayout* mergeLayout = new QHBoxLayout ();
    m_mergeNameEdit->setPlaceholderText (tr ("Neuer gemeinsamer Name für ausgewählte Sprecher..."));
    mergeLayout->addWidget (m_mergeNameEdit);
    mergeLayout->addWidget (m_mergeSpeakersButton);
    globalLayout->addLayout (mergeLayout);
    m_tabWidget->addTab (globalTab, tr ("Globale Sprecher"));

    //  Tab für die Bearbeitung der Sprecher einzelner Segmente.
    QWidget* segmentTab = new QWidget (this);
    QVBoxLayout* segmentLayout = new QVBoxLayout (segmentTab);
    m_segmentTable->setColumnCount (5);
    m_segmentTable->setHorizontalHeaderLabels (
        {tr ("Start"), tr ("Ende"), tr ("Sprecher"), tr ("Text"), tr ("Neuer Sprecher")});
    m_segmentTable->horizontalHeader ()->setStretchLastSection (true);
    segmentLayout->addWidget (m_segmentTable);
    m_tabWidget->addTab (segmentTab, tr ("Abschnitt-Sprecher"));

    mainLayout->addWidget (m_tabWidget);
    mainLayout->addWidget (m_statusLabel);

    //  Layout für die Buttons am unteren Rand des Dialogs.
    QHBoxLayout* buttonLayout = new QHBoxLayout ();
    buttonLayout->addStretch ();
    buttonLayout->addWidget (m_applyButton);
    buttonLayout->addWidget (m_okButton);
    buttonLayout->addWidget (m_cancelButton);
    mainLayout->addLayout (buttonLayout);

    connect (m_applyButton,
             &QPushButton::clicked,
             this,
             &SpeakerEditorDialog::handleApplyOkButtonClicked);
    connect (m_okButton,
             &QPushButton::clicked,
             this,
             &SpeakerEditorDialog::handleApplyOkButtonClicked);
    connect (m_cancelButton,
             &QPushButton::clicked,
             this,
             &SpeakerEditorDialog::handleCancelButtonClicked);
    connect (m_mergeSpeakersButton,
             &QPushButton::clicked,
             this,
             &SpeakerEditorDialog::onMergeSpeakersClicked);
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::onTranscriptionChanged ()
{
    //  Wenn sich das zugrundeliegende Datenmodell ändert, wird die gesamte Anzeige neu aufgebaut.
    if (!m_transcription)
    {
        return;
    }
    updateKnownSpeakers ();
    populateGlobalSpeakerTable ();
    populateSegmentTable ();
    setDialogStatus (tr ("Transkription aktualisiert."), true);
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::updateKnownSpeakers ()
{
    //  Sammelt alle einzigartigen Sprechernamen aus dem Transkript in einem Set.
    m_allKnownSpeakers.clear ();
    if (m_transcription)
    {
        for (const MetaText& metaText : m_transcription->getMetaTexts ())
        {
            m_allKnownSpeakers.insert (metaText.Speaker);
        }
    }
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::populateGlobalSpeakerTable ()
{
    //  Befüllt die Tabelle, in der jeder einzigartige Sprecher einmal auftaucht.
    m_globalSpeakerTable->blockSignals (
        true); //  Verhindert, dass Signale während des Befüllens ausgelöst werden.
    m_globalSpeakerTable->clearContents ();
    m_globalSpeakerTable->setRowCount (0);
    if (!m_transcription)
    {
        return;
    }

    QList<QString> sorted = m_allKnownSpeakers.values ();
    std::sort (sorted.begin (), sorted.end ());
    m_globalSpeakerTable->setRowCount (sorted.size ());
    m_currentGlobalNames.clear ();

    for (int i = 0; i < sorted.size (); ++i)
    {
        QString speaker = sorted.at (i);
        //  Die erste Spalte zeigt den originalen Sprechernamen und ist nicht editierbar.
        m_globalSpeakerTable->setItem (i, 0, new QTableWidgetItem (speaker));
        m_globalSpeakerTable->item (i, 0)->setFlags (Qt::ItemIsEnabled);

        //  Die zweite Spalte enthält ein Eingabefeld zur Änderung des Namens.
        QLineEdit* nameEdit = new QLineEdit (speaker, this);
        nameEdit->setProperty (
            "row", i); //  Speichert die Zeilennummer im Widget, um sie im Slot wiederzufinden.
        connect (nameEdit,
                 &QLineEdit::textChanged,
                 this,
                 &SpeakerEditorDialog::onGlobalSpeakerNameChanged);
        m_globalSpeakerTable->setCellWidget (i, 1, nameEdit);

        //  Initialisiert den Puffer für Namensänderungen.
        m_currentGlobalNames.insert (speaker, speaker);
    }
    m_globalSpeakerTable->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::populateSegmentTable ()
{
    //  Befüllt die Tabelle, die jedes einzelne Textsegment anzeigt.
    m_segmentTable->blockSignals (true);
    m_segmentTable->clearContents ();
    m_segmentTable->setRowCount (0);
    if (!m_transcription)
    {
        return;
    }

    const QList<MetaText>& metaTexts = m_transcription->getMetaTexts ();
    m_segmentTable->setRowCount (metaTexts.size ());
    m_currentSegmentNames.clear ();

    for (int i = 0; i < metaTexts.size (); ++i)
    {
        const MetaText& mt = metaTexts.at (i);
        //  Füllt die ersten vier Spalten mit den Segment-Daten. Diese sind nicht direkt editierbar.
        m_segmentTable->setItem (i, 0, new QTableWidgetItem (mt.Start));
        m_segmentTable->setItem (i, 1, new QTableWidgetItem (mt.End));
        m_segmentTable->setItem (i, 2, new QTableWidgetItem (mt.Speaker));
        m_segmentTable->setItem (i, 3, new QTableWidgetItem (mt.Text));
        for (int col = 0; col <= 3; ++col)
        {
            m_segmentTable->item (i, col)->setFlags (Qt::ItemIsEnabled);
        }

        //  Die fünfte Spalte enthält eine ComboBox, um den Sprecher für dieses Segment zu ändern.
        QComboBox* combo = new QComboBox (this);
        combo->setEditable (true); //  Erlaubt die Eingabe neuer Sprechernamen.
        combo->addItems (m_allKnownSpeakers.values ());
        int idx = combo->findText (mt.Speaker);
        combo->setCurrentIndex (idx);
        combo->setProperty ("row", i);
        connect (combo,
                 QOverload<int>::of (&QComboBox::currentIndexChanged),
                 this,
                 &SpeakerEditorDialog::onSegmentSpeakerChanged);
        m_segmentTable->setCellWidget (i, 4, combo);

        //  Initialisiert den Puffer für Segment-Änderungen.
        m_currentSegmentNames.insert ({mt.Start, mt.End}, mt.Speaker);
    }
    m_segmentTable->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::handleApplyOkButtonClicked ()
{
    //  Wendet die Änderungen des aktuell sichtbaren Tabs an.
    applyCurrentTabChanges ();

    //  Wenn der OK-Button geklickt wurde, wird der Dialog zusätzlich geschlossen.
    if (sender () == m_okButton)
    {
        accept ();
    }
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::handleCancelButtonClicked ()
{
    //  Schließt den Dialog, ohne Änderungen zu übernehmen.
    reject ();
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::applyCurrentTabChanges ()
{
    if (!m_transcription)
    {
        setDialogStatus (tr ("Fehler: Transkriptions-Objekt nicht verfügbar."), false);
        return;
    }
    //  beginBatchUpdate() verhindert, dass bei jeder einzelnen Änderung ein 'changed'-Signal gesendet wird.
    m_transcription->beginBatchUpdate ();
    bool changed = false;

    if (m_tabWidget->currentIndex () == 0) //  Globaler Tab ist aktiv
    {
        for (int i = 0; i < m_globalSpeakerTable->rowCount (); ++i)
        {
            QString oldName = m_globalSpeakerTable->item (i, 0)->text ();
            QLineEdit* edit = qobject_cast<QLineEdit*> (m_globalSpeakerTable->cellWidget (i, 1));
            QString newName = edit ? edit->text ().trimmed () : oldName;

            if (oldName != newName && m_transcription->changeSpeaker (oldName, newName))
            {
                changed = true;
            }
        }
        setDialogStatus (tr ("Globale Änderungen angewendet."), true);
    }
    else //  Segment-Tab ist aktiv
    {
        for (int i = 0; i < m_segmentTable->rowCount (); ++i)
        {
            QString start = m_segmentTable->item (i, 0)->text ();
            QString end = m_segmentTable->item (i, 1)->text ();
            QString oldSpeaker = m_segmentTable->item (i, 2)->text ();
            QComboBox* combo = qobject_cast<QComboBox*> (m_segmentTable->cellWidget (i, 4));
            QString newSpeaker = combo ? combo->currentText ().trimmed () : oldSpeaker;

            if (!newSpeaker.isEmpty () && newSpeaker != oldSpeaker)
            {
                if (m_transcription->changeSpeakerForSegment (start, end, newSpeaker))
                {
                    changed = true;
                }
            }
        }
        setDialogStatus (tr ("Abschnitts-Änderungen angewendet."), true);
    }
    //  endBatchUpdate() wendet alle Änderungen an und sendet ein einziges 'changed'-Signal, falls nötig.
    m_transcription->endBatchUpdate ();
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::setDialogStatus (
    const QString& text, bool temporary)
{
    m_statusLabel->setText (text);
    m_statusLabel->show ();
    if (temporary)
    {
        m_statusTimer->start ();
    }
    else
    {
        m_statusTimer->stop ();
    }
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::onSegmentSpeakerChanged (
    int index)
{
    //  Dieser Slot wird aufgerufen, wenn der Benutzer in der Segment-Tabelle einen Sprecher ändert.
    QComboBox* combo = qobject_cast<QComboBox*> (sender ());
    if (!combo)
    {
        return;
    }

    //  Die Änderungen werden nur im Puffer (m_currentSegmentNames) gespeichert,
    //  nicht direkt im Datenmodell.
    int row = combo->property ("row").toInt ();
    QString newSpeaker = combo->itemText (index);
    if (row >= 0 && row < m_segmentTable->rowCount ())
    {
        QString start = m_segmentTable->item (row, 0)->text ();
        QString end = m_segmentTable->item (row, 1)->text ();
        m_currentSegmentNames[{start, end}] = newSpeaker;
    }
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::onMergeSpeakersClicked ()
{
    //  Fasst alle in der globalen Tabelle ausgewählten Sprecher unter einem neuen Namen zusammen.
    QString newName = m_mergeNameEdit->text ().trimmed ();
    if (newName.isEmpty ())
    {
        return;
    }

    const auto ranges = m_globalSpeakerTable->selectedRanges ();
    for (const auto& range : ranges)
    {
        for (int row = range.topRow (); row <= range.bottomRow (); ++row)
        {
            //  Aktualisiert direkt das Eingabefeld in der Tabelle.
            if (auto* edit = qobject_cast<QLineEdit*> (m_globalSpeakerTable->cellWidget (row, 1)))
            {
                edit->setText (newName);
            }
        }
    }
    setDialogStatus (tr ("Sprecher zusammengefasst. Bitte 'Anwenden' klicken."), true);
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::onGlobalSpeakerNameChanged (
    const QString& text)
{
    //  Dieser Slot wird aufgerufen, wenn der Benutzer in der globalen Tabelle einen Namen ändert.
    QLineEdit* edit = qobject_cast<QLineEdit*> (sender ());
    if (edit)
    {
        //  Die Änderungen werden nur im Puffer (m_currentGlobalNames) gespeichert.
        int row = edit->property ("row").toInt ();
        QString oldName = m_globalSpeakerTable->item (row, 0)->text ();
        m_currentGlobalNames.insert (oldName, text.trimmed ());
    }
}

//--------------------------------------------------------------------------------------------------

void SpeakerEditorDialog::setSelectedSegment (
    const QString& start, const QString& end)
{
    //  Diese Methode wird extern aufgerufen, um ein bestimmtes Segment in der Tabelle zu markieren.
    m_selectedSegmentStart = start;
    m_selectedSegmentEnd = end;

    //  Sucht die passende Zeile und scrollt dorthin.
    for (int i = 0; i < m_segmentTable->rowCount (); ++i)
    {
        if (m_segmentTable->item (i, 0)->text () == start
            && m_segmentTable->item (i, 1)->text () == end)
        {
            m_tabWidget->setCurrentIndex (1); //  Zum Segment-Tab wechseln
            m_segmentTable->selectRow (i);
            m_segmentTable->scrollToItem (m_segmentTable->item (i, 0),
                                          QAbstractItemView::PositionAtCenter);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

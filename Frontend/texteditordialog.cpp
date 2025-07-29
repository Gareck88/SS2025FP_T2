#include "texteditordialog.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

TextEditorDialog::TextEditorDialog (
    Transcription* transcription, QWidget* parent)
    : QDialog (parent)
    , m_transcription (transcription)
    , m_table (new QTableWidget (this))
    , m_applyButton (new QPushButton (tr ("Anwenden"), this))
    , m_okButton (new QPushButton (tr ("OK"), this))
    , m_cancelButton (new QPushButton (tr ("Abbrechen"), this))
    , m_statusLabel (new QLabel (this))
    , m_statusTimer (new QTimer (this))
{
    //  Konfiguriert den Dialog als nicht-modal (blockiert nicht das Hauptfenster)
    //  und sorgt dafür, dass er beim Schließen automatisch gelöscht wird.
    setWindowModality (Qt::NonModal);
    setAttribute (Qt::WA_DeleteOnClose);

    //  Konfiguration des Timers für die Status-Nachrichten.
    m_statusTimer->setSingleShot (true);
    m_statusTimer->setInterval (3000);
    connect (m_statusTimer, &QTimer::timeout, m_statusLabel, &QLabel::hide);
    m_statusLabel->setAlignment (Qt::AlignCenter);
    m_statusLabel->setStyleSheet ("QLabel { color: blue; font-weight: bold; }");
    m_statusLabel->hide ();

    setupUI ();
    resize (1000, 700);

    //  Verbindet den Dialog mit dem Datenmodell, um auf externe Änderungen zu reagieren.
    if (m_transcription)
    {
        connect (m_transcription,
                 &Transcription::changed,
                 this,
                 &TextEditorDialog::onTranscriptionChanged);

        //  Füllt den Dialog initial mit Daten.
        onTranscriptionChanged ();
    }
    else
    {
        setDialogStatus (tr ("Fehler: Kein gültiges Transkriptions-Objekt übergeben."), false);
    }
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::setupUI ()
{
    setWindowTitle (tr ("Transkriptionstext bearbeiten"));
    QVBoxLayout* mainLayout = new QVBoxLayout ();

    //  Konfiguration der Haupttabelle.
    m_table->setColumnCount (4);
    m_table->setHorizontalHeaderLabels ({tr ("Start"), tr ("Ende"), tr ("Sprecher"), tr ("Text")});
    m_table->horizontalHeader ()->setStretchLastSection (
        true); //  Die Text-Spalte füllt den Rest des Platzes.
    m_table->setEditTriggers (QAbstractItemView::AllEditTriggers);
    m_table->setSelectionMode (QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior (QAbstractItemView::SelectRows);
    connect (m_table, &QTableWidget::itemChanged, this, &TextEditorDialog::onTextItemChanged);

    mainLayout->addWidget (m_table);
    mainLayout->addWidget (m_statusLabel);

    //  Layout für die Buttons am unteren Rand des Dialogs.
    QHBoxLayout* buttonLayout = new QHBoxLayout ();
    buttonLayout->addStretch ();
    buttonLayout->addWidget (m_applyButton);
    buttonLayout->addWidget (m_okButton);
    buttonLayout->addWidget (m_cancelButton);

    connect (m_applyButton,
             &QPushButton::clicked,
             this,
             &TextEditorDialog::handleApplyButtonClicked);
    connect (m_okButton, &QPushButton::clicked, this, &TextEditorDialog::handleOkButtonClicked);
    connect (m_cancelButton,
             &QPushButton::clicked,
             this,
             &TextEditorDialog::handleCancelButtonClicked);

    mainLayout->addLayout (buttonLayout);
    this->setLayout (mainLayout);
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::onTranscriptionChanged ()
{
    //  Lädt die Tabelle neu, wenn sich das zugrundeliegende Datenmodell ändert.
    populateTable ();
    setDialogStatus (tr ("Transkription aktualisiert."), true);
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::populateTable ()
{
    //  Verhindert, dass während des programmatischen Befüllens der Tabelle
    //  das itemChanged-Signal für jede Zelle ausgelöst wird.
    m_table->blockSignals (true);

    m_table->clearContents ();
    m_pendingTextChanges.clear (); //  Puffer für Änderungen ebenfalls leeren.

    if (!m_transcription)
    {
        m_table->blockSignals (false);
        return;
    }

    const QList<MetaText>& list = m_transcription->getMetaTexts ();
    m_table->setRowCount (list.size ());

    for (int i = 0; i < list.size (); ++i)
    {
        const MetaText& mt = list.at (i);

        //  Die ersten drei Spalten sind nicht editierbar.
        auto* startItem = new QTableWidgetItem (mt.Start);
        startItem->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem (i, 0, startItem);

        auto* endItem = new QTableWidgetItem (mt.End);
        endItem->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem (i, 1, endItem);

        auto* speakerItem = new QTableWidgetItem (mt.Speaker);
        speakerItem->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_table->setItem (i, 2, speakerItem);

        //  Die Text-Zelle ist editierbar.
        auto* textItem = new QTableWidgetItem (mt.Text);
        //  Wir speichern den Originaltext in der UserRole, um später
        //  prüfen zu können, ob der Benutzer den Text tatsächlich geändert hat.
        textItem->setData (Qt::UserRole, mt.Text);
        m_table->setItem (i, 3, textItem);
    }

    m_table->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::onTextItemChanged (
    QTableWidgetItem* item)
{
    //  Dieser Slot reagiert nur auf Änderungen in der vierten Spalte (Text).
    if (!item || item->column () != 3)
    {
        return;
    }

    int row = item->row ();
    QString oldText = item->data (Qt::UserRole).toString ();
    QString newText = item->text ();

    if (newText != oldText)
    {
        //  Die Änderung wird nicht sofort übernommen, sondern in einer Map zwischengespeichert.
        //  Als eindeutiger Schlüssel für das Segment dient das Paar aus Start- und Endzeit.
        QString start = m_table->item (row, 0)->text ();
        QString end = m_table->item (row, 1)->text ();
        m_pendingTextChanges[{start, end}] = newText;
    }
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::applyChanges ()
{
    if (!m_transcription)
    {
        return;
    }

    bool changed = false;
    //  Verhindert, dass für jede Textänderung ein separates Signal ausgelöst wird.
    m_transcription->beginBatchUpdate ();

    //  Iteriert durch alle gepufferten Änderungen und wendet sie auf das Datenmodell an.
    for (auto it = m_pendingTextChanges.constBegin (); it != m_pendingTextChanges.constEnd (); ++it)
    {
        const QString& start = it.key ().first;
        const QString& end = it.key ().second;
        const QString& newText = it.value ();

        if (m_transcription->changeText (start, end, newText))
        {
            changed = true;
        }
    }

    m_pendingTextChanges.clear ();      //  Puffer leeren, nachdem die Änderungen übernommen wurden.
    m_transcription->endBatchUpdate (); //  Sendet ein einziges 'changed'-Signal, falls nötig.

    if (changed)
    {
        setDialogStatus (tr ("Textänderungen übernommen."), true);
    }
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::handleApplyButtonClicked ()
{
    applyChanges ();
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::handleOkButtonClicked ()
{
    applyChanges ();
    accept ();
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::handleCancelButtonClicked ()
{
    reject ();
}

//--------------------------------------------------------------------------------------------------

void TextEditorDialog::setDialogStatus (
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
//--------------------------------------------------------------------------------------------------

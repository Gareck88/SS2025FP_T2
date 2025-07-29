#include "searchdialog.h"
#include "transcription.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QTimeEdit>
#include <QSet>

SearchDialog::SearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Erweiterte Suche im Transkript");
    resize(500, 400);

    // GUI-Elemente initialisieren
    keywordInput = new QLineEdit(this);
    speakerFilter = new QComboBox(this);
    tagFilter = new QComboBox(this);
    startTimeEdit = new QTimeEdit(this);
    endTimeEdit = new QTimeEdit(this);
    searchButton = new QPushButton("Suchen", this);
    resultsList = new QListWidget(this);
    statusLabel = new QLabel(this);

    // Standardfilter-Werte setzen
    speakerFilter->addItem("Alle Sprecher");
    tagFilter->addItem("Alle Tags");

    // Zeit-Formate setzen
    startTimeEdit->setDisplayFormat("HH:mm:ss");
    endTimeEdit->setDisplayFormat("HH:mm:ss");
    startTimeEdit->setTime(QTime(0, 0, 0));
    endTimeEdit->setTime(QTime(23, 59, 59));

    // Layout für Eingabeform erstellen
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow("Schlüsselwort:", keywordInput);
    formLayout->addRow("Sprecher:", speakerFilter);
    formLayout->addRow("Tag:", tagFilter);
    formLayout->addRow("Startzeit:", startTimeEdit);
    formLayout->addRow("Endzeit:", endTimeEdit);

    // Gesamtlayout zusammenbauen
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(searchButton);
    mainLayout->addWidget(resultsList);
    mainLayout->addWidget(statusLabel);

    // Signal-Slot-Verbindungen
    connect(searchButton, &QPushButton::clicked, this, &SearchDialog::onSearchClicked);
    connect(resultsList, &QListWidget::itemDoubleClicked, this, &SearchDialog::onItemDoubleClicked);
}

//--------------------------------------------------------------------------------------------------

void SearchDialog::setTranscription(Transcription *transcript)
{
    m_transcription = transcript;
    loadSpeakerAndTagOptions();
}

//--------------------------------------------------------------------------------------------------

void SearchDialog::loadSpeakerAndTagOptions()
{
    if (!m_transcription) return;

    QSet<QString> speakers;
    QSet<QString> tags;

    // Alle Sprecher und Tags aus dem Transkript sammeln
    for (const MetaText &segment : m_transcription->getMetaTexts()) {
        speakers.insert(segment.Speaker);
        for (const QString &tag : segment.Tags)
            tags.insert(tag);
    }

    // Filterfelder füllen
    for (const QString &s : speakers)
        speakerFilter->addItem(s);

    for (const QString &tag : tags)
        tagFilter->addItem(tag);
}

//--------------------------------------------------------------------------------------------------

QTime SearchDialog::parseTimeFromSeconds(const QString &seconds) const
{
    bool ok;
    int sec = seconds.toInt(&ok);
    if (!ok)
        return QTime(); // Rückgabe einer ungültigen Zeit bei Fehler
    return QTime(0, 0, 0).addSecs(sec);
}

//--------------------------------------------------------------------------------------------------

void SearchDialog::performSearch()
{
    resultsList->clear(); // Vorherige Ergebnisse löschen
    if (!m_transcription) return;

    QString keyword = keywordInput->text().trimmed();
    QString selectedSpeaker = speakerFilter->currentText();
    QString selectedTag = tagFilter->currentText();
    QTime start = startTimeEdit->time();
    QTime end = endTimeEdit->time();

    int hits = 0;

    // Alle Transkript-Segmente durchgehen
    for (const MetaText &segment : m_transcription->getMetaTexts()) {
        QTime segmentTime = parseTimeFromSeconds(segment.Start);

        // Zeitfilter anwenden
        if (!segmentTime.isValid() || segmentTime < start || segmentTime > end)
            continue;

        // Sprecherfilter
        if (selectedSpeaker != "Alle Sprecher" && segment.Speaker != selectedSpeaker)
            continue;

        // Tagfilter
        if (selectedTag != "Alle Tags" && !segment.Tags.contains(selectedTag))
            continue;

        // Keyword-Suche
        if (!keyword.isEmpty() && !segment.Text.contains(keyword, Qt::CaseInsensitive))
            continue;

        // Trefferanzeige vorbereiten
        QString display = QString("[%1] %2: %3")
                              .arg(segmentTime.toString("HH:mm:ss"))
                              .arg(segment.Speaker)
                              .arg(segment.Text);

        // Treffer zur Ergebnisliste hinzufügen
        auto *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, segmentTime);
        item->setData(Qt::UserRole + 1, segment.Speaker);
        item->setData(Qt::UserRole + 2, segment.Text);
        resultsList->addItem(item);
        hits++;
    }

    // Trefferanzahl anzeigen
    statusLabel->setText(QString("Gefundene Treffer: %1").arg(hits));
}

//--------------------------------------------------------------------------------------------------

void SearchDialog::onSearchClicked()
{
    performSearch();
}

//--------------------------------------------------------------------------------------------------

void SearchDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    QString matchedText = item->data(Qt::UserRole + 2).toString().trimmed();

    // Fallback, falls Datenfeld leer
    if (matchedText.isEmpty()) {
        matchedText = item->text();
    }

    // Signal senden und Dialog schließen
    emit searchResultSelected(matchedText);
    accept();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

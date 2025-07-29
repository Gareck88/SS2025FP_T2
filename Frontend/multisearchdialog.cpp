#include "multisearchdialog.h"
#include "transcription.h"
#include "databasemanager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTimeEdit>
#include <QSet>
#include <QMessageBox>


MultiSearchDialog::MultiSearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Erweiterte Mehrfachsuche");
    resize(600, 500);

    // GUI-Elemente initialisieren
    keywordInput = new QLineEdit(this);
    speakerFilter = new QComboBox(this);
    tagFilter = new QComboBox(this);
    startTimeEdit = new QTimeEdit(this);
    endTimeEdit = new QTimeEdit(this);
    searchButton = new QPushButton("Suchen", this);
    resultsList = new QListWidget(this);
    statusLabel = new QLabel(this);

    // Platzhalter für Filteroptionen
    speakerFilter->addItem("Alle Sprecher");
    tagFilter->addItem("Alle Tags");

    // Zeitformate setzen
    startTimeEdit->setDisplayFormat("HH:mm:ss");
    endTimeEdit->setDisplayFormat("HH:mm:ss");
    startTimeEdit->setTime(QTime(0, 0, 0));
    endTimeEdit->setTime(QTime(23, 59, 59));

    // Layouts aufbauen
    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow("Schlüsselwort:", keywordInput);
    formLayout->addRow("Sprecher:", speakerFilter);
    formLayout->addRow("Tag:", tagFilter);
    formLayout->addRow("Startzeit:", startTimeEdit);
    formLayout->addRow("Endzeit:", endTimeEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(searchButton);
    mainLayout->addWidget(resultsList);
    mainLayout->addWidget(statusLabel);

    // Signal-Slot-Verbindungen
    connect(searchButton, &QPushButton::clicked, this, &MultiSearchDialog::onSearchClicked);
    connect(resultsList, &QListWidget::itemDoubleClicked, this, &MultiSearchDialog::onItemDoubleClicked);
}
//--------------------------------------------------------------------------------------------------

void MultiSearchDialog::setTranscriptionsMap(const QMap<QString, Transcription*> &map)
{
    transcriptionMap = map;
    loadSpeakerAndTagOptionsFromTranscriptions(map);
}
//--------------------------------------------------------------------------------------------------

void MultiSearchDialog::loadSpeakerAndTagOptionsFromTranscriptions(const QMap<QString, Transcription*> &transcriptions)
{
    QSet<QString> allSpeakers;
    QSet<QString> allTags;

    for (auto it = transcriptions.constBegin(); it != transcriptions.constEnd(); ++it) {
        Transcription *t = it.value();
        if (!t)
            continue;

        const QList<MetaText> &segments = t->getMetaTexts();

        for (const MetaText &segment : segments) {
            allSpeakers.insert(segment.Speaker);

            for (const QString &tag : segment.Tags) {
                allTags.insert(tag);
            }
        }
    }

    // Filter-Widgets leeren und neu füllen
    speakerFilter->clear();
    tagFilter->clear();

    speakerFilter->addItem("Alle Sprecher");
    tagFilter->addItem("Alle Tags");

    for (const QString &s : std::as_const(allSpeakers))
        speakerFilter->addItem(s);

    for (const QString &tag : std::as_const(allTags))
        tagFilter->addItem(tag);
}
//--------------------------------------------------------------------------------------------------

QTime MultiSearchDialog::parseTimeFromSeconds(const QString &timestamp) const
{
    QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
    if (!dt.isValid())
        return QTime(); // Rückfall bei ungültigem Datum

    return dt.time();
}
//--------------------------------------------------------------------------------------------------

void MultiSearchDialog::onSearchClicked()
{
    performSearch();
}
//--------------------------------------------------------------------------------------------------

// Reaktion auf Doppelklick in Ergebnisliste – löst Signal aus und schließt Dialog
void MultiSearchDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    QString meetingName = item->data(Qt::UserRole).toString();
    QString matchedText = item->data(Qt::UserRole + 2).toString();
    emit searchResultSelected(matchedText, meetingName);
    accept();
}
//--------------------------------------------------------------------------------------------------

void MultiSearchDialog::performSearch()
{
    resultsList->clear(); // Vorherige Ergebnisse löschen

    QString searchTerm = keywordInput->text().trimmed();
    QString selectedSpeaker = speakerFilter->currentText();
    QString selectedTag = tagFilter->currentText();
    QTime startTime = startTimeEdit->time();
    QTime endTime = endTimeEdit->time();

    // Warnung bei leerer Eingabe
    if (searchTerm.isEmpty() && selectedSpeaker == "Alle Sprecher" && selectedTag == "Alle Tags") {
        QMessageBox::warning(this, "Suche", "Bitte gib einen Suchbegriff ein oder wähle mindestens einen Filter.");
        return;
    }

    int resultsCount = 0;

    // Alle Transkripte durchsuchen
    for (auto it = transcriptionMap.begin(); it != transcriptionMap.end(); ++it) {
        const QString &meetingName = it.key();
        Transcription *t = it.value();
        if (!t) continue;

        const QList<MetaText> &segments = t->getMetaTexts();

        for (const MetaText &segment : segments) {
            // Zeitfilter
            QTime segmentTime = parseTimeFromSeconds(segment.Start);
            if (segmentTime.isValid() && (segmentTime < startTime || segmentTime > endTime))
                continue;

            // Sprecherfilter
            if (selectedSpeaker != "Alle Sprecher" && segment.Speaker != selectedSpeaker)
                continue;

            // Tagfilter
            if (selectedTag != "Alle Tags" && !segment.hasTag(selectedTag))
                continue;

            // Textinhalt durchsuchen
            if (!searchTerm.isEmpty() && !segment.Text.contains(searchTerm, Qt::CaseInsensitive))
                continue;

            QString timeStr = segmentTime.isValid() ? segmentTime.toString("HH:mm:ss") : "Unbekannt";

            QString displayText = QString("Besprechung: %1\nZeit:        %2\nSprecher:    %3\nTranskript:  %4")
                                      .arg(meetingName)
                                      .arg(timeStr)
                                      .arg(segment.Speaker)
                                      .arg(segment.Text);

            // Ergebnis zur Ergebnisliste hinzufügen
            QListWidgetItem *item = new QListWidgetItem(displayText, resultsList);
            item->setData(Qt::UserRole, meetingName);
            item->setData(Qt::UserRole + 1, segment.Speaker);
            item->setData(Qt::UserRole + 2, segment.Text);
            item->setData(Qt::UserRole + 3, segmentTime);
            resultsCount++;
        }
    }

    // Ergebnisanzeige oder Hinweis, dass keine Treffer vorliegen
    if (resultsCount == 0)
        QMessageBox::information(this, "Keine Treffer", "Keine Segmente gefunden.");
    else
        statusLabel->setText(QString::number(resultsCount) + " Treffer gefunden.");
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


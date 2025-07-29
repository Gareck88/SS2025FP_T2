#include "searchdialog.h"
#include "transcription.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QTimeEdit>
#include <QVBoxLayout>

SearchDialog::SearchDialog (
    QWidget *parent)
    : QDialog (parent)
{
    setWindowTitle ("Erweiterte Suche im Transkript");
    resize (500, 400);

    keywordInput = new QLineEdit (this);
    speakerFilter = new QComboBox (this);
    tagFilter = new QComboBox (this);
    startTimeEdit = new QTimeEdit (this);
    endTimeEdit = new QTimeEdit (this);
    searchButton = new QPushButton ("Suchen", this);
    resultsList = new QListWidget (this);
    statusLabel = new QLabel (this);

    speakerFilter->addItem ("Alle Sprecher");
    tagFilter->addItem ("Alle Tags");

    startTimeEdit->setDisplayFormat ("HH:mm:ss");
    endTimeEdit->setDisplayFormat ("HH:mm:ss");
    startTimeEdit->setTime (QTime (0, 0, 0));
    endTimeEdit->setTime (QTime (23, 59, 59));

    QFormLayout *formLayout = new QFormLayout ();
    formLayout->addRow ("Schlüsselwort:", keywordInput);
    formLayout->addRow ("Sprecher:", speakerFilter);
    formLayout->addRow ("Tag:", tagFilter);
    formLayout->addRow ("Startzeit:", startTimeEdit);
    formLayout->addRow ("Endzeit:", endTimeEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout (this);
    mainLayout->addLayout (formLayout);
    mainLayout->addWidget (searchButton);
    mainLayout->addWidget (resultsList);
    mainLayout->addWidget (statusLabel);

    connect (searchButton, &QPushButton::clicked, this, &SearchDialog::onSearchClicked);
    connect (resultsList, &QListWidget::itemDoubleClicked, this, &SearchDialog::onItemDoubleClicked);
}

void SearchDialog::setTranscription (
    Transcription *transcript)
{
    m_transcription = transcript;
    loadSpeakerAndTagOptions ();
}

void SearchDialog::loadSpeakerAndTagOptions ()
{
    if (!m_transcription)
        return;

    QSet<QString> speakers;
    QSet<QString> tags;

    for (const MetaText &segment : m_transcription->getMetaTexts ())
    {
        speakers.insert (segment.Speaker);
        for (const QString &tag : segment.Tags)
            tags.insert (tag);
    }

    for (const QString &s : speakers)
        speakerFilter->addItem (s);

    for (const QString &tag : tags)
        tagFilter->addItem (tag);
}

QTime SearchDialog::parseTimeFromSeconds (
    const QString &seconds) const
{
    bool ok;
    int sec = seconds.toInt (&ok);
    if (!ok)
        return QTime (); // Invalid
    return QTime (0, 0, 0).addSecs (sec);
}

void SearchDialog::performSearch ()
{
    resultsList->clear ();
    if (!m_transcription)
        return;

    QString keyword = keywordInput->text ().trimmed ();
    QString selectedSpeaker = speakerFilter->currentText ();
    QString selectedTag = tagFilter->currentText ();
    QTime start = startTimeEdit->time ();
    QTime end = endTimeEdit->time ();

    int hits = 0;

    for (const MetaText &segment : m_transcription->getMetaTexts ())
    {
        QTime segmentTime = parseTimeFromSeconds (segment.Start);
        if (!segmentTime.isValid () || segmentTime < start || segmentTime > end)
            continue;

        if (selectedSpeaker != "Alle Sprecher" && segment.Speaker != selectedSpeaker)
            continue;

        if (selectedTag != "Alle Tags" && !segment.Tags.contains (selectedTag))
            continue;

        if (!keyword.isEmpty () && !segment.Text.contains (keyword, Qt::CaseInsensitive))
            continue;

        QString display = QString ("[%1] %2: %3")
                              .arg (segmentTime.toString ("HH:mm:ss"))
                              .arg (segment.Speaker)
                              .arg (segment.Text);
        resultsList->addItem (display);
        hits++;
    }

    statusLabel->setText (QString ("Gefundene Treffer: %1").arg (hits));
}

void SearchDialog::onSearchClicked ()
{
    performSearch ();
}

void SearchDialog::onItemDoubleClicked (
    QListWidgetItem *item)
{
    if (!item)
        return;
    emit searchResultSelected (item->text ());
    accept ();
}

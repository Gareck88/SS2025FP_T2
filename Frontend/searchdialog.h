/**
 * @file searchdialog.h
 * @brief 
 * @author Yolanda Fiska
 */
#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QLabel;
class QTimeEdit;
class QListWidgetItem;
class Transcription;

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog (QWidget *parent = nullptr);
    void setTranscription (Transcription *transcript);

signals:
    void searchResultSelected (const QString &matchedText);

private slots:
    void onSearchClicked ();
    void onItemDoubleClicked (QListWidgetItem *item);

private:
    QLineEdit *keywordInput;
    QComboBox *speakerFilter;
    QTimeEdit *startTimeEdit;
    QTimeEdit *endTimeEdit;
    QComboBox *tagFilter;
    QPushButton *searchButton;
    QListWidget *resultsList;
    QLabel *statusLabel;

    Transcription *m_transcription = nullptr;

    void loadSpeakerAndTagOptions ();
    void performSearch ();
    QTime parseTimeFromSeconds (const QString &seconds) const;
};

#endif // SEARCHDIALOG_H

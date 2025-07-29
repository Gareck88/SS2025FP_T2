/**
 * @file searchdialog.h
 * @brief Diese Klasse stellt einen Dialog zum Durchsuchen einer Transkription bereit.
 * @author Yolanda Fiska
 */
#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>

// Vorwärtsdeklarationen für verwendete Qt-Widgets
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
    /**
     * @brief Konstruktor des Dialogs.
     * @param parent Übergeordnetes Widget, optional.
     */
    explicit SearchDialog(QWidget *parent = nullptr);

    /**
     * @brief Setzt die Transkription, in der gesucht werden soll.
     * @param transcript Zeiger auf ein Transcription-Objekt.
     */
    void setTranscription(Transcription *transcript);

signals:
    /**
     * @brief Wird ausgelöst, wenn ein Suchergebnis ausgewählt wurde.
     * @param matchedText Der gefundene Textausschnitt.
     */
    void searchResultSelected(const QString &matchedText);

private slots:
    // Slot wird aufgerufen, wenn der Benutzer auf „Suchen“ klickt
    void onSearchClicked();

    // Slot wird aufgerufen, wenn ein Suchergebnis doppelt angeklickt wird
    void onItemDoubleClicked(QListWidgetItem *item);

private:
     // UI-Elemente
    // Eingabefeld für Suchbegriffe
    QLineEdit *keywordInput;     ///< Eingabefeld für das Suchwort
    QComboBox *speakerFilter;    ///< Filterauswahl für Sprecher
    QTimeEdit *startTimeEdit;    ///< Zeitfilter: Startzeit
    QTimeEdit *endTimeEdit;      ///< Zeitfilter: Endzeit
    QComboBox *tagFilter;        ///< Filterauswahl für Tags
    QPushButton *searchButton;   ///< Button zum Auslösen der Suche
    QListWidget *resultsList;    ///< Anzeige der Suchergebnisse
    QLabel *statusLabel;         ///< Statusanzeige (z. B. Trefferanzahl)

    Transcription *m_transcription = nullptr; ///< Aktive Transkription zur Durchsuchung


    /**
     * @brief Lädt verfügbare Sprecher und Tags aus der Transkription in die Filter.
     */
    void loadSpeakerAndTagOptions();

    /**
     * @brief Führt die eigentliche Suche anhand der Filterkriterien durch.
     */
    void performSearch();

    /**
     * @brief Konvertiert Sekundenangabe in ein QTime-Objekt.
     * @param seconds Zeit in Sekunden als String.
     * @return QTime entsprechend der Sekundenangabe.
     */
    QTime parseTimeFromSeconds(const QString &seconds) const;
};

#endif // SEARCHDIALOG_H

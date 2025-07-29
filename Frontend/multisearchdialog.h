#ifndef MULTISEARCHDIALOG_H
#define MULTISEARCHDIALOG_H

#include <QDialog>
#include <QMap>
#include <QDateEdit>

class QLineEdit;
class QComboBox;
class QTimeEdit;
class QPushButton;
class QListWidget;
class QLabel;
class QListWidgetItem;
class Transcription;


/**
 * @class MultiSearchDialog
 * @brief Dialogfenster für eine erweiterte Suche in mehreren Transkriptionen.
 *
 * Dieses Dialogfenster ermöglicht es dem Nutzer, gezielt nach Schlüsselwörtern in mehreren
 * gespeicherten Transkriptionen zu suchen. Die Suche kann optional nach Sprecher, Tag und
 * Zeitintervall gefiltert werden. Gefundene Ergebnisse werden in einer Liste angezeigt,
 * bei Doppelklick wird das zugehörige Meeting geladen.
 *
 * @param parent Das übergeordnete QWidget, meist das Hauptfenster.
 */
class MultiSearchDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief Konstruktor für den Dialog.
     * @param parent Das übergeordnete QWidget.
     */
    explicit MultiSearchDialog(QWidget *parent = nullptr);

    /**
     * @brief Übergibt die Map aller Transkriptionen.
     * @param map Map mit Besprechungsnamen als Schlüssel und Transcription-Pointern als Werte.
     */
    void setTranscriptionsMap(const QMap<QString, Transcription*> &map);

signals:
    /**
     * @brief Wird ausgelöst, wenn ein Suchergebnis ausgewählt wurde.
     * @param matchedText Der gefundene Textausschnitt.
     * @param meetingName Titel des zugehörigen Meetings.
     */
    void searchResultSelected(const QString &matchedText, const QString &meetingName);

private slots:
    // Wird ausgelöst, wenn der Benutzer auf den "Suchen"-Button klickt
    void onSearchClicked();

    // Wird ausgelöst, wenn ein Ergebnis doppelt angeklickt wird
    void onItemDoubleClicked(QListWidgetItem *item);

    // Führt die eigentliche Suche aus und zeigt die Treffer an
    void performSearch();

private:
    /**
     * @brief Lädt alle verfügbaren Sprecher und Tags aus den Transkriptionen.
     * @param transcriptions Die Map mit allen Transkriptionsobjekten.
     */
    void loadSpeakerAndTagOptionsFromTranscriptions(const QMap<QString, Transcription*> &transcriptions);

    /**
     * @brief Wandelt Sekunden (als String) in QTime um.
     * @param seconds Zeitwert in Sekunden als String.
     * @return Entsprechendes QTime-Objekt.
     */
    QTime parseTimeFromSeconds(const QString &seconds) const;

    // UI-Elemente
    QLineEdit *keywordInput;      // Eingabefeld für das Suchwort
    QComboBox *speakerFilter;     // Auswahlfeld für Sprecher
    QComboBox *tagFilter;         // Auswahlfeld für Tags
    QTimeEdit *startTimeEdit;     // Startzeitfilter
    QTimeEdit *endTimeEdit;       // Endzeitfilter
    QPushButton *searchButton;    // Button zur Auslösung der Suche
    QListWidget *resultsList;     // Anzeige der Suchergebnisse
    QLabel *statusLabel;          // Statusmeldung (z. B. Trefferanzahl)
    QDateEdit *dateFromEdit;      // Filter: Beginn des Datumsbereichs
    QDateEdit *dateToEdit;        // Filter: Ende des Datumsbereichs
    ///< Alle geladenen Transkriptionen (Meetingtitel → Transcription*)
    QMap<QString, Transcription*> transcriptionMap;
};

#endif // MULTISEARCHDIALOG_H

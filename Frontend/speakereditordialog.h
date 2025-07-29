/**
 * @file speakereditordialog.h
 * @brief Enthält die Deklaration des SpeakerEditorDialog zur Bearbeitung von Sprechernamen.
 * @author Mike Wild
 */
#ifndef SPEAKEREDITORDIALOG_H
#define SPEAKEREDITORDIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>

#include "transcription.h"

// Forward-Deklarationen für UI-Elemente
class QTableWidgetItem;
class QTableWidget;
class QComboBox;
class QPushButton;
class QLabel;
class QTabWidget;
class QLineEdit;
class QTimer;

/**
 * @brief Ein nicht-modaler Dialog zur Bearbeitung von Sprecherinformationen im Transkript.
 *
 * Der Dialog bietet zwei Ansichten (Tabs):
 * 1. Eine globale Ansicht, um einen Sprechernamen im gesamten Transkript zu ändern
 *    (z.B. "SPEAKER_00" in "Max Mustermann").
 * 2. Eine segment-basierte Ansicht, um den Sprecher für einzelne Textabschnitte zu korrigieren.
 *    Der Dialog operiert auf einem ihm übergebenen Transcription-Objekt.
 */
class SpeakerEditorDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor, der den Dialog initialisiert und mit dem Datenmodell verknüpft.
     * @param transcription Ein Zeiger auf das Transcription-Objekt, das bearbeitet werden soll.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit SpeakerEditorDialog (Transcription* transcription, QWidget* parent = nullptr);

    /**
     * @brief Ermöglicht das programmatische Vor-Auswählen eines Segments im Dialog.
     * @param start Der Start-Zeitstempel des zu selektierenden Segments.
     * @param end Der End-Zeitstempel des zu selektierenden Segments.
     */
    void setSelectedSegment (const QString& start, const QString& end);

public slots:
    /**
     * @brief Aktualisiert die Ansichten des Dialogs, wenn sich das zugrundeliegende
     * Transcription-Objekt ändert.
     * @note Sollte mit dem `Transcription::changed()` Signal verbunden werden.
     */
    void onTranscriptionChanged ();

private slots:
    /** @brief Erstellt die Benutzeroberfläche und verbindet die internen Signale und Slots. */
    void setupUI ();

    /** @brief Füllt die Tabelle für die globalen Sprechernamen mit Daten. */
    void populateGlobalSpeakerTable ();

    /** @brief Füllt die Tabelle für die einzelnen Textsegmente mit Daten. */
    void populateSegmentTable ();

    /** @brief Slot für die "Anwenden" und "OK" Buttons. Übernimmt die Änderungen. */
    void handleApplyOkButtonClicked ();

    /** @brief Slot für den "Abbrechen" Button. Schließt den Dialog ohne Änderungen. */
    void handleCancelButtonClicked ();

    /** @brief Zeigt eine temporäre oder permanente Nachricht in der Statuszeile des Dialogs an. */
    void setDialogStatus (const QString& text, bool temporary = true);

    /** @brief Reagiert auf eine Änderung in der Sprecher-ComboBox eines Segments. */
    void onSegmentSpeakerChanged (int index);

    /** @brief Reagiert auf eine Textänderung im Eingabefeld für einen globalen Sprechernamen. */
    void onGlobalSpeakerNameChanged (const QString& text);

    /** @brief Führt die ausgewählten Sprecher unter einem neuen Namen zusammen. */
    void onMergeSpeakersClicked ();

private:
    /** @brief Wendet die im aktuell aktiven Tab vorgenommenen Änderungen auf das Transcription-Objekt an. */
    void applyCurrentTabChanges ();

    /** @brief Aktualisiert die interne Liste aller bekannten Sprechernamen aus dem Transkript. */
    void updateKnownSpeakers ();

    // --- Member-Variablen ---
    QPointer<Transcription> m_transcription; ///< Ein sicherer Zeiger auf das Transkript-Datenmodell.

    // UI-Elemente
    QTabWidget* m_tabWidget;
    QTableWidget* m_globalSpeakerTable;
    QTableWidget* m_segmentTable;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QLabel* m_statusLabel;
    QTimer* m_statusTimer;
    QLineEdit* m_mergeNameEdit;
    QPushButton* m_mergeSpeakersButton;

    // Interne Zustands- und Puffer-Variablen
    QSet<QString> m_allKnownSpeakers; ///< Eine Menge aller einzigartigen Sprechernamen im Transkript.
    QMap<QString, QString> m_currentGlobalNames; ///< Puffer für globale Namensänderungen.
    QMap<QPair<QString, QString>, QString>
        m_currentSegmentNames; ///< Puffer für segmentweise Sprecheränderungen.

    QString m_selectedSegmentStart; ///< Merker für das programmatisch ausgewählte Segment.
    QString m_selectedSegmentEnd;
};

#endif // SPEAKEREDITORDIALOG_H

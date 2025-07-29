/**
 * @file texteditordialog.h
 * @brief Enthält die Deklaration des TextEditorDialog zur Bearbeitung des Transkript-Textes.
 * @author Mike Wild
 */
#ifndef TEXTEDITORDIALOG_H
#define TEXTEDITORDIALOG_H

#include <QDialog>
#include <QMap>
#include <QPointer>

#include "transcription.h"

// Forward-Deklarationen
class QTableWidget;
class QPushButton;
class QLabel;
class QTimer;
class QTableWidgetItem;
class QLineEdit;

/**
 * @brief Ein nicht-modaler Dialog zur direkten Bearbeitung der Textinhalte von Transkript-Segmenten.
 *
 * Der Dialog stellt alle Segmente des Transkripts in einer Tabelle dar, in der
 * der Text direkt editiert werden kann. Änderungen werden gepuffert und erst
 * durch einen Klick auf "Anwenden" oder "OK" in das Transcription-Datenmodell übernommen.
 */
class TextEditorDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor, der den Dialog initialisiert.
     * @param transcription Ein Zeiger auf das Transcription-Objekt, dessen Texte bearbeitet werden sollen.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit TextEditorDialog (Transcription* transcription, QWidget* parent = nullptr);

public slots:
    /**
     * @brief Aktualisiert die Tabelle, falls das zugrundeliegende Transkript extern geändert wurde.
     * @note Sollte mit dem `Transcription::changed()` Signal verbunden werden.
     */
    void onTranscriptionChanged ();

private slots:
    /** @brief Übernimmt alle gepufferten Änderungen in das Transcription-Objekt. */
    void applyChanges ();

    /** @brief Slot für den "Anwenden"-Button. Ruft applyChanges() auf. */
    void handleApplyButtonClicked ();

    /** @brief Slot für den "OK"-Button. Ruft applyChanges() auf und schließt den Dialog. */
    void handleOkButtonClicked ();

    /** @brief Slot für den "Abbrechen"-Button. Schließt den Dialog ohne Änderungen. */
    void handleCancelButtonClicked ();

    /** @brief Wird aufgerufen, wenn ein Benutzer den Text in einer Zelle der Tabelle ändert. */
    void onTextItemChanged (QTableWidgetItem* item);

private:
    /** @brief Erstellt und konfiguriert die UI-Elemente des Dialogs. */
    void setupUI ();

    /** @brief Füllt die Tabelle mit den Segment-Daten aus dem Transcription-Objekt. */
    void populateTable ();

    /** @brief Zeigt eine Statusnachricht im Dialog an. */
    void setDialogStatus (const QString& text, bool temporary = true);

    // --- Member-Variablen ---
    QPointer<Transcription> m_transcription; ///< Sicherer Zeiger auf das Datenmodell.

    // UI-Elemente
    QTableWidget* m_table;
    QPushButton* m_applyButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QLabel* m_statusLabel;
    QTimer* m_statusTimer;

    /**
     * @brief Puffer für noch nicht gespeicherte Textänderungen.
     * Der Key ist ein Paar aus Start-/End-Zeitstempel, der Value ist der neue Text.
     */
    QMap<QPair<QString, QString>, QString> m_pendingTextChanges;
};

#endif // TEXTEDITORDIALOG_H

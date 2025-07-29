#include "transcriptpdfexporter.h"
#include "transcription.h"

#include <QColor>
#include <QFile>
#include <QMap>
#include <QMarginsF>
#include <QPageSize>
#include <QPdfWriter>
#include <QSettings>
#include <QTextDocument>
#include <QTime>

TranscriptPdfExporter::TranscriptPdfExporter (
    const Transcription &transcription)
    : m_transcription (transcription)
{
    //  Liest die vom Benutzer konfigurierten Layout-Einstellungen aus den QSettings.
    //  Falls keine Einstellungen vorhanden sind, werden sinnvolle Standardwerte verwendet.
    QSettings settings;
    m_fontSizeHeadline = settings.value ("pdf/fontSizeHeadline", 42).toInt ();
    m_fontSizeMetadata = settings.value ("pdf/fontSizeMetadata", 10).toInt ();
    m_fontSizeBody = settings.value ("pdf/fontSizeBody", 12).toInt ();
    m_fontFamily = settings.value ("pdf/fontFamily", "sans-serif").toString ();
    m_marginLeft = settings.value ("pdf/marginLeft", 25).toInt ();
    m_marginTop = settings.value ("pdf/marginTop", 25).toInt ();
    m_marginRight = settings.value ("pdf/marginRight", 25).toInt ();
    m_marginBottom = settings.value ("pdf/marginBottom", 25).toInt ();
}

//--------------------------------------------------------------------------------------------------

bool TranscriptPdfExporter::exportToPdf (
    const QString &filePath) const
{
    QPdfWriter writer (filePath);
    setupPdfWriter (writer);

    QTextDocument doc;

    QString htmlContent;
    buildHtmlContent (htmlContent);
    doc.setHtml (htmlContent);

    //  Die print()-Funktion von QTextDocument ist der Schlüssel für korrekte Paginierung.
    doc.print (&writer);

    if (QFile::exists (filePath) && QFile (filePath).size () > 0)
    {
        return true;
    }
    else
    {
        qWarning () << "PDF-Export nach" << filePath << "scheint fehlgeschlagen zu sein.";
        return false;
    }
}

//--------------------------------------------------------------------------------------------------

void TranscriptPdfExporter::setupPdfWriter (
    QPdfWriter &writer) const
{
    writer.setPageSize (QPageSize (QPageSize::A4));
    writer.setResolution (300);
    writer.setPageMargins (QMarginsF (m_marginLeft, m_marginTop, m_marginRight, m_marginBottom));
}

//--------------------------------------------------------------------------------------------------

void TranscriptPdfExporter::buildHtmlContent (
    QString &html) const
{
    //  --- 1. Metadaten für die Anzeige vorbereiten. ---
    QString durationStr = m_transcription.getDurationAsString ();
    QString dateStr = m_transcription.dateTime ().toString ("dd. MMMM yyyy, hh:mm:ss");
    QString tagsStr;
    if (!m_transcription.tags ().isEmpty ())
    {
        tagsStr = QString ("<b>Tags:</b> %1").arg (m_transcription.tags ().join (", "));
    }

    //  --- 2. Transkript-Segmente zu zusammenhängenden Dialogblöcken pro Sprecher gruppieren. ---
    struct DialogBlock
    {
        QString speaker, text;
    };
    QList<DialogBlock> groupedBlocks;
    if (!m_transcription.getMetaTexts ().isEmpty ())
    {
        DialogBlock currentBlock = {m_transcription.getMetaTexts ().first ().Speaker,
                                    m_transcription.getMetaTexts ().first ().Text};
        for (int i = 1; i < m_transcription.getMetaTexts ().size (); ++i)
        {
            const auto &segment = m_transcription.getMetaTexts ().at (i);
            if (segment.Speaker == currentBlock.speaker)
            {
                currentBlock.text.append (" " + segment.Text);
            }
            else
            {
                groupedBlocks.append (currentBlock);
                currentBlock = {segment.Speaker, segment.Text};
            }
        }
        groupedBlocks.append (currentBlock);
    }

    //  --- 3. HTML für die Dialog-Tabelle aus den gruppierten Blöcken generieren. ---
    static const QList<QColor> speakerColors = {QColor ("#00539C"),
                                                QColor ("#2E8B57"),
                                                QColor ("#B22222"),
                                                QColor ("#800080"),
                                                QColor ("#D2691E"),
                                                QColor ("#4682B4"),
                                                QColor ("#008080"),
                                                QColor ("#8B4513")};
    QMap<QString, QColor> speakerToColorMap;
    int colorIndex = 0;
    QString transcriptBody;
    transcriptBody += "<table class='dialog-table'>";
    for (const auto &block : groupedBlocks)
    {
        if (!speakerToColorMap.contains (block.speaker))
        {
            speakerToColorMap[block.speaker] = speakerColors.at (colorIndex % speakerColors.size ());
            colorIndex++;
        }
        QColor color = speakerToColorMap.value (block.speaker);
        transcriptBody
            += QString (
                   "<tr>"
                   "  <td class='speaker-cell' style='color: %1;'><b>%2:</b>&nbsp;&nbsp;&nbsp;</td>"
                   "  <td class='text-cell'>%3</td>"
                   "</tr>")
                   .arg (color.name ())
                   .arg (block.speaker.toHtmlEscaped ())
                   .arg (block.text.toHtmlEscaped ());
    }
    transcriptBody += "</table>";

    //  --- 4. Das gesamte HTML-Dokument mit CSS zusammenbauen. ---
    html = QString (
               "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
               "<style>"
               "body { font-family: '%1'; font-size: %2pt; color: #111; }"
               "p.headline { font-size: %3pt; font-weight: bold; text-align: center; "
               "margin-bottom: 20px; }"
               "div.metadata { font-size: %4pt; color: #333; border: 1px solid #ccc; "
               "background-color: #f9f9f9; padding: 15px; margin-top: 20px; margin-bottom: 30px; }"
               "table.dialog-table { width: 100%; border-collapse: collapse; }"
               "td.speaker-cell { width: 20%%; vertical-align: top; padding-bottom: 12px; }"
               "td.text-cell { width: 80%%; vertical-align: top; padding-bottom: 12px; text-align: "
               "justify; }"
               "</style></head>"
               "<body>"
               "<p class='headline'>%5</p>"
               "<div class='metadata'><b>Datum:</b> %6<br/><b>Dauer:</b> %7<br/>%8</div>"
               "<hr/>"
               "%9"
               "</body></html>")
               .arg (m_fontFamily)                             //  %1
               .arg (m_fontSizeBody)                           //  %2
               .arg (m_fontSizeHeadline)                       //  %3
               .arg (m_fontSizeMetadata)                       //  %4
               .arg (m_transcription.name ().toHtmlEscaped ()) //  %5
               .arg (dateStr)                                  //  %6
               .arg (durationStr)                              //  %7
               .arg (tagsStr)                                  //  %8
               .arg (transcriptBody);                          //  %9
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

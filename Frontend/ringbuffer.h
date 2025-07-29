/**
 * @file ringbuffer.h
 * @brief Enthält die Deklaration und Implementierung einer Ringpuffer-Klasse.
 * @author Mike Wild
 */
#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <algorithm>
#include <numeric>
#include <vector>

/**
 * @brief Eine einfache und effiziente Ringpuffer-Implementierung für float-Werte.
 *
 * Diese Klasse implementiert einen "First-In, First-Out" (FIFO) Puffer mit einer
 * festen Kapazität. Wenn der Puffer voll ist und neue Daten geschrieben werden,
 * werden die ältesten Daten überschrieben. Dies ist nützlich für das Streamen
 * von Audiodaten, wo nur die letzten paar Sekunden an Daten relevant sind.
 * Da dies eine Header-only-Klasse ist, sind alle Methoden inline implementiert.
 */
class RingBuffer
{
public:
    /**
     * @brief Erstellt einen Puffer mit einer festen maximalen Kapazität.
     * @param capacity Die maximale Anzahl an float-Werten, die der Puffer halten kann.
     */
    explicit RingBuffer (
        size_t capacity = 0)
    {
        m_buffer.resize (capacity);
    }

    /**
     * @brief Ändert die Größe des Puffers und löscht seinen Inhalt.
     * @param capacity Die neue maximale Kapazität.
     */
    void resize (
        size_t capacity)
    {
        m_buffer.resize (capacity);
        clear ();
    }

    /**
     * @brief Löscht den Inhalt des Puffers, indem die Zeiger zurückgesetzt werden.
     */
    void clear ()
    {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    /**
     * @brief Gibt die Anzahl der aktuell im Puffer befindlichen Elemente zurück.
     * @return Die aktuelle Füllmenge.
     */
    size_t size () const { return m_size; }

    /**
     * @brief Gibt die maximale Kapazität des Puffers zurück.
     * @return Die maximale Anzahl an Elementen, die der Puffer halten kann.
     */
    size_t capacity () const { return m_buffer.size (); }

    /**
     * @brief Schreibt neue Daten in den Puffer.
     *
     * Fügt die Daten am Schreibzeiger hinzu. Wenn der Puffer voll ist,
     * werden die ältesten Daten überschrieben.
     * @param data Ein Zeiger auf das Array mit den zu schreibenden Daten.
     * @param count Die Anzahl der zu schreibenden float-Werte.
     */
    void write (
        const float* data, size_t count)
    {
        if (count == 0 || capacity () == 0)
        {
            return;
        }

        // Wenn wir mehr Daten schreiben, als freier Platz vorhanden ist,
        // müssen wir die ältesten Daten "vergessen", indem wir den Lesezeiger (tail) nach vorne schieben.
        size_t free_space = capacity () - m_size;
        if (count > free_space)
        {
            size_t overwrite_count = count - free_space;
            // Verschiebe den Lesezeiger um die Anzahl der zu überschreibenden Elemente.
            // Der Modulo-Operator sorgt für das "Herumwickeln" am Ende des Puffers.
            m_tail = (m_tail + overwrite_count) % capacity ();
        }

        // Schreibe die neuen Daten an der aktuellen Schreibposition (head).
        for (size_t i = 0; i < count; ++i)
        {
            m_buffer[m_head] = data[i];
            // Verschiebe den Schreibzeiger und wickle ihn bei Bedarf um.
            m_head = (m_head + 1) % capacity ();
        }

        // Aktualisiere die Größe des Puffers. Sie kann nicht größer als die Kapazität werden.
        m_size = std::min (m_size + count, capacity ());
    }

    /**
     * @brief Liest einen Sample-Wert an einer bestimmten Fließkomma-Position.
     *
     * Diese Funktion verwendet lineare Interpolation zwischen zwei Samples, um einen
     * Wert an einer nicht-ganzzahligen Position zu schätzen. Dies ist sehr nützlich
     * für Audio-Resampling.
     * @param pos Die Fließkomma-Position des gewünschten Samples relativ zum Pufferanfang.
     * @return Der interpolierte Sample-Wert.
     */
    float sampleAt (
        double pos) const
    {
        // Für eine Interpolation benötigen wir mindestens zwei Punkte.
        if (m_size < 2)
        {
            return (m_size == 1) ? m_buffer[m_tail] : 0.0f;
        }

        // Finde die beiden umgebenden ganzzahligen Indizes.
        size_t index0 = static_cast<size_t> (pos);
        size_t index1 = index0 + 1;

        // Sicherstellen, dass wir nicht über das Ende der gültigen Daten hinaus lesen.
        if (index1 >= m_size)
        {
            // Am Rand geben wir einfach den letzten gültigen Wert zurück, um Klicks zu vermeiden.
            return m_buffer[(m_tail + m_size - 1) % capacity ()];
        }

        // Berechne den Anteil zwischen den beiden Punkten (z.B. 0.75 für eine Position von 4.75).
        float frac = static_cast<float> (pos - index0);

        // Hole die beiden Sample-Werte. Wichtig: Die Indizes müssen relativ
        // zum aktuellen Lesezeiger (m_tail) berechnet werden.
        float s0 = m_buffer[(m_tail + index0) % capacity ()];
        float s1 = m_buffer[(m_tail + index1) % capacity ()];

        // Lineare Interpolation: Starte bei s0 und addiere den Bruchteil der Differenz zu s1.
        return s0 + frac * (s1 - s0);
    }

    /**
     * @brief "Konsumiert" eine Anzahl von Samples, indem der Lesezeiger verschoben wird.
     *
     * Entfernt effektiv die ältesten `count` Elemente aus dem Puffer.
     * @param count Die Anzahl der zu entfernenden Elemente.
     */
    void consume (
        size_t count)
    {
        // Wir können nicht mehr Elemente entfernen, als vorhanden sind.
        count = std::min (count, m_size);
        // Verschiebe den Lesezeiger und wickle ihn bei Bedarf um.
        m_tail = (m_tail + count) % capacity ();
        m_size -= count;
    }

private:
    std::vector<float> m_buffer; ///< Der zugrundeliegende Speicher für die Pufferdaten.
    size_t m_head = 0;           ///< Die Position (Index) für den nächsten Schreibvorgang.
    size_t m_tail = 0;           ///< Die Position (Index) des ältesten, noch gültigen Samples.
    size_t m_size = 0;           ///< Die aktuelle Anzahl der gültigen Samples im Puffer.
};

#endif // RINGBUFFER_H

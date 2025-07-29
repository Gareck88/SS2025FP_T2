#include "wincapturethread.h"

#include <QDebug>
#include <QSettings>
#include <comdef.h> //  Für _com_error zur lesbaren Ausgabe von HRESULT-Fehlercodes.

#pragma comment(lib, "ole32.lib") //  Stellt sicher, dass die COM-Bibliothek gelinkt wird.

//--------------------------------------------------------------------------------------------------

WinCaptureThread::WinCaptureThread(QObject *parent)
    : CaptureThread(parent)
{
}

//--------------------------------------------------------------------------------------------------

void WinCaptureThread::run()
{
    //  Schritt 1: Initialisiere die COM-Bibliothek für diesen spezifischen Thread.
    //  Jeder Thread, der COM-Objekte verwendet, muss dies tun.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        qWarning() << "WinCaptureThread::run: CoInitializeEx FAILED. Thread cannot execute.";
        return;
    }

    //  Schritt 2: Führe die generische Lebenszyklus-Logik der Basisklasse aus.
    CaptureThread::run();

    //  Schritt 3: Gib die COM-Bibliothek für diesen Thread wieder frei.
    CoUninitialize ();
}

//--------------------------------------------------------------------------------------------------

bool WinCaptureThread::initializeCapture()
{
    //  Dieser Block führt die komplexe Initialisierung der WASAPI-Schnittstelle durch.
    HRESULT hr;

    //  Erstellt den Geräte-Enumerator, das zentrale Objekt zum Auffinden von Audio-Geräten.
    hr = CoCreateInstance (__uuidof (MMDeviceEnumerator),
                           nullptr,
                           CLSCTX_ALL,
                           IID_PPV_ARGS (&m_deviceEnumerator));
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: CoCreateInstance failed: "
                   << _com_error(hr).ErrorMessage();
        return false;
    }

    //  --- System-Audio (Loopback) initialisieren ---
    //  Hole das Standard-WIEDERGABE-Gerät (eRender).
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint (eRender, eConsole, &m_deviceSys);
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: GetDefaultAudioEndpoint (Sys) failed: "
                   << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    //  Aktiviere den IAudioClient, die Hauptschnittstelle zum Gerät.
    hr = m_deviceSys->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&m_audioClientSys)
        );
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: Activate (Sys) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    //  Frage das native Audio-Format des Geräts ab.
    WAVEFORMATEX* wfexSys = nullptr;
    hr = m_audioClientSys->GetMixFormat(&wfexSys);
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: GetMixFormat (Sys) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }
    m_nativeSampleRateSys = wfexSys->nSamplesPerSec;
    m_nativeChannelsSys = wfexSys->nChannels;

    //  Initialisiere den Client im Shared-Mode und setze das LOOPBACK-Flag.
    //  Dies ist der entscheidende Schritt, um aufzuzeichnen, was an die Lautsprecher gesendet wird.
    hr = m_audioClientSys->Initialize (AUDCLNT_SHAREMODE_SHARED,
                                       AUDCLNT_STREAMFLAGS_LOOPBACK,
                                       10000000,
                                       0,
                                       wfexSys,
                                       nullptr);
    CoTaskMemFree (wfexSys); //  Die wfex-Struktur muss nach Verwendung freigegeben werden.
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: Initialize (Sys) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    //  Hole den Capture-Client, um die Daten aus dem Puffer lesen zu können.
    hr = m_audioClientSys->GetService (IID_PPV_ARGS (&m_captureClientSys));
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: GetService (Sys) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    //  --- Mikrofon-Audio initialisieren (ähnlicher Prozess wie oben) ---
    //  Hole das Standard-AUFNAHME-Gerät (eCapture).
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint (eCapture, eConsole, &m_deviceMic);
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: GetDefaultAudioEndpoint (Mic) failed: "
                   << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    hr = m_deviceMic->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&m_audioClientMic)
        );
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: Activate (Mic) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    WAVEFORMATEX* wfexMic = nullptr;
    hr = m_audioClientMic->GetMixFormat(&wfexMic);
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: GetMixFormat (Mic) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }
    m_nativeSampleRateMic = wfexMic->nSamplesPerSec;
    m_nativeChannelsMic = wfexMic->nChannels;

    hr = m_audioClientMic->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, wfexMic, nullptr);
    CoTaskMemFree(wfexMic);
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: Initialize (Mic) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    hr = m_audioClientMic->GetService(IID_PPV_ARGS(&m_captureClientMic));
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: GetService (Mic) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    //  --- Puffer und Timer für das Resampling vorbereiten ---
    const int bufferDurationInSeconds = 5;
    size_t bufferSizeSys = m_nativeSampleRateSys * bufferDurationInSeconds;
    size_t bufferSizeMic = m_nativeSampleRateMic * bufferDurationInSeconds;

    m_fifoSysL.resize(bufferSizeSys);
    if (m_nativeChannelsSys > 1)
    {
        m_fifoSysR.resize(bufferSizeSys);
    }
    m_fifoMicL.resize(bufferSizeMic);
    if (m_nativeChannelsMic > 1)
    {
        m_fifoMicR.resize(bufferSizeMic);
    }

    m_fifoSysL.clear();
    m_fifoSysR.clear();
    m_fifoMicL.clear();
    m_fifoMicR.clear();
    m_resampPosSys = 0.0;
    m_resampPosMic = 0.0;
    m_sampleAccumulator = 0.0;

    QueryPerformanceFrequency(&m_perfCounterFreq);
    QueryPerformanceCounter(&m_lastTime);

    //  Starte beide Audio-Streams.
    hr = m_audioClientSys->Start();
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: Start (Sys) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    hr = m_audioClientMic->Start();
    if (FAILED(hr))
    {
        qWarning() << "WinCapture: Start (Mic) failed: " << _com_error(hr).ErrorMessage();
        cleanupCapture();
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

void WinCaptureThread::captureLoopIteration()
{
    HRESULT hr;
    UINT32 packetLengthInFrames = 0;
    BYTE* pData = nullptr;
    DWORD flags;

    //  Fragt, ob neue Daten im System-Audio-Puffer verfügbar sind.
    hr = m_captureClientSys->GetNextPacketSize(&packetLengthInFrames);
    if (SUCCEEDED(hr) && packetLengthInFrames > 0)
    {
        //  Holt die Daten aus dem Puffer und schreibt sie kanalgetrennt in die Ringpuffer.
        hr = m_captureClientSys->GetBuffer (&pData, &packetLengthInFrames, &flags, nullptr, nullptr);
        if (SUCCEEDED(hr))
        {
            auto* floatData = reinterpret_cast<float*>(pData);
            for (UINT32 i = 0; i < packetLengthInFrames; ++i)
            {
                m_fifoSysL.write(&floatData[i * m_nativeChannelsSys], 1);
                if (m_nativeChannelsSys > 1)
                {
                    m_fifoSysR.write(&floatData[i * m_nativeChannelsSys + 1], 1);
                }
            }
            m_captureClientSys->ReleaseBuffer(packetLengthInFrames);
        }
    }

    //  Fragt, ob neue Daten im Mikrofon-Puffer verfügbar sind.
    hr = m_captureClientMic->GetNextPacketSize (&packetLengthInFrames);
    if (SUCCEEDED(hr) && packetLengthInFrames > 0)
    {
        hr = m_captureClientMic->GetBuffer(
            &pData,
            &packetLengthInFrames,
            &flags,
            nullptr,
            nullptr
            );
        if (SUCCEEDED(hr))
        {
            auto* floatData = reinterpret_cast<float*>(pData);
            for (UINT32 i = 0; i < packetLengthInFrames; ++i)
            {
                m_fifoMicL.write(&floatData[i * m_nativeChannelsMic], 1);
                if (m_nativeChannelsMic > 1)
                {
                    m_fifoMicR.write(&floatData[i * m_nativeChannelsMic + 1], 1);
                }
            }
            m_captureClientMic->ReleaseBuffer(packetLengthInFrames);
        }
    }

    //  --- Resampling und Synchronisation via Zeitmessung ---
    //  HINWEIS: Wir verlassen uns nicht darauf, dass beide Audio-Geräte immer gleichzeitig Daten
    //  liefern. Besonders bei Stille würde ein Gerät keine Pakete senden, was ohne diesen
    //  Mechanismus zur Desynchronisation führen würde. Stattdessen messen wir die exakt
    //  vergangene Zeit seit dem letzten Durchlauf.
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double deltaSeconds = static_cast<double>(now.QuadPart - m_lastTime.QuadPart)
                          / m_perfCounterFreq.QuadPart;
    m_lastTime = now;

    //  Basierend auf der vergangenen Zeit wird berechnet, wie viele Samples wir bei unserer
    //  Ziel-Samplerate (48kHz) generieren müssen. Dies erzeugt effektiv "stille" Frames
    //  für einen inaktiven Stream und hält ihn synchron mit einem aktiven Stream.
    const UINT32 targetSampleRate = 48000;
    double exactFramesToGenerate = targetSampleRate * deltaSeconds;

    //  Der Akkumulator verhindert Rundungsfehler bei der Umwandlung von double zu size_t über die Zeit.
    m_sampleAccumulator += exactFramesToGenerate;
    size_t framesToGenerate = static_cast<size_t>(m_sampleAccumulator);
    m_sampleAccumulator -= framesToGenerate;

    if (framesToGenerate == 0)
    {
        msleep (1); //  Kurze Pause, wenn keine neuen Frames generiert werden müssen.
        return;
    }

    //  Berechnet das Verhältnis zwischen nativer und Ziel-Samplerate für das Resampling.
    const double resampRatioSys = static_cast<double> (m_nativeSampleRateSys) / targetSampleRate;
    const double resampRatioMic = static_cast<double>(m_nativeSampleRateMic) / targetSampleRate;

    QList<float> chunk;
    chunk.reserve(framesToGenerate * 2);

    QSettings settings("SS2025FP_T2", "AudioTranskriptor");
    float sysGain = settings.value("sysGain", 0.4f).toFloat();
    float micGain = settings.value("micGain", 1.0f).toFloat();

    for (size_t i = 0; i < framesToGenerate; ++i)
    {
        float sysL = 0.0f, sysR = 0.0f, micL = 0.0f, micR = 0.0f;

        if ((m_resampPosSys + resampRatioSys) < m_fifoSysL.size())
        {
            sysL = m_fifoSysL.sampleAt(m_resampPosSys);
            sysR = (m_nativeChannelsSys > 1) ? m_fifoSysR.sampleAt(m_resampPosSys) : sysL;
        }

        if ((m_resampPosMic + resampRatioMic) < m_fifoMicL.size())
        {
            micL = m_fifoMicL.sampleAt(m_resampPosMic);
            micR = (m_nativeChannelsMic > 1) ? m_fifoMicR.sampleAt(m_resampPosMic) : micL;
        }

        //  Mische die Samples und verhindere Übersteuerung (Clipping).
        chunk.append (qBound (-1.0f, sysL * sysGain + micL * micGain, 1.0f));
        chunk.append(qBound(-1.0f, sysR * sysGain + micR * micGain, 1.0f));

        //  Rücke die Lese-Positionen für den nächsten Sample-Frame vor.
        m_resampPosSys += resampRatioSys;
        m_resampPosMic += resampRatioMic;
    }

    emit pcmChunkReady(chunk);

    //  "Konsumiere" die bereits gelesenen Daten aus den Ringpuffern, um Speicher freizugeben.
    size_t sysFramesToConsume = static_cast<size_t> (m_resampPosSys);
    if (sysFramesToConsume > 0)
    {
        m_fifoSysL.consume(sysFramesToConsume);
        if (m_nativeChannelsSys > 1)
        {
            m_fifoSysR.consume(sysFramesToConsume);
        }
        m_resampPosSys -= sysFramesToConsume;
    }

    size_t micFramesToConsume = static_cast<size_t>(m_resampPosMic);
    if (micFramesToConsume > 0)
    {
        m_fifoMicL.consume(micFramesToConsume);
        if (m_nativeChannelsMic > 1)
        {
            m_fifoMicR.consume(micFramesToConsume);
        }
        m_resampPosMic -= micFramesToConsume;
    }

    msleep(5);
}

//--------------------------------------------------------------------------------------------------

void WinCaptureThread::cleanupCapture()
{
    //  Stoppt die Audio-Streams.
    if (m_audioClientSys)
    {
        m_audioClientSys->Stop();
    }
    if (m_audioClientMic)
    {
        m_audioClientMic->Stop();
    }

    //  Ein Lambda zur sicheren Freigabe von COM-Interface-Zeigern.
    //  Verhindert Abstürze durch doppelte Freigaben, indem der Zeiger nach Release() auf nullptr gesetzt wird.
    auto safeRelease = [] (auto** ppT)
    {
        if (*ppT)
        {
            (*ppT)->Release();
            *ppT = nullptr;
        }
    };

    //  Gibt alle verwendeten COM-Objekte in umgekehrter Reihenfolge ihrer Erstellung frei.
    safeRelease (&m_captureClientSys);
    safeRelease(&m_audioClientSys);
    safeRelease(&m_deviceSys);

    safeRelease(&m_captureClientMic);
    safeRelease(&m_audioClientMic);
    safeRelease(&m_deviceMic);

    safeRelease(&m_deviceEnumerator);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

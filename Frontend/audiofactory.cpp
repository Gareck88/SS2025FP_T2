#include "audiofactory.h"
#include <qglobal.h>

#if defined(Q_OS_LINUX)
#include "pulsecapturethread.h"
#elif defined(Q_OS_WIN)
#include "wincapturethread.h"
#elif defined(Q_OS_MAC)
#include "maccapturethread.h"
#endif

//--------------------------------------------------------------------------------------------------

CaptureThread *AudioFactory::createThread(QObject *parent)
{
#if defined(Q_OS_LINUX)
    return new PulseCaptureThread(parent);
#elif defined(Q_OS_WIN)
    return new WinCaptureThread(parent);
#elif defined(Q_OS_MAC)
    return new MacCaptureThread(parent);
#else
    return nullptr;
#endif
}

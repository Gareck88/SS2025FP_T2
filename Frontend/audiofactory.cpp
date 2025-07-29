#include "audiofactory.h"
#include <qglobal.h>

#if defined(Q_OS_LINUX)
#include "pulsecapturethread.h"
#elif defined(Q_OS_WIN)
#include "wincapturethread.h"
#endif

//--------------------------------------------------------------------------------------------------

CaptureThread *AudioFactory::createThread (
    QObject *parent)
{
#if defined(Q_OS_LINUX)
    return new PulseCaptureThread (parent);
#elif defined(Q_OS_WIN)
    return new WinCaptureThread (parent);
#else
    return nullptr;
#endif
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      SignalDrivenThread.cpp
 * \brief     source file for general thread ( for camera hal2 implementation )
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/05/31
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 */
//#define LOG_NDEBUG 0
#define LOG_TAG "SignalDrivenThread"
#include <utils/Log.h>

#include "SignalDrivenThread.h"

namespace android {


SignalDrivenThread::SignalDrivenThread()
{
}

SignalDrivenThread::SignalDrivenThread(const char* name,
                            int32_t priority, size_t stack)
    :Thread(false)
{
    ALOGV("DEBUG(%s):", __func__);
    m_processingSignal = 0;
    m_receivedSignal = 0;
    run(name, priority, stack);
    return;
}

SignalDrivenThread::~SignalDrivenThread()
{
    ALOGV("DEBUG(%s):", __func__);
    return;
}

status_t SignalDrivenThread::SetSignal(uint32_t signal)
{
    ALOGV("DEBUG(%s):Setting Signal (%x)", __func__, signal);

    Mutex::Autolock lock(m_signalMutex);
    ALOGV("DEBUG(%s):Signal Set     (%x) - prev(%x)", __func__, signal, m_receivedSignal);
    m_receivedSignal |= signal;
    m_threadCondition.signal();
    return NO_ERROR;
}

uint32_t SignalDrivenThread::GetProcessingSignal()
{
    ALOGV("DEBUG(%s): Signal (%x)", __func__, m_processingSignal);

    return m_processingSignal;
}

/*
void SignalDrivenThread::ClearProcessingSignal(uint32_t signal)
{
    ALOGV("DEBUG(%s):Clearing Signal (%x) from (%x)", __func__, signal, m_processingSignal);

    m_processingSignal &= ~(signal);
    return;
}
*/

status_t SignalDrivenThread::readyToRun()
{
    ALOGV("DEBUG(%s):", __func__);
    return readyToRunInternal();
}


bool SignalDrivenThread::threadLoop()
{
    {
        Mutex::Autolock lock(m_signalMutex);
        ALOGV("DEBUG(%s):Waiting Signal", __func__);
        while (!m_receivedSignal)
        {
            m_threadCondition.wait(m_signalMutex);
        }
        m_processingSignal = m_receivedSignal;
        m_receivedSignal = 0;
    }
    ALOGV("DEBUG(%s):Got Signal (%x)", __func__, m_processingSignal);

    if (m_processingSignal & SIGNAL_THREAD_TERMINATE)
    {
        ALOGV("DEBUG(%s):Thread Terminating", __func__);
        return (false);
    }
    else if (m_processingSignal & SIGNAL_THREAD_PAUSE)
    {
        ALOGV("DEBUG(%s):Thread Paused", __func__);
        return (true);
    }

    threadLoopInternal();
    return (true);
}


}; // namespace android

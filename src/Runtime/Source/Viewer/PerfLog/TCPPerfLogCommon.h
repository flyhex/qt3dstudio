/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//==============================================================================
//	Includes
//==============================================================================
#include <stdio.h>

//==============================================================================
//	Structs
//==============================================================================
typedef struct _SPerfLogData
{
    unsigned long m_Timestamp;
    float m_FPS;
    unsigned long m_GPUMemoryUsage;
    unsigned long m_CPUMemoryUsage;
    float m_CPULoad;
    long m_GPULoad;
    float m_EMCLoad;
    char m_StringData[128];
} SPerfLogData;

typedef struct _SPerfLogDataExtra
{
    float m_MinFPS;
    float m_MaxFPS;
    float m_MinCPULoad;
    float m_MaxCPULoad;
    long m_MinGPULoad;
    long m_MaxGPULoad;
} SPerfLogDataExtra;

//==============================================================================
/**
 *	Formats SPerfLogData into a user readable form.
 */
const char *FormatPerfLogData(const SPerfLogData *inData)
{
    static char theMessageBuffer[256];
    theMessageBuffer[0] = 0;

    static unsigned long thePrevTimestamp = 0;
    static unsigned long theTimestampDiff = 0;
    if (thePrevTimestamp != 0)
        theTimestampDiff = inData->m_Timestamp - thePrevTimestamp;

    ::sprintf(theMessageBuffer, "Timestamp: %lu\tTimediff: %lu\tFPS: %.2f\tUsecase: "
                                "%s\tGPUMemoryUsage: %lu\tCPUMemoryUsage: %lu\tCPULoad: "
                                "%.2f\tGPULoad: %ld\tEMCLoad: %.2f\n",
              inData->m_Timestamp, theTimestampDiff, inData->m_FPS, inData->m_StringData,
              inData->m_GPUMemoryUsage, inData->m_CPUMemoryUsage, inData->m_CPULoad * 100,
              inData->m_GPULoad, inData->m_EMCLoad * 100);

    thePrevTimestamp = inData->m_Timestamp;

    return theMessageBuffer;
}

//==============================================================================
/**
 *	Formats SPerfLogDataExtra into a user readable form.
 */
void FormatPerfLogDataExtra(const SPerfLogData *inData, const SPerfLogDataExtra *inExtraData,
                            char *inMessageBuffer)
{
    ::sprintf(inMessageBuffer, "FPS: %.0f/%.0f/%.0f CPU: %.0f/%.0f/%.0f GPU: %ld/%ld/%ld EMC: %.0f",
              inData->m_FPS, inExtraData->m_MaxFPS, inExtraData->m_MinFPS, inData->m_CPULoad * 100,
              inExtraData->m_MaxCPULoad * 100, inExtraData->m_MinCPULoad * 100, inData->m_GPULoad,
              inExtraData->m_MaxGPULoad, inExtraData->m_MinGPULoad, inData->m_EMCLoad * 100);
}

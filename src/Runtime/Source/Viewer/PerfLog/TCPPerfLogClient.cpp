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
#include "TCPPerfLogClient.h"
#include "UICMacros.h"
#include "UICPlatformSpecific.h"
#include <stdio.h>
#include <string.h>

//==============================================================================
//	Statics
//==============================================================================
static TCPPerfLogClient *s_PerfLogClient = NULL;
static SPerfLogDataExtra s_LogDataExtra = { 0 };
static const KDust s_OneSec = 1000000000;

//==============================================================================
/**
 *	Extern function to create and initialize the TCPPerfLogClient.
 */
int InitializePerfLogClient(const char *inServerAddress, const char *inPort, KDust inLogFreqMsec,
                            const char *inLocalLogFilename)
{
    int theReturn = 1;

    if (!s_PerfLogClient) {
        s_PerfLogClient =
            new TCPPerfLogClient(inServerAddress, inPort, inLogFreqMsec, inLocalLogFilename);
        theReturn = s_PerfLogClient->IsConnected() || s_PerfLogClient->IsLocal();
    }

    return theReturn;
}

void PerfLogMarkBegin(KDust inCurrentTimeMsec)
{
    if (s_PerfLogClient)
        s_PerfLogClient->MarkLogBegin(inCurrentTimeMsec);
}

void PerfLogMarkEnd(KDust inCurrentTimeMsec)
{
    if (s_PerfLogClient)
        s_PerfLogClient->MarkLogEnd(inCurrentTimeMsec);
}

void InitializeNVPerfMon(EGLDisplay inDisplay)
{
    if (s_PerfLogClient)
        s_PerfLogClient->InitializeNVPerfMon(inDisplay);
}

void CleanupPerfLogClient(EGLDisplay inDisplay)
{
    if (s_PerfLogClient)
        s_PerfLogClient->CleanupPerfLogClient(inDisplay);

    delete s_PerfLogClient;
}

void PerfLogGetShortDisplay(char *outMessageBuffer)
{
    if (s_PerfLogClient)
        s_PerfLogClient->GetShortDisplay(outMessageBuffer);
}

//==============================================================================
/**
 *	Extern function to send a log given a timestamp and the currently measured
 *  FPS
 */
void PerfLogSend(unsigned long inTimeStamp, float inFPS)
{
    if (s_PerfLogClient)
        s_PerfLogClient->SendLog(inTimeStamp, inFPS);
}

//==============================================================================
/**
 *	Extern function to set string data into the log packet.
 */
void PerfLogSetStringData(const char *inStringData)
{
    if (s_PerfLogClient)
        s_PerfLogClient->SetStringData(inStringData);
}

//==============================================================================
/**
 *	CTOR
 *  Attempts to make a connection to a specified server address and port.
 */
TCPPerfLogClient::TCPPerfLogClient(const char *inServerAddress, const char *inServerPort,
                                   KDust inLogFreqUSec, const char *inLocalLogFilename)
    : m_Socket(sizeof(SPerfLogData), true)
    , m_ProcMeminfo(NULL)
    , m_ProcStatinfo(NULL)
    , m_NvRmHandle(NULL)
    , m_Connected(false)
    , m_SendLog(false)
    , m_LocalLogFile(NULL)
    , m_RequiredPasses(1)
    , m_CurrentPass(0)
    , m_FrameCount(0)
    , m_LogFreq(inLogFreqUSec)
    , m_LastLogTime(0)
    , m_LastExtraLogTime(0)
    , m_NVPerfMonitor(NULL)
    , m_NVGPUIdleCounter(NULL)
{
    //::memset( &m_Data, 0, sizeof( m_Data) );
    m_Connected = m_Socket.Connect(inServerAddress, inServerPort);

#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
    // Open the meminfo procfs
    m_ProcMeminfo = ::fopen("/proc/meminfo", "r");
    ::setvbuf(m_ProcMeminfo, NULL, _IONBF, 0);

    // Open the stat procfs
    m_ProcStatinfo = ::fopen("/proc/stat", "r");
    ::setvbuf(m_ProcStatinfo, NULL, _IONBF, 0);
#endif

#ifdef _TEGRA_LINUX
    // Get a handle into vrm
    NvRmOpen(m_NvRmHandle, 0);

    // Activate DFS so that we can retrieve the EMC utilization
    if (NvRmDfsSetState(m_NvRmHandle, NvRmDfsRunState_ClosedLoop) != 0)
        kdLogMessage("\nUnable to activate DFS");
#endif

    if (inLocalLogFilename)
        m_LocalLogFile = ::fopen(inLocalLogFilename, "w");
}

//==============================================================================
/**
 *	DTOR
 *  Closes opened handles.
 */
TCPPerfLogClient::~TCPPerfLogClient()
{
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
    fclose(m_ProcMeminfo);
    fclose(m_ProcStatinfo);
#endif

#ifdef _TEGRA_LINUX
    NvRmClose(m_NvRmHandle);
#endif
    m_Socket.Disconnect();

    if (m_LocalLogFile)
        ::fclose(m_LocalLogFile);
}

//==============================================================================
/**
 *	Marks the beginning of perf logging.
 */
void TCPPerfLogClient::MarkLogBegin(KDust inCurrentTime)
{
    if (m_LogFreq > 0) {
        if ((inCurrentTime - m_LastLogTime) > m_LogFreq) {
            m_SendLog = true; // Signal to initiate logging (might required a few frames before log
                              // data is fully captured)

            // Starts the NV perf monitor for this frame
            if (m_NVPerfMonitor)
                m_NVPerfMonFuncs.eglPerfMonitorBeginExperimentNV();
        }

        if (m_SendLog) {
            if (m_NVPerfMonitor)
                m_NVPerfMonFuncs.eglPerfMonitorBeginPassNV(m_CurrentPass % m_RequiredPasses);
        }

        if ((inCurrentTime - m_LastExtraLogTime) > s_OneSec) {
            SPerfLogData *theData = static_cast<SPerfLogData *>(m_Socket.GetData());
            s_LogDataExtra.m_MinFPS = s_LogDataExtra.m_MaxFPS = theData->m_FPS;
            s_LogDataExtra.m_MinCPULoad = s_LogDataExtra.m_MaxCPULoad = theData->m_CPULoad;
            s_LogDataExtra.m_MinGPULoad = s_LogDataExtra.m_MaxGPULoad = theData->m_GPULoad;
            m_LastExtraLogTime = inCurrentTime;
        }
    }
}

//==============================================================================
/**
 *	Marks the end of perf logging.
 */
void TCPPerfLogClient::MarkLogEnd(KDust inCurrentTime)
{
    ++m_FrameCount;

    if (m_SendLog) {
        if (m_NVPerfMonitor) {
            // Signal the end of NV perf logging for this frame
            m_NVPerfMonFuncs.eglPerfMonitorEndPassNV();
            ++m_CurrentPass;
        }

        if ((m_CurrentPass % m_RequiredPasses == 0)) {
            if (m_NVPerfMonitor)
                m_NVPerfMonFuncs.eglPerfMonitorEndExperimentNV();

            KDust theElaspedTime = inCurrentTime - m_LastLogTime;
            SendLog(static_cast<unsigned long>(inCurrentTime / 1000000),
                    static_cast<float>(m_FrameCount / (theElaspedTime / 1000000000.0)));
            m_LastLogTime = inCurrentTime;
            m_FrameCount = 0;
            m_SendLog = false;
        }
    }
}

//==============================================================================
/**
 *	Sets timestamp and FPS into log and calls GatherStatistics.
 *  Sends log data into the socket.
 */
bool TCPPerfLogClient::SendLog(unsigned long inTimestamp, float inFPS)
{
    SPerfLogData *theData = static_cast<SPerfLogData *>(m_Socket.GetData());
    theData->m_Timestamp = inTimestamp;
    theData->m_FPS = inFPS;
    GatherStatistics(theData);

    s_LogDataExtra.m_MinFPS = Q3DStudio::Q3DStudio_min(inFPS, s_LogDataExtra.m_MinFPS);
    s_LogDataExtra.m_MaxFPS = Q3DStudio::Q3DStudio_max(inFPS, s_LogDataExtra.m_MaxFPS);

    s_LogDataExtra.m_MinCPULoad =
        Q3DStudio::Q3DStudio_min(theData->m_CPULoad, s_LogDataExtra.m_MinCPULoad);
    s_LogDataExtra.m_MaxCPULoad =
        Q3DStudio::Q3DStudio_max(theData->m_CPULoad, s_LogDataExtra.m_MaxCPULoad);

    s_LogDataExtra.m_MinGPULoad =
        Q3DStudio::Q3DStudio_min(theData->m_GPULoad, s_LogDataExtra.m_MinGPULoad);
    s_LogDataExtra.m_MaxGPULoad =
        Q3DStudio::Q3DStudio_max(theData->m_GPULoad, s_LogDataExtra.m_MaxGPULoad);

    bool theResult = false;

    if (m_LocalLogFile)
        theResult |= ::fputs(FormatPerfLogData(theData), m_LocalLogFile) > 0;

    if (IsConnected())
        theResult |= m_Socket.Send() > 0;

    return theResult;
}

//==============================================================================
/**
 *	Sets string data into the next log packet to be sent.
 */
void TCPPerfLogClient::SetStringData(const char *inStringData)
{
    SPerfLogData *theData = static_cast<SPerfLogData *>(m_Socket.GetData());
    ::strcpy(theData->m_StringData, inStringData);
}

//==============================================================================
/**
 *	Obtain memory usage using meminfo procfs.
 */
void TCPPerfLogClient::GetCPUMemoryUsage(unsigned long &outUsage)
{
    unsigned long theTotalMem = 0;
    unsigned long theFreeMem = 0;

#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
    rewind(m_ProcMeminfo);
    fflush(m_ProcMeminfo);

    fscanf(m_ProcMeminfo, "MemTotal: %lu kB\nMemFree: %lu kB", &theTotalMem, &theFreeMem);
#endif

    outUsage = theTotalMem - theFreeMem;
}

//==============================================================================
/**
 *	Obtain GPU memory usage from nvrm.
 */
void TCPPerfLogClient::GetGPUCarveoutUsage(unsigned long &outUsage)
{
    long theUsedCarveout = 0;

#ifdef _TEGRA_LINUX
    NvRmMemGetStat(NvRmMemStat_UsedCarveout, theUsedCarveout);
#endif

    outUsage = theUsedCarveout;
}

//==============================================================================
/**
 *	Obtain CPU utilization from nvrm DFS.
 */
void TCPPerfLogClient::GetCPULoad(float &outPercentage)
{
#ifdef _LINUXPLATFORM
    // Get CPU load from /proc/stat
    static SCPUInfo thePreviousInfo;
    SCPUInfo theCurrentInfo;

    rewind(m_ProcStatinfo);
    fflush(m_ProcStatinfo);

    // Read first line in /proc/stat which is a summary for all CPU cores on the system
    fscanf(m_ProcStatinfo, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", &theCurrentInfo.m_User,
           &theCurrentInfo.m_Nice, &theCurrentInfo.m_System, &theCurrentInfo.m_Idle,
           &theCurrentInfo.m_Wait, &theCurrentInfo.m_X, &theCurrentInfo.m_Y, &theCurrentInfo.m_Z);

    // Idle frames calculation
    unsigned long theIdleFrames = theCurrentInfo.m_Idle - thePreviousInfo.m_Idle;
    if (theIdleFrames < 0)
        theIdleFrames = 0; // Appears to be a bug in 2.4.x kernels??

    // Get the total number of timeslices
    unsigned long theTotalFrames = (theCurrentInfo.m_User - thePreviousInfo.m_User)
        + (theCurrentInfo.m_Nice - thePreviousInfo.m_Nice)
        + (theCurrentInfo.m_System - thePreviousInfo.m_System) + theIdleFrames
        + (theCurrentInfo.m_Wait - thePreviousInfo.m_Wait)
        + (theCurrentInfo.m_X - thePreviousInfo.m_X) + (theCurrentInfo.m_Y - thePreviousInfo.m_Y)
        + (theCurrentInfo.m_Z - thePreviousInfo.m_Z);

    if (theTotalFrames < 1)
        theTotalFrames = 1;

    outPercentage = 1.0f - (theIdleFrames * 1.0f / theTotalFrames);

    thePreviousInfo = theCurrentInfo; // /proc/stat is an aggregation, so track this for the next
                                      // time GetCPULoad is called

#else
    outPercentage = -.01f;
#endif
}

//==============================================================================
/**
 *	Retrieve the GPU load performance counter
 */
void TCPPerfLogClient::GetGPULoad(long &outPercentage)
{
    EGLuint64NV theCycles = 0;
    EGLuint64NV theGPUIdle = 0;

    if (m_NVPerfMonitor
        && (m_NVPerfMonFuncs.eglGetPerfMarkerCounterNV(EGL_DEFAULT_PERFMARKER_NV,
                                                       m_NVGPUIdleCounter, &theGPUIdle, &theCycles)
            == EGL_TRUE))
        outPercentage = static_cast<long>(100 - theGPUIdle);
    else
        outPercentage = 0;
}

//==============================================================================
/**
 *	Retrieve the external memory controller load.
 *  DFS must be running first.
 */
void TCPPerfLogClient::GetEMCLoad(float &outPercentage)
{
// Get CPU clock utilization from DFS
#ifdef _TEGRA_LINUX
    NvRmDfsClockUsage theUsage;
    ::memset(&theUsage, 0, sizeof(theUsage));
    if (NvRmDfsGetClockUtilization(m_NvRmHandle, NvRmDfsClockId_Emc, theUsage) == 0)
        outPercentage = theUsage.AverageKHz * 1.0f / theUsage.CurrentKHz;
    else
#endif
        outPercentage = -0.01f;
}

//==============================================================================
/**
 *	Helper function to gather various statistics.
 */
void TCPPerfLogClient::GatherStatistics(SPerfLogData *inLogData)
{
    // Grab and set statistics here
    GetGPUCarveoutUsage(inLogData->m_GPUMemoryUsage);
    GetCPUMemoryUsage(inLogData->m_CPUMemoryUsage);
    GetCPULoad(inLogData->m_CPULoad);
    GetGPULoad(inLogData->m_GPULoad);
    GetEMCLoad(inLogData->m_EMCLoad);
}

//==============================================================================
/**
 *	Initialize NV perf monitor counters
 *	Currently only one counter requested (GPU Idle)
 */
void TCPPerfLogClient::InitializeNVPerfMon(EGLDisplay inDisplay)
{
#ifdef _TEGRA_LINUX
    EGLPerfCounterNV theCounters[50];
    EGLint theReturnedCounterSize = 0;

    m_NVPerfMonitor = m_NVPerfMonFuncs.eglCreatePerfMonitorNV(
        inDisplay, EGL_PERFMONITOR_HARDWARE_COUNTERS_BIT_NV);
    m_NVPerfMonFuncs.eglMakeCurrentPerfMonitorNV(m_NVPerfMonitor);

    m_NVPerfMonFuncs.eglGetPerfCountersNV(m_NVPerfMonitor, theCounters, 100,
                                          &theReturnedCounterSize);
    for (long theIndex = 0; theIndex < theReturnedCounterSize; ++theIndex) {
        const char *theCounterName = m_NVPerfMonFuncs.eglQueryPerfCounterStringNV(
            m_NVPerfMonitor, theCounters[theIndex], EGL_COUNTER_NAME_NV);

        // Look for a counter with the name 'gpu idle'
        if (Q3DStudio_stricmp(theCounterName, "GPU Idle") == 0) {
            m_NVPerfMonFuncs.eglPerfMonitorAddCountersNV(1, &theCounters[theIndex]);
            m_NVGPUIdleCounter = theCounters[theIndex];
            break;
        }
    }

    m_NVPerfMonFuncs.eglValidatePerfMonitorNV(&m_RequiredPasses);
#else
    UNREFERENCED_PARAMETER(inDisplay);
#endif
}

//==============================================================================
/**
 *	Deinitializes the NV perf monitor
 */
void TCPPerfLogClient::CleanupPerfLogClient(EGLDisplay inDisplay)
{
    if (m_NVPerfMonitor)
        m_NVPerfMonFuncs.eglDestroyPerfMonitorNV(inDisplay, m_NVPerfMonitor);
}

//==============================================================================
/**
 *	Retrieve a formatted output string containing performance statistics
 */
void TCPPerfLogClient::GetShortDisplay(char *inMessageBuffer)
{
    SPerfLogData *theData = static_cast<SPerfLogData *>(m_Socket.GetData());
    FormatPerfLogDataExtra(theData, &s_LogDataExtra, inMessageBuffer);
}

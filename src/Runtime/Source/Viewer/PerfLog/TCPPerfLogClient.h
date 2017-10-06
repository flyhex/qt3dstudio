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

#pragma once
//==============================================================================
//	Includes
//==============================================================================
#include <stdio.h>
#include <EGL/egl.h>
#include "SimpleTCPClientSocket.h"
#include "TCPPerfLogCommon.h"
#include "TCPPerfLogClientStub.h"

#ifdef _PCPLATFORM
#include <EGL/eglext.h>
#endif

//==============================================================================
//	Header definations copied from nvrm headers
//==============================================================================
extern "C" {

/**
 * NvRm heap statistics. See NvRmMemGetStat() for further details.
 */
enum NvRmMemStat {
    /**
     * Total number of bytes reserved for the carveout heap.
     */
    NvRmMemStat_TotalCarveout = 1,
    /**
     * Number of bytes used in the carveout heap.
     */
    NvRmMemStat_UsedCarveout,
    /**
     * Size of the largest free block in the carveout heap.
     * Size can be less than the difference of total and
     * used memory.
     */
    NvRmMemStat_LargestFreeCarveoutBlock,
    /**
     * Total number of bytes in the GART heap.
     */
    NvRmMemStat_TotalGart,
    /**
     * Number of bytes reserved from the GART heap.
     */
    NvRmMemStat_UsedGart,
    /**
     * Size of the largest free block in GART heap. Size can be
     * less than the difference of total and used memory.
     */
    NvRmMemStat_LargestFreeGartBlock,
};

/**
 * Defines SOC-wide clocks controlled by Dynamic Frequency Scaling (DFS)
 * that can be targeted by Starvation and Busy hints
 */
enum NvRmDfsClockId {
    /// Specifies CPU clock
    NvRmDfsClockId_Cpu = 1,

    /// Specifies AVP clock
    NvRmDfsClockId_Avp,

    /// Specifies System bus clock
    NvRmDfsClockId_System,

    /// Specifies AHB bus clock
    NvRmDfsClockId_Ahb,

    /// Specifies APB bus clock
    NvRmDfsClockId_Apb,

    /// Specifies video pipe clock
    NvRmDfsClockId_Vpipe,

    /// Specifies external memory controller clock
    NvRmDfsClockId_Emc,
};

typedef unsigned long NvRmFreqKHz;

/**
 * Holds information on DFS clock domain utilization
 */
struct NvRmDfsClockUsage
{
    /// Minimum clock domain frequency
    NvRmFreqKHz MinKHz;

    /// Maximum clock domain frequency
    NvRmFreqKHz MaxKHz;

    /// Low corner frequency - current low boundary for DFS control algorithm.
    /// Can be dynamically adjusted via APIs: NvRmDfsSetLowCorner() for all DFS
    /// domains, NvRmDfsSetCpuEnvelope() for CPU, and NvRmDfsSetEmcEnvelope()
    /// for EMC. When all DFS domains hit low corner, DFS stops waking up CPU
    ///  from low power state.
    NvRmFreqKHz LowCornerKHz;

    /// High corner frequency - current high boundary for DFS control algorithm.
    /// Can be dynamically adjusted via APIs: NvRmDfsSetCpuEnvelope() for Cpu,
    /// NvRmDfsSetEmcEnvelope() for Emc, and NvRmDfsSetAvHighCorner() for other
    //  DFS domains.
    NvRmFreqKHz HighCornerKHz;

    /// Current clock domain frequency
    NvRmFreqKHz CurrentKHz;

    /// Average frequency of domain *activity* (not average frequency). For
    /// domains that do not have activity monitors reported as unspecified.
    NvRmFreqKHz AverageKHz;
};

/**
 * Defines DFS manager run states
 */
enum NvRmDfsRunState {
    /// DFS is in invalid, not initialized state
    NvRmDfsRunState_Invalid = 0,

    /// DFS is disabled / not supported (terminal state)
    NvRmDfsRunState_Disabled = 1,

    /// DFS is stopped - no automatic clock control. Starvation and Busy hints
    /// are recorded but have no affect.
    NvRmDfsRunState_Stopped,

    /// DFS is running in closed loop - full automatic control of SoC-wide
    /// clocks based on clock activity measuremnets. Starvation and Busy hints
    /// are functional as well.
    NvRmDfsRunState_ClosedLoop,

    /// DFS is running in closed loop with profiling (can not be set on non
    /// profiling build).
    NvRmDfsRunState_ProfiledLoop,
};

typedef void *NvRmDeviceHandle;

NvRmDfsRunState NvRmDfsGetState(NvRmDeviceHandle inRmDeviceHandle);
long NvRmDfsSetState(NvRmDeviceHandle inRmDeviceHandle, NvRmDfsRunState inDfsRunState);

/**
 * Gets information on DFS controlled clock utilization. If DFS is stopped
 * or disabled the average frequency is always equal to current frequency.
 *
 * @param hRmDeviceHandle The RM device handle.
 * @param ClockId The DFS ID of the clock targeted by this request.
 * @param pClockInfo Output storage pointer for clock utilization information.
 *
 * @return NvSuccess if clock usage information is returned successfully.
 */
// NvError
// NvRmDfsGetClockUtilization(
//    [in] NvRmDeviceHandle hRmDeviceHandle,
//    [in] NvRmDfsClockId ClockId,
//    [out] NvRmDfsClockUsage pClockUsage);
long NvRmDfsGetClockUtilization(NvRmDeviceHandle inRmDeviceHandle, NvRmDfsClockId inClockId,
                                NvRmDfsClockUsage &inClockUsage);

/**
 * Get a memory statistics value.
 *
 * Querying values may have an effect on  system performance and may include
 * processing, like heap traversal.
 *
 * @param Stat NvRmMemStat value that chooses the value to return.
 * @param Result Result, if the call was successful. Otherwise value
 *      is not touched.
 * @returns NvSuccess on success, NvError_BadParameter if Stat is
 *      not a valid value, NvError_NotSupported if the Stat is
 *      not available for some reason, or
 *      NvError_InsufficientMemory.
 */
// NvError
// NvRmMemGetStat([in] NvRmMemStat Stat, [out] NvS32 Result);
long NvRmMemGetStat(NvRmMemStat inStat, long &outResult);

/**
 * Opens the Resource Manager for a given device.
 *
 * Can be called multiple times for a given device.  Subsequent
 * calls will not necessarily return the same handle.  Each call to
 * NvRmOpen() must be paired with a corresponding call to NvRmClose().
 *
 * Assert encountered in debug mode if DeviceId value is invalid.
 *
 * This call is not intended to perform any significant hardware
 * initialization of the device; rather its primary purpose is to
 * initialize RM's internal data structures that are involved in
 * managing the device.
 *
 * @param pHandle the RM handle is stored here.
 * @param DeviceId implementation-dependent value specifying the device
 *     to be opened.  Currently must be set to zero.
 *
 * @retval NvSuccess Indicates that RM was successfully opened.
 * @retval NvError_InsufficientMemory Indicates that RM was unable to allocate
 *     memory for its internal data structures.
 */
// NvError NvRmOpen( [out] NvRmDeviceHandle pHandle, [in] NvU32 DeviceId );
long NvRmOpen(NvRmDeviceHandle &inHandle, unsigned long outDeviceId);

/**
 * Closes the Resource Manager for a given device.
 *
 * Each call to NvRmOpen() must be paired with a corresponding call
 * to NvRmClose().
 *
 * @param hDevice The RM handle.  If hDevice is NULL, this API has no effect.
 */
// void NvRmClose( [in] NvRmDeviceHandle hDevice );
void NvRmClose(void *inDevice);

} // extern "C"

// Represents information obtained from /proc/stat
typedef struct _SCPUInfo
{
    unsigned long m_User; // timeslices in userspace
    unsigned long m_Nice; // timeslices for niced processed
    unsigned long m_System; // timeslices in kernelspace
    unsigned long m_Idle; // timeslices in idle
    unsigned long m_Wait; // timeslices in wait
    unsigned long m_X; // other timeslices
    unsigned long m_Y; // other timeslices
    unsigned long m_Z; // other timeslices

    _SCPUInfo()
        : m_User(0)
        , m_Nice(0)
        , m_System(0)
        , m_Idle(0)
        , m_Wait(0)
        , m_X(0)
        , m_Y(0)
        , m_Z(0)
    {
    }

} SCPUInfo;

//==============================================================================
// Utility function to grab EGL function pointers for NV perf monitor.
//==============================================================================
typedef struct _SNVPerfMonitorFuncs
{
    PFNEGLCREATEPERFMONITORNVPROC eglCreatePerfMonitorNV;
    PFNEGLGETCURRENTPERFMONITORNVPROC eglGetCurrentPerfMonitorNV;
    PFNEGLDESTROYPERFMONITORNVPROC eglDestroyPerfMonitorNV;
    PFNEGLGETPERFCOUNTERSNVPROC eglGetPerfCountersNV;
    PFNEGLQUERYPERFCOUNTERSTRINGNVPROC eglQueryPerfCounterStringNV;
    PFNEGLPERFMONITORADDCOUNTERSNVPROC eglPerfMonitorAddCountersNV;
    PFNEGLMAKECURRENTPERFMONITORNVPROC eglMakeCurrentPerfMonitorNV;
    PFNEGLGETPERFMARKERCOUNTERNVPROC eglGetPerfMarkerCounterNV;
    PFNEGLPERFMONITORBEGINEXPERIMENTNVPROC eglPerfMonitorBeginExperimentNV;
    PFNEGLPERFMONITORENDEXPERIMENTNVPROC eglPerfMonitorEndExperimentNV;
    PFNEGLPERFMONITORBEGINPASSNVPROC eglPerfMonitorBeginPassNV;
    PFNEGLPERFMONITORENDPASSNVPROC eglPerfMonitorEndPassNV;
    PFNEGLVALIDATEPERFMONITORNVPROC eglValidatePerfMonitorNV;

    _SNVPerfMonitorFuncs()
    {
        eglCreatePerfMonitorNV =
            (PFNEGLCREATEPERFMONITORNVPROC)eglGetProcAddress("eglCreatePerfMonitorNV");
        eglDestroyPerfMonitorNV =
            (PFNEGLDESTROYPERFMONITORNVPROC)eglGetProcAddress("eglDestroyPerfMonitorNV");
        eglGetCurrentPerfMonitorNV =
            (PFNEGLGETCURRENTPERFMONITORNVPROC)eglGetProcAddress("eglGetCurrentPerfMonitorNV");
        eglGetPerfCountersNV =
            (PFNEGLGETPERFCOUNTERSNVPROC)eglGetProcAddress("eglGetPerfCountersNV");
        eglQueryPerfCounterStringNV =
            (PFNEGLQUERYPERFCOUNTERSTRINGNVPROC)eglGetProcAddress("eglQueryPerfCounterStringNV");
        eglPerfMonitorAddCountersNV =
            (PFNEGLPERFMONITORADDCOUNTERSNVPROC)eglGetProcAddress("eglPerfMonitorAddCountersNV");
        eglMakeCurrentPerfMonitorNV =
            (PFNEGLMAKECURRENTPERFMONITORNVPROC)eglGetProcAddress("eglMakeCurrentPerfMonitorNV");
        eglGetPerfMarkerCounterNV =
            (PFNEGLGETPERFMARKERCOUNTERNVPROC)eglGetProcAddress("eglGetPerfMarkerCounterNV");
        eglPerfMonitorBeginExperimentNV = (PFNEGLPERFMONITORBEGINEXPERIMENTNVPROC)eglGetProcAddress(
            "eglPerfMonitorBeginExperimentNV");
        eglPerfMonitorEndExperimentNV = (PFNEGLPERFMONITORENDEXPERIMENTNVPROC)eglGetProcAddress(
            "eglPerfMonitorEndExperimentNV");
        eglPerfMonitorBeginPassNV =
            (PFNEGLPERFMONITORBEGINPASSNVPROC)eglGetProcAddress("eglPerfMonitorBeginPassNV");
        eglPerfMonitorEndPassNV =
            (PFNEGLPERFMONITORENDPASSNVPROC)eglGetProcAddress("eglPerfMonitorEndPassNV");
        eglValidatePerfMonitorNV =
            (PFNEGLVALIDATEPERFMONITORNVPROC)eglGetProcAddress("eglValidatePerfMonitorNV");
    }

} SNVPerfMonitorFuncs;

//==============================================================================
//	Handles gathering of performance statistics and sending them.
//  Attempts to make a connection to a server that it will send data to.
//==============================================================================
class TCPPerfLogClient
{
public:
    TCPPerfLogClient(const char *inServerAddress, const char *inServerPort, KDust inLogFreqUSec,
                     const char *inLocalLogFilename);
    ~TCPPerfLogClient();

public:
    void MarkLogBegin(KDust inCurrentTime);
    void MarkLogEnd(KDust inCurrentTime);

    bool SendLog(unsigned long inTimeStamp, float inFPS);
    void SetStringData(const char *inStringData);
    bool IsConnected() { return m_Connected; }
    bool IsLocal() { return m_LocalLogFile != NULL; }
    void InitializeNVPerfMon(EGLDisplay inDisplay);
    void CleanupPerfLogClient(EGLDisplay inDisplay);
    void GetShortDisplay(char *inMessageBuffer);

protected:
    void GatherStatistics(SPerfLogData *inLogData);
    void GetCPUMemoryUsage(unsigned long &outUsage);
    void GetGPUCarveoutUsage(unsigned long &outUsage);
    void GetCPULoad(float &outPercentage);
    void GetGPULoad(long &outPercentage);
    void GetEMCLoad(float &outPercentage);
    void GetGPUWaits(long &outPercentage);

protected:
    SimpleTCPClientSocket m_Socket;
    FILE *m_ProcMeminfo;
    FILE *m_ProcStatinfo;
    void *m_NvRmHandle;
    bool m_Connected;
    bool m_SendLog;
    FILE *m_LocalLogFile;

    EGLint m_RequiredPasses;
    EGLint m_CurrentPass;

    long m_FrameCount;
    KDust m_LogFreq;
    KDust m_LastLogTime;
    KDust m_LastExtraLogTime;

    const SNVPerfMonitorFuncs m_NVPerfMonFuncs;
    EGLPerfMonitorNV m_NVPerfMonitor;
    EGLPerfCounterNV m_NVGPUIdleCounter;
};

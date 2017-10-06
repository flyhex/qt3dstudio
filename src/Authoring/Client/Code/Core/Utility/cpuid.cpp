/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "stdafx.h"
#include "cpuid.h"

extern "C" {

// These are the bit flags that get set on calling cpuid
// with register eax set to 1
#define _MMX_FEATURE_BIT 0x00800000
#define _SSE_FEATURE_BIT 0x02000000
#define _SSE2_FEATURE_BIT 0x04000000

// This bit is set when cpuid is called with
// register set to 80000001h (only applicable to AMD)
#define _3DNOW_FEATURE_BIT 0x80000000


#ifndef _WIN32
void __cpuid(int cpuInfo[4], int function_id)
{
    asm volatile
          ("cpuid" : "=a" (cpuInfo[0]), "=b" (cpuInfo[1]), "=c" (cpuInfo[2]), "=d" (cpuInfo[3])
           : "a" (function_id), "c" (0));
}
#endif

int IsCPUID()
{
    int CPUInfo[4];
    int nIds;

    // __cpuid with an InfoType argument of 0 returns the number of
    // valid Ids in CPUInfo[0]
    __cpuid(CPUInfo, 0);
    nIds = CPUInfo[0];

    return nIds;
}

/***
* int _os_support(int feature)
*   - Checks if OS Supports the capablity or not
*
* Entry:
*   feature: the feature we want to check if OS supports it.
*
* Exit:
*   Returns 1 if OS support exist and 0 when OS doesn't support it.
*
****************************************************************/

int _os_support(int feature)
{
#if defined(_AMD64_)
    return 1;
#elif defined(__clang__)
    switch (feature) {
#ifdef __SSE__
    case _CPU_FEATURE_SSE:
        return 1;
#endif
#ifdef __SSE2__
    case _CPU_FEATURE_SSE2:
        return 1;
#endif
#ifdef __SSE3__
    case _CPU_FEATURE_3DNOW:
        return 1;
#endif
    default:
        return 0;
    }
#else
    __try {
        switch (feature) {
        case _CPU_FEATURE_SSE:
            __asm {
                xorps xmm0, xmm0 // executing SSE instruction
            }
            break;
        case _CPU_FEATURE_SSE2:
            __asm {
                xorpd xmm0, xmm0 // executing SSE2 instruction
            }
            break;
        case _CPU_FEATURE_3DNOW:
            __asm {
                pfrcp mm0, mm0 // executing 3DNow! instruction
                emms
            }
            break;
        case _CPU_FEATURE_MMX:
            __asm {
                pxor mm0, mm0 // executing MMX instruction
                emms
            }
            break;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION) {
            return 0;
        }
        return 0;
    }
    return 1;
#endif
}

/***
*
* void map_mname(int, int, const char *, char *)
*   - Maps family and model to processor name
*
****************************************************/

void map_mname(int family, int model, const char *v_name, char *m_name)
{
    // Default to name not known
    m_name[0] = '\0';

    if (!strncmp("AuthenticAMD", v_name, 12)) {
        switch (family) { // extract family code
        case 4: // Am486/AM5x86
            strcpy(m_name, "AMD Am486");
            break;

        case 5: // K6
            switch (model) { // extract model code
            case 0:
            case 1:
            case 2:
            case 3:
                strcpy(m_name, "AMD K5");
                break;
            case 6:
            case 7:
                strcpy(m_name, "AMD K6");
                break;
            case 8:
                strcpy(m_name, "AMD K6-2");
                break;
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
                strcpy(m_name, "AMD K6-3");
                break;
            }
            break;

        case 6: // Athlon
            // No model numbers are currently defined
            strcpy(m_name, "AMD ATHLON");
            break;
        }
    } else if (!strncmp("GenuineIntel", v_name, 12)) {
        switch (family) { // extract family code
        case 4:
            switch (model) { // extract model code
            case 0:
            case 1:
                strcpy(m_name, "INTEL 486DX");
                break;
            case 2:
                strcpy(m_name, "INTEL 486SX");
                break;
            case 3:
                strcpy(m_name, "INTEL 486DX2");
                break;
            case 4:
                strcpy(m_name, "INTEL 486SL");
                break;
            case 5:
                strcpy(m_name, "INTEL 486SX2");
                break;
            case 7:
                strcpy(m_name, "INTEL 486DX2E");
                break;
            case 8:
                strcpy(m_name, "INTEL 486DX4");
                break;
            }
            break;

        case 5:
            switch (model) { // extract model code
            case 1:
            case 2:
            case 3:
                strcpy(m_name, "INTEL Pentium");
                break;
            case 4:
                strcpy(m_name, "INTEL Pentium-MMX");
                break;
            }
            break;

        case 6:
            switch (model) { // extract model code
            case 1:
                strcpy(m_name, "INTEL Pentium-Pro");
                break;
            case 3:
            case 5:
                strcpy(m_name, "INTEL Pentium-II");
                break; // actual differentiation depends on cache settings
            case 6:
                strcpy(m_name, "INTEL Celeron");
                break;
            case 7:
            case 8:
            case 10:
                strcpy(m_name, "INTEL Pentium-III");
                break; // actual differentiation depends on cache settings
            }
            break;

        case 15 | (0x00 << 4): // family 15, extended family 0x00
            switch (model) {
            case 0:
                strcpy(m_name, "INTEL Pentium-4");
                break;
            }
            break;
        }
    } else if (!strncmp("CyrixInstead", v_name, 12)) {
        strcpy(m_name, "Cyrix");
    } else if (!strncmp("CentaurHauls", v_name, 12)) {
        strcpy(m_name, "Centaur");
    }

    if (!m_name[0]) {
        strcpy(m_name, "Unknown");
    }
}

/***
*
* int _cpuid (_p_info *pinfo)
*
* Entry:
*
*   pinfo: pointer to _p_info.
*
* Exit:
*
*   Returns int with capablity bit set even if pinfo = NULL
*
****************************************************/

int _cpuid(_p_info *pinfo)
{
    int nModel = 0;
    int nFamily = 0;
    int nSteppingID = 0;
    bool bSSE3Instructions;
    int CPUInfo[4];
    int CPUFeatures[4];
    int feature = 0;
    int os_support = 0;
    union {
        char cBuf[12 + 1];
        struct
        {
            DWORD dw0;
            DWORD dw1;
            DWORD dw2;
        } s;
    } Ident;

    if (!IsCPUID()) {
        return 0;
    }

    // get the vendor string
    __cpuid(CPUInfo, 0);
    memset(Ident.cBuf, 0, sizeof(Ident));
    (Ident.s.dw0) = CPUInfo[1];
    (Ident.s.dw1) = CPUInfo[3];
    (Ident.s.dw2) = CPUInfo[2];

    // get cpu features
    __cpuid(CPUFeatures, 1);
    nSteppingID = CPUFeatures[0] & 0xf;
    nModel = (CPUFeatures[0] >> 4) & 0xf;
    nFamily = (CPUFeatures[0] >> 8) & 0xf;

    bSSE3Instructions = (CPUFeatures[2] & 0x1);

    if (bSSE3Instructions) {
        feature |= _CPU_FEATURE_SSE;
        feature |= _CPU_FEATURE_SSE2;
        feature |= _CPU_FEATURE_SSE3;
        if (_os_support(_CPU_FEATURE_SSE))
            os_support |= _CPU_FEATURE_SSE;
        if (_os_support(_CPU_FEATURE_SSE2))
            os_support |= _CPU_FEATURE_SSE2;
        if (_os_support(_CPU_FEATURE_SSE3))
            os_support |= _CPU_FEATURE_SSE3;
    }

    if (pinfo) {
        memset(pinfo, 0, sizeof(_p_info));

        pinfo->os_support = os_support;
        pinfo->feature = feature;
        pinfo->family = nFamily;
        pinfo->model = nModel; // retrieve model
        pinfo->stepping = nSteppingID; // retrieve stepping

        Ident.cBuf[12] = 0;
        strcpy(pinfo->v_name, Ident.cBuf);

        map_mname(pinfo->family, pinfo->model, pinfo->v_name, pinfo->model_name);

        pinfo->checks = _CPU_FEATURE_SSE | _CPU_FEATURE_SSE2;
    }

    return feature;
}
}

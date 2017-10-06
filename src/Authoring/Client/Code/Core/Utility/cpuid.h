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

#ifndef _INC_CPUID
#define _INC_CPUID

#define _CPU_FEATURE_MMX 0x0001
#define _CPU_FEATURE_SSE 0x0002
#define _CPU_FEATURE_SSE2 0x0004
#define _CPU_FEATURE_SSE3 0x0008
#define _CPU_FEATURE_SSE4 0x0010
#define _CPU_FEATURE_3DNOW 0x0020

#define _MAX_VNAME_LEN 13
#define _MAX_MNAME_LEN 30

// Added by PB
// MSDN Code to detect processor and OS capabilities. From CPUID code example.
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vcsample/html/vcsamcpuiddeterminecpucapabilities.asp

typedef struct _processor_info
{
    char v_name[_MAX_VNAME_LEN]; // vendor name
    char model_name[_MAX_MNAME_LEN]; // name of model
    // e.g. Intel Pentium-Pro
    int family; // family of the processor
    // e.g. 6 = Pentium-Pro architecture
    int model; // model of processor
    // e.g. 1 = Pentium-Pro for family = 6
    int stepping; // processor revision number
    int feature; // processor feature
    // (same as return value from _cpuid)
    int os_support; // does OS Support the feature?
    int checks; // mask of checked bits in feature
    // and os_support fields
} _p_info;

#ifdef __cplusplus
extern "C"
#endif
    int
    _cpuid(_p_info *);

#endif

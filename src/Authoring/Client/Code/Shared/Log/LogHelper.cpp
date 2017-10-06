/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

//==============================================================================
//	Prefix
//==============================================================================

#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================

#include "LogHelper.h"
#include "UICMath.h"

//==============================================================================
//	Statics
//==============================================================================

IUICLog2 *CLogHelper::s_UICLog2;

//==============================================================================
//	Methods
//==============================================================================

#ifdef WIN32

void CLogHelper::Log(long inLogType, const wchar_t *inFileName, long inLineNumber,
                     long inCategoryType, const wchar_t *inLogString, ...)
{
    if (s_UICLog2 != NULL) {
        TSAddEntryIndirect theAddEntryIndirectStruct = { 0 };

        theAddEntryIndirectStruct.m_StructSize = sizeof(theAddEntryIndirectStruct);
        // theAddEntryIndirectStruct.m_LogStringId		// not used
        wcsncpy(theAddEntryIndirectStruct.m_LogString, inLogString,
                Q3DStudio::MIN((size_t)MAX_PATH, wcslen(inLogString)));
        va_start(theAddEntryIndirectStruct.m_LogParamList, inLogString);
        wcsncpy(theAddEntryIndirectStruct.m_FileNameString, inFileName,
                Q3DStudio::MIN((size_t)MAX_PATH, wcslen(inFileName)));
        theAddEntryIndirectStruct.m_LineNumberLong = inLineNumber;
        theAddEntryIndirectStruct.m_Mask = UICLOG_STRING;
        theAddEntryIndirectStruct.m_Level = LOGLEVEL_CRITICAL;
        theAddEntryIndirectStruct.m_LogType = inLogType;
        theAddEntryIndirectStruct.m_CategoryType = inCategoryType;

        s_UICLog2->AddEntryIndirect((long)&theAddEntryIndirectStruct);

        va_end(theAddEntryIndirectStruct.m_LogParamList);
    }
}

void CLogHelper::Log(long inLogType, const wchar_t *inFileName, long inLineNumber,
                     long inCategoryType, long inStringId, ...)
{
    if (s_UICLog2 != NULL) {
        TSAddEntryIndirect theAddEntryIndirectStruct = { 0 };

        theAddEntryIndirectStruct.m_StructSize = sizeof(theAddEntryIndirectStruct);
        theAddEntryIndirectStruct.m_LogStringId = inStringId;

        va_start(theAddEntryIndirectStruct.m_LogParamList, inStringId);
        wcsncpy(theAddEntryIndirectStruct.m_FileNameString, inFileName,
                Q3DStudio::MIN((size_t)MAX_PATH, wcslen(inFileName)));
        theAddEntryIndirectStruct.m_LineNumberLong = inLineNumber;
        theAddEntryIndirectStruct.m_Mask = UICLOG_ID;
        theAddEntryIndirectStruct.m_Level = LOGLEVEL_CRITICAL;
        theAddEntryIndirectStruct.m_LogType = inLogType;
        theAddEntryIndirectStruct.m_CategoryType = inCategoryType;

        s_UICLog2->AddEntryIndirect((long)&theAddEntryIndirectStruct);

        va_end(theAddEntryIndirectStruct.m_LogParamList);
    }
}

void CLogHelper::Trace(long inCategoryType, const wchar_t *inLogString, ...)
{
    try {
        if (s_UICLog2 != NULL) {
            TSAddEntryIndirect theAddEntryIndirectStruct = { 0 };

            theAddEntryIndirectStruct.m_StructSize = sizeof(theAddEntryIndirectStruct);
            wcsncpy(theAddEntryIndirectStruct.m_LogString, inLogString,
                    Q3DStudio::MIN((size_t)MAX_PATH, wcslen(inLogString)));
            va_start(theAddEntryIndirectStruct.m_LogParamList, inLogString);
            theAddEntryIndirectStruct.m_Mask = UICLOG_STRING;
            theAddEntryIndirectStruct.m_Level = LOGLEVEL_INFO;
            theAddEntryIndirectStruct.m_CategoryType = inCategoryType;

            s_UICLog2->AddEntryIndirect((long)&theAddEntryIndirectStruct);

            va_end(theAddEntryIndirectStruct.m_LogParamList);
        }
    }

    catch (...) {
    }
}

void CLogHelper::AddEntry(long inType, const wchar_t *inMsg)
{
    if (s_UICLog2 != NULL) {
        s_UICLog2->AddEntry(inType, (BYTE *)inMsg);
    }
}

void CLogHelper::Start()
{
    ::CoInitialize(NULL);

    try {
        if (s_UICLog2 == NULL) {
            HRESULT theResult =
                ::CoCreateInstance(__uuidof(UICLog), NULL, CLSCTX_INPROC_SERVER, __uuidof(IUICLog2),
                                   reinterpret_cast<void **>(&s_UICLog2));
            if (!s_UICLog2 || S_OK != theResult) {
                ::OutputDebugString(L"FAILED to create UICLog2 interface.\n");
            }
        }
    }

    catch (...) {
    }
}

void CLogHelper::Stop()
{
    try {
        if (s_UICLog2) {
            s_UICLog2->Terminate();
            s_UICLog2->Release();
            s_UICLog2 = NULL;
        }

        ::CoUninitialize();
    }

    catch (...) {
    }
}

#else

void CLogHelper::Log(long inLogType, const wchar_t *inFileName, long inLineNumber,
                long inCategoryType, const wchar_t *inLogString, ...)
{

}

void CLogHelper::Log(long inLogType, const wchar_t *inFileName, long inLineNumber,
                long inCategoryType, long inStringId, ...)
{

}

void CLogHelper::Trace(long inCategoryType, const wchar_t *inLogString, ...)
{

}

void CLogHelper::AddEntry(long inType, const wchar_t *inMsg)
{

}

void CLogHelper::Start()
{
}

void CLogHelper::Stop()
{
}

#endif // WIN32

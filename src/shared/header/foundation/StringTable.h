/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_STRING_TABLE_H
#define QT3DS_RENDER_STRING_TABLE_H

#if defined(_WIN32) || defined(__QNXNTO__)
#include <wchar.h>
#ifndef WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T
#define WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T
#endif
#endif

#include <QtCore/qstring.h>
#include "Qt3DSFoundation.h"
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
#include "Qt3DSRefCounted.h"
#include "Qt3DSAllocator.h"
#include "EASTL/functional.h"
#include "EABase/eabase.h" //char16_t definition
#include "Qt3DSDataRef.h"
#include "Qt3DSOption.h"
#endif

namespace qt3ds {
namespace foundation {
    typedef char8_t Qt3DSBChar;
    typedef const char8_t *Qt3DSBCharPtr;
    class IStringTable;
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED

    // Serialization types, NVRenderSerializationTypes.h
    struct SStrRemapMap;
    struct SWriteBuffer;

    class CRegisteredString;

    // Discriminated union of either data ref or str table
    // used so clients can test serialization without actually saving
    // the string table out and rebooting the entire system.
    class CStrTableOrDataRef
    {
        Option<NVDataRef<QT3DSU8>> m_DataRef;
        IStringTable *m_StrTable;

    public:
        CStrTableOrDataRef()
            : m_StrTable(NULL)
        {
        }
        CStrTableOrDataRef(NVDataRef<QT3DSU8> inData)
            : m_DataRef(inData)
        {
        }
        CStrTableOrDataRef(IStringTable &inStrTable)
            : m_StrTable(&inStrTable)
        {
        }
        CStrTableOrDataRef(const CStrTableOrDataRef &inOther)
            : m_DataRef(inOther.m_DataRef)
            , m_StrTable(inOther.m_StrTable)
        {
        }
        CStrTableOrDataRef &operator=(const CStrTableOrDataRef &inOther)
        {
            m_DataRef = inOther.m_DataRef;
            m_StrTable = inOther.m_StrTable;
            return *this;
        }
        bool HasStrTable() const { return m_StrTable != NULL; }
        bool HasDataRef() const { return m_DataRef.hasValue(); }
        NVDataRef<QT3DSU8> GetDataRef() const { return *m_DataRef; }
        IStringTable *GetStringTable() const
        {
            QT3DS_ASSERT(m_StrTable);
            return m_StrTable;
        }
    };
#endif
    // String that can only be constructed from strings in the string table.
    // These strings are valid utf-8 strings
    class CRegisteredString
    {
        QString m_String;
        IStringTable *m_table;

    public:
        CRegisteredString()
            : m_String("")
        {
        }
        CRegisteredString(const QString &str, IStringTable *t)
            : m_String(str), m_table(t)
        {
        }
        CRegisteredString(const CRegisteredString &inOther)
            : m_String(inOther.m_String), m_table(inOther.m_table)
        {
        }
        CRegisteredString &operator=(const CRegisteredString &inOther)
        {
            m_String = inOther.m_String;
            m_table = inOther.m_table;
            return *this;
        }

        bool operator==(const CRegisteredString &inStr) const { return m_String == inStr.m_String; }
        // If you use this in a real map then you will get them sorted roughly by the correct
        // order.
        bool operator<(const char *inStr) const
        {
            // Ensure non-null strings to strcmp.
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
            const char *myStr = m_String ? m_String : "";
            inStr = inStr ? inStr : "";
            int answer;
            while (*myStr && *inStr) {
                answer = *inStr - *myStr;
                if (answer)
                    return answer < 0;
                ++myStr;
                ++inStr;
            }
            answer = *inStr - *myStr;
            return answer < 0;
#endif
            return m_String < QString(inStr);
        }
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
        size_t hash() const { return eastl::hash<size_t>()(reinterpret_cast<size_t>(m_String)); }
#endif
        operator Qt3DSBCharPtr() { return c_str(); }
        Qt3DSBCharPtr c_str() const;
        const wchar_t *wc_str() const;
        bool IsValid() const { return !m_String.isNull(); }
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
        // If this string is in the map, changes it to the map value.
        void Remap(const SStrRemapMap &inMap);

        void Remap(const IStringTable &inTable);

        // Using m_String as an offset, if possible remap into
        // inDataPtr's block of memory
        void Remap(NVDataRef<QT3DSU8> inDataPtr);

        void Remap(const CStrTableOrDataRef &inRef)
        {
            if (inRef.HasDataRef())
                Remap(inRef.GetDataRef());
            else if (inRef.HasStrTable())
                Remap(*inRef.GetStringTable());
            else {
                QT3DS_ASSERT(false);
            }
        }

        static CRegisteredString ISwearThisHasBeenRegistered(Qt3DSBCharPtr str)
        {
            CRegisteredString retval;
            retval.m_String = str;
            return retval;
        }
#endif
    };
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
    class IStringTable;

    class CStringHandle
    {
        QT3DSU32 m_Data;

        CStringHandle(QT3DSU32 hdl)
            : m_Data(hdl)
        {
        }

    public:
        CStringHandle()
            : m_Data(0)
        {
        }
        CStringHandle(const CStringHandle &other)
            : m_Data(other.m_Data)
        {
        }
        CStringHandle &operator=(const CStringHandle &other)
        {
            m_Data = other.m_Data;
            return *this;
        }
        bool IsValid() const { return m_Data != 0; }
        QT3DSU32 handle() const { return m_Data; }
        inline const char8_t *c_str(IStringTable &strTable) const;
        operator QT3DSU32() const { return m_Data; }
        static CStringHandle ISwearThisHasBeenRegistered(QT3DSU32 strHandle)
        {
            return CStringHandle(strHandle);
        }
    };
#endif
    // String table stores strings in utf-8 format and does valid utf-16,utf-32 -> utf-8
    // also converts utf8 -> either utf-32 or utf-16, depending on sizeof( wchar_t ) if
    // requested.
    // UICStrConvertUTF.h
    // Also generates offsets that are consistent so clients can convert their strings
    // to offsets during save and convert from offset to string during load regardless
    // of if they load before or after this table.
    class QT3DS_AUTOTEST_EXPORT IStringTable
    {
    public:
        // default state is for multithreaded access to be disabled.
        // remember to implement in CNullStringManager in UICTestPresentation.hxx
        virtual void EnableMultithreadedAccess() = 0;
        virtual void DisableMultithreadedAccess() = 0;

        virtual CRegisteredString RegisterStr(Qt3DSBCharPtr str) = 0;
        // utf-16->utf-8
        virtual CRegisteredString RegisterStr(const char16_t *str) = 0;
        // utf-32->utf-8
        virtual CRegisteredString RegisterStr(const char32_t *str) = 0;
        virtual CRegisteredString RegisterStr(const QString &str) = 0;

        virtual CRegisteredString RegisterStr(const wchar_t *str) = 0;

        virtual const wchar_t *GetWideStr(Qt3DSBCharPtr src) = 0;
        virtual const wchar_t *GetWideStr(const wchar_t *str) = 0;
        virtual const wchar_t *GetWideStr(const QString &str) = 0;

        Qt3DSBCharPtr GetNarrowStr(const wchar_t *src) { return RegisterStr(src).c_str(); }
        virtual Qt3DSBCharPtr GetNarrowStr(const QString &src) = 0;
        Qt3DSBCharPtr GetNarrowStr(Qt3DSBCharPtr src) { return RegisterStr(src).c_str(); }
    };
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
    inline const char8_t *CStringHandle::c_str(IStringTable &strTable) const
    {
        return strTable.HandleToStr(m_Data);
    }
#endif
}
}

#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
// eastl extensions to allow easy hashtable creation using string table strings.
namespace eastl {

template <>
struct hash<qt3ds::foundation::CRegisteredString>
{
    size_t operator()(const qt3ds::foundation::CRegisteredString &str) const { return str.hash(); }
};

template <>
struct equal_to<qt3ds::foundation::CRegisteredString>
{
    bool operator()(const qt3ds::foundation::CRegisteredString &lhs,
                    const qt3ds::foundation::CRegisteredString &rhs) const
    {
        return lhs == rhs;
    }
};
}
#endif

#endif

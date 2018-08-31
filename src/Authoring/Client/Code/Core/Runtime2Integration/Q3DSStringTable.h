/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef Q3DS_STRING_TABLE_H
#define Q3DS_STRING_TABLE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "Qt3DSDMStringTable.h"
#include <QtCore/qvector.h>
#include <QtCore/qstring.h>
#include <QtCore/qset.h>
#include "foundation/StringTable.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/SerializationTypes.h"

namespace Q3DStudio
{

typedef QVector<wchar_t> WQString;

inline WQString toWQString(const QString &inStr)
{
    WQString ptr;
    ptr.resize(inStr.length() + 1);
    inStr.toWCharArray(ptr.data());
    return ptr;
}

class Q3DSStringTable : public qt3dsdm::IStringTable
{
    class RenderStringtable : public qt3ds::foundation::IStringTable
    {
    public:
        // default state is for multithreaded access to be disabled.
        // remember to implement in CNullStringManager in UICTestPresentation.hxx
        void EnableMultithreadedAccess() override
        {
        }
        void DisableMultithreadedAccess() override
        {
        }

        qt3ds::foundation::CRegisteredString RegisterStr(
                qt3ds::foundation::Qt3DSBCharPtr str) override
        {
            return RegisterQStr(QString::fromLatin1(str));
        }
        // utf-16->utf-8
        qt3ds::foundation::CRegisteredString RegisterStr(const char16_t *str) override
        {
            return RegisterQStr(QString::fromUtf16(str));
        }
        // utf-32->utf-8
        qt3ds::foundation::CRegisteredString RegisterStr(const char32_t *str) override
        {
            return RegisterQStr(QString::fromStdU32String(std::u32string(str)));
        }
        qt3ds::foundation::CRegisteredString RegisterStr(const QString &str)
        {
            return RegisterQStr(str);
        }
        qt3ds::foundation::CRegisteredString RegisterStr(const wchar_t *str) override
        {
            return RegisterStr(QString::fromWCharArray(str));
        }
        const wchar_t *GetWideStr(qt3ds::foundation::Qt3DSBCharPtr src) override
        {
            qt3ds::foundation::CRegisteredString rs = RegisterStr(src);
            return rs.wc_str();
        }
        const wchar_t *GetWideStr(const wchar_t *str) override
        {
            qt3ds::foundation::CRegisteredString rs = RegisterStr(str);
            return rs.wc_str();
        }
        const wchar_t *GetWideStr(const QString &str) override
        {
            if (!m_hash.contains(str))
                RegisterQStr(str);
            if (!m_wdatahash.contains(str))
                m_wdatahash.insert(str, toWQString(str));
            return m_wdatahash[str].data();
        }
        qt3ds::foundation::Qt3DSBCharPtr GetNarrowStr(const QString &str) override
        {
            if (!m_hash.contains(str))
                RegisterQStr(str);
            if (!m_cdatahash.contains(str))
                m_cdatahash.insert(str, str.toLatin1());
            return m_cdatahash[str].data();
        }

        qt3ds::foundation::Qt3DSBCharPtr GetNarrowStr(const wchar_t *src)
        {
            return RegisterStr(src).c_str();
        }
        qt3ds::foundation::Qt3DSBCharPtr GetNarrowStr(qt3ds::foundation::Qt3DSBCharPtr src)
        {
            return RegisterStr(src).c_str();
        }
    private:
        qt3ds::foundation::CRegisteredString RegisterQStr(const QString &str)
        {
            if (m_hash.contains(str))
                return m_hash[str];
            qt3ds::foundation::CRegisteredString s(str, this);
            m_hash.insert(str, s);
            return s;
        }
        QHash<QString, qt3ds::foundation::CRegisteredString> m_hash;
        QHash<QString, WQString> m_wdatahash;
        QHash<QString, QByteArray> m_cdatahash;
    };

    RenderStringtable m_renderStringTable;

public:
    Q3DSStringTable()
    {
    }

    ~Q3DSStringTable() override
    {
    }
    const wchar_t *RegisterStr(const wchar_t *inStr) override
    {
        return m_renderStringTable.GetWideStr(inStr);
    }
    virtual const char8_t *RegisterStr(const char8_t *inStr) override
    {
        return m_renderStringTable.GetNarrowStr(inStr);
    }
    // Get the utf-8 or utf-(sizeof wchar_t) converted strings
    virtual const wchar_t *GetWideStr(const char8_t *inStr) override
    {
        return m_renderStringTable.GetWideStr(inStr);
    }
    virtual const char8_t *GetNarrowStr(const wchar_t *inStr) override
    {
        return m_renderStringTable.GetNarrowStr(inStr);
    }
    virtual qt3ds::foundation::IStringTable &GetRenderStringTable() override
    {
        return m_renderStringTable;
    }

    static std::shared_ptr<Q3DSStringTable> instance()
    {
        static std::shared_ptr<Q3DSStringTable> s_table;
        if (!s_table)
            s_table.reset(new Q3DSStringTable());
        return s_table;
    }
};

} // namespace Q3DStudio

#endif

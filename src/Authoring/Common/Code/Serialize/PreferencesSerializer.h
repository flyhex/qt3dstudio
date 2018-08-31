/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#ifndef INCLUDED_PREFERENCES_SERIALIZER_H
#define INCLUDED_PREFERENCES_SERIALIZER_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMXML.h"
#include "StringTokenizer.h"
#include "Qt3DSFileTools.h"

using namespace qt3dsdm;

class CPreferencesSerializer
{
public: // construction
    CPreferencesSerializer();
    ~CPreferencesSerializer();

public: // methods
    void SetPreferencesFile(const QString &theFile);

    void Begin(const QString &inTag);

    bool GetSubElemValue(const QString &inSubElem, QString &outValue);

    void Revert();

    void Remove(const QString &inTag);

    void SetSubElemValue(const QString &inSubElem, const QString &inValue);

    long CountSubElems() const;

    void RemoveSubElem(const QString &inSubElem);

    bool ExistElem(const QString &inElemName);

    bool IsFileSet() const { return m_fileSet; }

private:
    void Serialize();

private:
    bool m_fileSet;
    QFileInfo m_preferencesFile;
    eastl::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>> m_preferencesIO;
    std::shared_ptr<IDOMFactory> m_preferencesFactory;
};

#endif // INCLUDED_PREFERENCES_SERIALIZER_H

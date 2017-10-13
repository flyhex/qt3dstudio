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
#include "UICDMXML.h"
#include "StringTokenizer.h"
#include "UICFileTools.h"

using namespace qt3dsdm;

class CPreferencesSerializer
{
public: // construction
    CPreferencesSerializer();
    ~CPreferencesSerializer();

public: // methods
    void SetPreferencesFile(const Q3DStudio::CString &theFile);

    void Begin(const Q3DStudio::CString &inTag);

    bool GetSubElemValue(const Q3DStudio::CString &inSubElem, Q3DStudio::CString &outValue);

    void Revert();

    void Remove(const Q3DStudio::CString &inTag);

    void SetSubElemValue(const Q3DStudio::CString &inSubElem, const Q3DStudio::CString &inValue);

    long CountSubElems() const;

    void RemoveSubElem(const Q3DStudio::CString &inSubElem);

    bool ExistElem(const Q3DStudio::CString &inElemName);

    bool IsFileSet() const { return m_FileSet; }

private:
    void Serialize();

private:
    bool m_FileSet;
    Q3DStudio::CFilePath m_PreferencesFile;
    TStringTablePtr m_PreferencesStrTable;
    eastl::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>> m_PreferencesIO;
    std::shared_ptr<IDOMFactory> m_PreferencesFactory;
};

#endif // INCLUDED_PREFERENCES_SERIALIZER_H
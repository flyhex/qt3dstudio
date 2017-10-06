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

#ifndef INCLUDED_STUDIO_PROJECT_VARIABLES_H
#define INCLUDED_STUDIO_PROJECT_VARIABLES_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Multicaster.h"
#include "StdAfx.h"

//==============================================================================
//	Forwards
//==============================================================================
class CCore;

class CStudioProjectVariables
{
    //==============================================================================
    //	Internal Classes
    //==============================================================================
public:
    class IVariableChangeListener
    {
    public:
        virtual void OnVariableChanged() = 0;
    };

    //==============================================================================
    //	Typedef
    //==============================================================================
protected:
    typedef std::map<Q3DStudio::CString, Q3DStudio::CString>
        TVariableMap; // change to new coding standard

    //==============================================================================
    //	Construction
    //==============================================================================
public:
    CStudioProjectVariables(CCore *inCore);
    virtual ~CStudioProjectVariables();

    //==============================================================================
    //	Member variables
    //==============================================================================
protected:
    CCore *m_Core;
    TVariableMap m_VariablesMap;
    CMulticaster<IVariableChangeListener *> m_ChangeListeners;

    //==============================================================================
    //	Methods
    //==============================================================================
public:
    Q3DStudio::CString GetProjectVariables() const;
    void GetProjectVariables(std::list<Q3DStudio::CString> &ioVariables);
    bool SetProjectVariables(std::list<Q3DStudio::CString> &inVariables);
    void Clear();

    Q3DStudio::CString ResolveString(const Q3DStudio::CString &inString);
    bool ResolveVariable(const Q3DStudio::CString &inVariable, Q3DStudio::CString &outValue,
                         bool inCaseInsensitive = true);

    // Change listeners
    void AddChangeListener(IVariableChangeListener *inListener);
    void RemoveChangeListener(IVariableChangeListener *inListener);
    virtual void FireChangeEvent();

    // Used for serialization *only*
    TVariableMap &UnsafeGetVariablesMap() { return m_VariablesMap; }
};

#endif // INCLUDED_STUDIO_PROJECT_VARIABLES_H

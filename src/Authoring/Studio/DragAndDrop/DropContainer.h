/****************************************************************************
**
** Copyright (C) 1999-2003 NVIDIA Corporation.
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

#ifndef INCLUDED_DROPCONTAINER
#define INCLUDED_DROPCONTAINER

//==============================================================================
//	Includes
//==============================================================================
#include "DropProxy.h"
#include "DropSource.h"
#include <vector>

class CStudioApp;

class CDropContainer
{
public:
    typedef std::vector<long> TFlavor;
    typedef TFlavor::iterator TFlavorItr;

protected:
    TFlavor m_Flavors; ///< This is a list of flavors handled by this container.

public:
    CDropContainer() {}
    virtual ~CDropContainer() {}

    CDropContainer::TFlavorItr GetFlavorBegin();
    CDropContainer::TFlavorItr GetFlavorEnd();

    void AddMainFlavor(long inMainFlavor);

    // These need to get implemented by the Cross platform Container.
    virtual void onDragEnter() = 0;
    virtual bool OnDragWithin(CDropSource &inSource) = 0;
    virtual bool OnDragReceive(CDropSource &inSource) = 0;
    virtual void OnDragLeave() = 0;
    virtual void OnReflectMouse(CPt &inPoint, Qt::KeyboardModifiers inFlags) = 0;
};

class CWinDropContainer : public CDropContainer
{
public:
    CWinDropContainer();
    virtual ~CWinDropContainer();

    void RegisterForDnd(QWidget *inWindow);
    long ReflectMouse(long inX, long inY);

protected:
    // These are utility functions.
    CDropProxy m_DropProxy; ///< The COLEDropSource pass through.
};

#endif

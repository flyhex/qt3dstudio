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
#include "StudioApp.h"
#include "DropContainer.h"
#include "HotKeys.h"
#include "MouseCursor.h"
#include "FileDropSource.h"
#include "ResourceCache.h"

//===============================================================================
/**
 * 	A derived object will call this to subscribe to Drop Flavors.
 *	@param inMainFlavor the Flavor to add.
 *	@see CDropSource.h
 */
void CDropContainer::AddMainFlavor(long inMainFlavor)
{
    m_Flavors.push_back(inMainFlavor);
}

//===============================================================================
/**
 * 	This an accessor to get the begining of the iterator.
 *	@return the Iterator
 */
CDropContainer::TFlavorItr CDropContainer::GetFlavorBegin()
{
    return m_Flavors.begin();
}

//===============================================================================
/**
 * This is an iterator to the end.
 *	@return the Iterator
 */
CDropContainer::TFlavorItr CDropContainer::GetFlavorEnd()
{
    return m_Flavors.end();
}

//===============================================================================
/**
 *	Constructor to build the container.
 *	This also sets up the DropProxy.
 *	@see CDropProxy
 */
CWinDropContainer::CWinDropContainer()
    : m_DropProxy(this)
{
}
//===============================================================================
/**
 *	Destructor
 */
CWinDropContainer::~CWinDropContainer()
{
}

//===============================================================================
/**
 *	This is so the Window that is derived from CWinContainer can receive drags.
 *	@param inWindow the outer to drag.
 */
void CWinDropContainer::RegisterForDnd(QWidget *inWindow)
{
    // This passes the inWindow down to the COLEDropSource.
    m_DropProxy.Register(inWindow);
}

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

//==============================================================================
//	Includes
//==============================================================================
#include "stdafx.h"
#include "WinDnd.h"
#include "DropSource.h"

//===============================================================================
/**
 * 	Constructor
 */
CWinDragManager::CWinDragManager()
{
    memset(&m_DragItem, 0, sizeof(SDropItem));
}

//===============================================================================
/**
 * 	Dtor
 */
CWinDragManager::~CWinDragManager()
{
}

//===============================================================================
/**
 * 	This will take any data flavor and wrap it up into out drag data.
 */
void CWinDragManager::AddData(long inFlavor, void *inData, unsigned long inSize)
{
    m_DragItem.m_Flavor = inFlavor;
    m_DragItem.m_Data = inData;
    m_DragItem.m_Size = inSize;
}

//===============================================================================
/**
 * 	I made this so it would kind of match the MAC version.
 */
void CWinDragManager::StartDrag()
{
    Track();
}

//===============================================================================
/**
 * 	This is the Blocking function that does the Dragging.
 *	It is assumed that all of the Data is is requested to be dragged, is already in the list
 *	and we take it out of the list and pack it into the Win OLEDropSource and let it go.
 *	We should only return when the whole operation is over.
 */
void CWinDragManager::Track()
{
    // Convert our data into a OLE DragSource.
    HGLOBAL theGlobalData = nullptr;
    COleDataSource theDataSource;

    // Empty the data source
    theDataSource.Empty();

    // long		theDataSize = m_DragItem.m_Size;
    void *theData = m_DragItem.m_Data;
    long theFlavor = m_DragItem.m_Flavor;

    // if ( theDataSize > 0 )
    {
        // Allocate a global block of data
        DWORD theGlobalObjectSize = sizeof(CDropSource *);//sizeof(long /*theDataSize*/);
        theGlobalData = GlobalAlloc(GHND, theGlobalObjectSize);

        // Lock down the global memory object for copying data
        CDropSource *theDropSource = reinterpret_cast<CDropSource *>(GlobalLock(theGlobalData));

        // Copy the CDropSource pointer
        CopyMemory(theDropSource, theData, theGlobalObjectSize);

        // Unlock, but do not free
        GlobalUnlock(theGlobalData);

        // Prepare OLE data for transfer
        theDataSource.CacheGlobalData((CLIPFORMAT)theFlavor, theGlobalData, nullptr);
    }

    // This will go through the framework.
    if (DROPEFFECT_NONE == theDataSource.DoDragDrop()) {
        theDataSource.Empty();
        if (theGlobalData)
            ::GlobalFree(theGlobalData);
    }
}

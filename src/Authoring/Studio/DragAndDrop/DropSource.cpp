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
#include "DropTarget.h"
#include "StudioObjectTypes.h"
#include "HotKeys.h"
#include "Doc.h"
#include "IDragable.h"
#include "FileDropSource.h"
#include "ExplorerFileDropSource.h"
#include "TimelineDropSource.h"
#include "BasicObjectDropSource.h"
#include "ListBoxDropSource.h"
#include "Views.h"
#include "MainFrm.h"
#include "TimelineWidget.h"

CDropSource::CDropSource(long inFlavor, unsigned long inSize)
    : m_Flavor(inFlavor)
    , m_Size(inSize)
    , m_ObjectType(0)
    , m_HasValidTarget(false)
    , m_CurrentFlags(0)
{
    g_StudioApp.GetViews()->getMainFrame()->getTimelineWidget()->enableDnD();
}

CDropSource::~CDropSource()
{
    g_StudioApp.GetViews()->getMainFrame()->getTimelineWidget()->enableDnD(false);
}

CDropSource *CDropSourceFactory::Create(long inFlavor, void *inData, unsigned long inSize)
{
    CDropSource *theDropSource(nullptr);
    switch (inFlavor) {

    case QT3DS_FLAVOR_FILE: {
        theDropSource = new CExplorerFileDropSource(inFlavor, inData, inSize);
    } break;
    case QT3DS_FLAVOR_TEXT:
        // Don't do anythiing for this
        break;

    case QT3DS_FLAVOR_ASSET_UICFILE:
        // make an Aset out of this.
        theDropSource = new CFileDropSource(inFlavor, inData, inSize);
        break;
    }

    return theDropSource;
}

CDropSource *CDropSourceFactory::Create(long inFlavor, IDragable *inDragable)
{
    CDropSource *theDropSource(nullptr);
    switch (inFlavor) {
    case QT3DS_FLAVOR_LISTBOX:
        theDropSource = new CListBoxDropSource(inFlavor, inDragable);
        break;

    case QT3DS_FLAVOR_BASIC_OBJECTS:
        theDropSource = new CBasicObjectDropSource(inFlavor, inDragable);
        break;

    case QT3DS_FLAVOR_ASSET_TL:
        theDropSource = new CTimeLineDropSource(inFlavor, inDragable);
        break;

    default:
        theDropSource = Create(inFlavor, reinterpret_cast<void *>(inDragable), sizeof(inDragable));
    }

    return theDropSource;
}

CDropSource *CDropSourceFactory::Extract(long inFlavor, void *inData, unsigned long /*inSize*/)
{
    CDropSource *theDropSource(nullptr);
    switch (inFlavor) {
    // For all of the Studio Flavors we just need to extract the dropsource out of it.
    case QT3DS_FLAVOR_LISTBOX:
        // make an Aset out of this.
        theDropSource = static_cast<CListBoxDropSource *>(inData);
        break;

    case QT3DS_FLAVOR_BASIC_OBJECTS:
        // make an Aset out of this.
        theDropSource = static_cast<CBasicObjectDropSource *>(inData);
        break;

    case QT3DS_FLAVOR_ASSET_TL:
        // cast it to the right type just so we don't loose the virtual table.
        theDropSource = static_cast<CTimeLineDropSource *>(inData);
        break;

    case QT3DS_FLAVOR_ASSET_UICFILE:
        theDropSource = static_cast<CFileDropSource *>(inData);

        break;
    default:
        theDropSource = nullptr;
    }

    return theDropSource;
}

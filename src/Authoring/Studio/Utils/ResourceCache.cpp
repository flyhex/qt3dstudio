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

//=============================================================================
// Prefix
//=============================================================================
#include "stdafx.h"

//=============================================================================
// Includes
//=============================================================================
#include "ResourceCache.h"
#include "MouseCursor.h"
#include "StudioUtils.h"

#include <QUrl>

//=============================================================================
/**
 * Constructor
 */
CResourceCache::CResourceCache()
{
}

//=============================================================================
/**
 * Destructor: releases all loaded resources.
 */
CResourceCache::~CResourceCache()
{
    Clear();
}

//=============================================================================
/**
 * Returns a default instance of the resource cache so that the whole application
 * can use the same cache if desired.
 * @return Handle to the default instance of the cache
 */
CResourceCache *CResourceCache::GetInstance()
{
    static CResourceCache theCache;
    return &theCache;
}

//=============================================================================
/**
 * Retrieves a bitmap image of the specified name.  Currently only accepts .png
 * files.
 * @param inName Name of the bitmap file to be fetched
 * @return Pointer to a bitmap object or NULL if the image could not be loaded
 */
QPixmap CResourceCache::GetBitmap(const QString &inName)
{
    QPixmap theImage;

    // If our image name is not empty, then lets get it...
    if (!inName.isEmpty()) {
        TImageMap::iterator thePos = m_Images.find(inName);
        if (thePos != m_Images.end()) {
            theImage = thePos->second;
        } else {
            const QString resPath = QString("%1%2").arg(resourceImagePath(), inName);
            if (theImage.load(resPath)) {
                m_Images[inName] = theImage;
            } else {
                qWarning() << Q_FUNC_INFO << "missing image at path:" << resPath;
            }
        }
    }
    return theImage;
}

//=============================================================================
/**
 * Retrieves the specified cursor resource.  The cursor is loaded if necessary
 * otherwise a previously loaded cursor of the same ID is returned.
 * @param inResourceID ID of the cursor to be loaded (see SCursor.h)
 * @return Pointer to the cursor, or NULL if the cursor could not be loaded
 */
QCursor CResourceCache::GetCursor(CMouseCursor::TUICMouseCursor inResourceID)
{
    CMouseCursor *theCursor = NULL;
    CMouseCursor::TUICMouseCursor theKey = inResourceID;

    TCursorMap::iterator thePos = m_Cursors.find(theKey);
    if (thePos != m_Cursors.end()) {
        theCursor = thePos->second;
    } else {
        theCursor = new CMouseCursor();
        if (theCursor->Load(inResourceID))
            m_Cursors[theKey] = theCursor;
        else {
            delete theCursor;
            theCursor = nullptr;
        }
    }

    return theCursor ? theCursor->GetHandle() : QCursor();
}

//=============================================================================
/**
 * Clears all the maps of resources and deletes any associated resources.
 * Called by the destructor.
 */
void CResourceCache::Clear()
{
    m_Images.clear();

    TCursorMap::iterator theCursorPos = m_Cursors.begin();
    for (; theCursorPos != m_Cursors.end(); ++theCursorPos) {
        CMouseCursor *theCursor = theCursorPos->second;
        delete theCursor;
    }
    m_Cursors.clear();
}

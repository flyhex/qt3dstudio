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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "SIcon.h"
#include "Renderer.h"
#include "MasterP.h"
#include "ResourceCache.h"

//=============================================================================
/**
 * Create a new Icon with no image.
 */
CSIcon::CSIcon()
{
    SetAbsoluteSize(CPt(0, 0));
    SetImage(QPixmap());
}

//=============================================================================
/**
 * Create a new Icon with the specified images loaded from the resource cache.
 * @param inResName Name of the image to be loaded for the normal icon
 * @param inDisabledResName Name of the image to be loaded for the icon when it's disabled
 */
CSIcon::CSIcon(const QString &inResName, const QString &inDisabledResName /*= ""*/)
{
    SetAbsoluteSize(CPt(0, 0));
    CResourceCache *theCache = CResourceCache::GetInstance();
    m_EnabledImage = theCache->GetBitmap(inResName);
    m_DisabledImage = theCache->GetBitmap(inDisabledResName);
    SetImage(m_EnabledImage);
}

//=============================================================================
/**
 * Create a new Icon with the specified image.
 * Any pixels in the image with the transparent color will not be drawn.
 * The size of this control will be set to the size of the image.
 * @param inResource string specifying what image to draw.
 * @param inDisabledResource string specifying what disabled image to draw
 */
CSIcon::CSIcon(const QPixmap &inResource, const QPixmap &inDisabledResource /*=NULL*/)
    : m_DisabledImage(inDisabledResource)
    , m_EnabledImage(inResource)
{
    SetAbsoluteSize(CPt(0, 0));
    SetImage(m_EnabledImage);
}

//=============================================================================
/**
 * Destructor
 */
CSIcon::~CSIcon()
{
}

//=============================================================================
/**
 * Set the image that this icon is drawing as.
 * Any pixels in the image with the transparent color will not be drawn.
 * The size of this control will be set to the size of the image.
 * @param inResource the resource ID if the new image to draw.
 * @param inPreserveSize false to check and set size ( setting to true can reduce flickering in some
 * cases )
 */
void CSIcon::SetImage(const QPixmap &inResource, bool inPreserveSize)
{
    m_Image = inResource;

    if (!inPreserveSize) {
        // If we successfully loaded the new image then modify the size.
        if (!m_Image.isNull())
            SetAbsoluteSize(CPt(GetImageSize().width(), GetImageSize().height()));
        else
            SetAbsoluteSize(CPt(0, 0));
    }
    Invalidate();
}

//=============================================================================
/**
 * Set image to the one specified
 * @param inResName Name of the image to be loaded for the normal icon
 * @param inPreserveSize false to check and set size ( setting to true can reduce flickering in some
 * cases )
 */
void CSIcon::SetImage(const QString &inResName, bool inPreserveSize)
{
    CResourceCache *theCache = CResourceCache::GetInstance();
    SetImage(theCache->GetBitmap(inResName), inPreserveSize);
}

//=============================================================================
/**
 * Draw this image.
 * @param inRenderer the renderer to draw to.
 */
void CSIcon::Draw(CRenderer *inRenderer)
{
    inRenderer->PushClippingRect(QRect(QPoint(0,0), GetSize()));
    if (!m_Image.isNull()) {
        inRenderer->DrawBitmap(CPt(0, 0), m_Image);
    }
    inRenderer->PopClippingRect();
}

//=============================================================================
/**
 * Get the size of this image.
 */
QSize CSIcon::GetImageSize()
{
    return m_Image.size();
}

//=============================================================================
/**
 * Enables or disables the parent and switches the image to be displayed for
 * this icon to a disabled or enabled image.
 */
void CSIcon::SetParentEnabled(bool inIsEnabled)
{
    CControl::SetParentEnabled(inIsEnabled);
    if (!IsEnabled()) {
        SetImage(m_DisabledImage);
    } else {
        SetImage(m_EnabledImage);
    }
    Invalidate();
}

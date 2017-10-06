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

#include "resource.h"
#include "MemoryDC.h"

/////////////////////////////////////////////////////////////////////////////
// CMemoryDC

//==============================================================================
/*
 *		Constructor: Initializes the object.
 */
//==============================================================================
CMemoryDC::CMemoryDC()
    : m_ValidFlag(FALSE)
    , m_SrcDC(nullptr)
    , m_OldBmp(nullptr)
{
}

//==============================================================================
/**
 *		Destructor: Releases the object.
 */
//==============================================================================
CMemoryDC::~CMemoryDC()
{
    // automatically release and draw on exit
    Release();
}

//==============================================================================
/**
 *	Create
 *
 *	Creates the memory DC and image bitmap
 *
 *	@param	inDC				Source device context
 *	@param	inSrcRect			CRect containing the source rectangle for drawing
 */
//==============================================================================
void CMemoryDC::Create(CDC *inDC, CRect inSrcRect)
{
    ASSERT(inDC != nullptr);

    if (!m_ValidFlag) {
        // create the memory DC
        CreateCompatibleDC(inDC);

        // save the source DC
        m_SrcDC = inDC;

        // keep track of the destination rectangle
        m_SrcRect.CopyRect(inSrcRect);

        // create a bitmap for the memory bitmap image
        m_MemBmp.CreateCompatibleBitmap(inDC, inSrcRect.Width(), inSrcRect.Height());

        // select the memory image into the memory DC
        m_OldBmp = SelectObject(&m_MemBmp);

        m_ValidFlag = TRUE;
    }
}

//==============================================================================
/**
 *	Release
 *
 *	Releases the memory DC and image bitmap and optionally copies the image
 *	to the source DC.
 *
 *	@param	inCopyToSourceFlag	If TRUE, copies the memory image to the source DC
 */
//==============================================================================
void CMemoryDC::Release(BOOL inCopyToSourceFlag)
{
    // copy the offscreen buffer to the sourceDC passed in Create()

    if (m_ValidFlag) {
        // blit to source DC to the m_SrcRect
        if ((inCopyToSourceFlag) && (m_SrcDC != nullptr))
            m_SrcDC->BitBlt(m_SrcRect.left, m_SrcRect.top, m_SrcRect.Width(), m_SrcRect.Height(),
                            this, 0, 0, SRCCOPY);

        // de-select the memory image from the DC
        SelectObject(m_OldBmp);

        // delete the memory bitmap image
        m_MemBmp.DeleteObject();

        // delete the memory DC
        DeleteDC();

        m_ValidFlag = FALSE;
        m_OldBmp = nullptr;
    }
}

//==============================================================================
/**
 *	CopySourceImage
 *
 *	Copies the source image from the m_SrcRect into the memory DC/image.
 *
 *	@param	None
 */
//==============================================================================
void CMemoryDC::CopySourceImage()
{
    if (m_ValidFlag) {
        // copy the image from the source rectangle to the offscreen image
        if (m_SrcDC != nullptr)
            this->BitBlt(0, 0, m_SrcRect.Width(), m_SrcRect.Height(), m_SrcDC, m_SrcRect.left,
                         m_SrcRect.top, SRCCOPY);
    }
}

//==============================================================================
/**
 *	ConvertRect
 *
 *	Converts a rectangle based on the source coordinates to one based on
 *	the memory image's coordinates.
 *
 *	@param	inDrawRect	Source drawing CRect
 *
 *	@return	CRect		Contains the converted rectangle
 */
//==============================================================================
CRect CMemoryDC::ConvertRect(CRect inDrawRect)
{
    CRect theRect;

    theRect.CopyRect(inDrawRect);
    theRect.OffsetRect(-m_SrcRect.left, -m_SrcRect.top);

    return theRect;
}

//==============================================================================
/**
 *	ConvertPoint
 *
 *	Converts a point based on the source coordinates to one based on
 *	the memory image's coordinates.
 *
 *	@param	inDrawRect	Source drawing CPoint
 *
 *	@return	CPoint		Contains the converted point
 */
//==============================================================================
CPoint CMemoryDC::ConvertPoint(CPoint inDrawPoint)
{
    CPoint thePoint;

    // convert point relative to this DC
    thePoint = inDrawPoint;
    thePoint.x -= m_SrcRect.left;
    thePoint.y -= m_SrcRect.top;

    return thePoint;
}

//==============================================================================
/**
 *	ConvertXPos
 *
 *	Converts an x-position based on the source coordinates to one based on
 *	the memory image's coordinates.
 *
 *	@param	inXValue	Source drawing x-position
 *
 *	@return	long		Contains the converted x-position
 */
//==============================================================================
long CMemoryDC::ConvertXPos(long inXValue)
{
    long theXPos;

    // convert x coordinate relative to this DC
    theXPos = (long)inXValue - (long)m_SrcRect.left;

    return theXPos;
}

//==============================================================================
/**
 *	ConvertYPos
 *
 *	Converts an y-position based on the source coordinates to one based on
 *	the memory image's coordinates.
 *
 *	@param	inYValue	Source drawing y-position
 *
 *	@return	long		Contains the converted y-position
 */
//==============================================================================
long CMemoryDC::ConvertYPos(long inYValue)
{
    long theYPos;

    // convert y coordinate relative to this DC
    theYPos = (long)inYValue - (long)m_SrcRect.top;

    return theYPos;
}

//==============================================================================
/**
 *	SetRect
 *
 *	Sets the drawing/output bounding rectangle.
 *
 *	@param	inRect		Bounding rectangle for drawing
 */
//==============================================================================
void CMemoryDC::SetRect(CRect inRect)
{
    m_SrcRect = inRect;
}

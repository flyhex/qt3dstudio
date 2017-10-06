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

#ifndef INCLUDED_MEMORY_DC_H
#define INCLUDED_MEMORY_DC_H

#pragma once

//==============================================================================
//	Includes
//==============================================================================

//==============================================================================
/**
 *	CMemoryDC: Subclassed CDC for drawing offscreen
 *
 *	This is a subclassed CDC so we can control drawing into a DC with offscreen
 *	buffers to prevent flickering.
  */
//==============================================================================

class CMemoryDC : public CDC
{
public:
    //==========================================================================
    //	Fields
    //==========================================================================

protected:
    BOOL m_ValidFlag; // TRUE if we can successfully release/copy the offscreen image to the screen
    CRect m_SrcRect; // Bounding rectangle for drawing
    CBitmap m_MemBmp; // Offscreen bitmap image
    CBitmap *m_OldBmp; // Previous bitmap in the offscreen DC
    CDC *m_SrcDC; // Source device context for final blit

    //==========================================================================
    //	Methods
    //==========================================================================

public:
    void SetRect(CRect inRect);

    // Construction

    CMemoryDC();
    virtual ~CMemoryDC();

    // Access

    CPoint ConvertPoint(CPoint inDrawPoint);
    CRect ConvertRect(CRect inDrawRect);
    long ConvertXPos(long inXValue);
    long ConvertYPos(long inYValue);
    void CopySourceImage();
    void Create(CDC *inDC, CRect inSrcRect);

    void Release(BOOL inCopyToSourceFlag = TRUE);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // INCLUDED_MEMORY_DC_H

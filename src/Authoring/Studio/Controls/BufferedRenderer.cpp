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

#include "Qt3DSCommonPrecompile.h"

#include "BufferedRenderer.h"
#include "AppFonts.h"


//=============================================================================
/**
 * Create a buffered renderer.
 * @param inSize the size of this renderer.
 */
CBufferedRenderer::CBufferedRenderer(const QSize &inSize)
    : CWinRenderer()
{
    m_CurrentBitmap = QPixmap(inSize);

    // KDAB_TODO: I don't see why m_OldBitmap is needed, nor what is the difference between CBufferedRenderer and COffscreenRenderer
    m_OldBitmap = m_CurrentBitmap;

    m_painter = new QPainter(&m_CurrentBitmap);
    QFont font = CAppFonts::GetInstance()->GetNormalFont();
    m_painter->setFont(font);

    m_Size = inSize;

    PushClippingRect(QRect(QPoint(0,0), inSize));
}


//=============================================================================
/**
 * Destructor
 */
CBufferedRenderer::~CBufferedRenderer()
{
    delete m_painter;
    m_painter = nullptr;
}

//=============================================================================
/**
 * Copy this BufferedRenderer to inRenderer using alpha blend.
 * This will semi-transparently copy the contents of this buffer to inRenderer
 * using the alpha.
 * @param inRenderer the destination renderer.
 * @param inAlpha the opacity, 255 = opaque 0 = transparent.
 */
/*void CBufferedRenderer::TransparentBltTo( CRenderer* inRenderer, short inAlpha )
{

        CPt theDestination;
        theDestination.Offset( inRenderer->GetTranslation( ) );

        AlphaBlendExt( inRenderer->GetGraphicsDevice( )->m_hDC, (short)theDestination.x,
(short)theDestination.y, (short)m_Size.x, (short)m_Size.y, m_DC->m_hDC, 0, 0, (short)m_Size.x,
(short)m_Size.y, inAlpha );
//	::DrawAlphaBlendBitmap( inRenderer->GetDC( ), (short)theDestination.x,
(short)theDestination.y, m_CurrentBitmap, (short)m_Size.x, (short)m_Size.y, 0, 0, inAlpha );

}*/

//==============================================================================
/**
 *	Draws a bitmap using alpha blending.  Works in NT/2000/9x
 *
 *	@param	inDCDest		Destination device context
 *	@param	inX				X coordinate for drawing
 *	@param	inY				Y coordinate for drawing
 *	@param	inWidth			Drawing width
 *	@param	inHeight		Drawing height
 *	@param	inDCSrc			Source device context
 *	@param	inSourceX		X source coordinate
 *	@param	inSourceY		Y source coordinate
 *	@param	inSourceWidth	Source drawing width
 *	@param	inSourceHeight	Source drawing height
 *	@param	inAlpha			Alpha blend: 0-255, where 255 is opaque
 */
//==============================================================================
/*bool CBufferedRenderer::AlphaBlendExt( HDC inDCDest, short inX, short inY, short inWidth, short
inHeight, HDC inDCSrc, short inSourceX, short inSourceY, short inSourceWidth, short inSourceHeight,
short inAlpha )
{
        BITMAPINFOHEADER	theBMI;

        theBMI.biSize = sizeof( BITMAPINFOHEADER );
        theBMI.biWidth = inWidth;
        theBMI.biHeight = inHeight;
        theBMI.biPlanes = 1;
        theBMI.biBitCount = 32;				// 24 bits + alpha channel
        theBMI.biCompression = BI_RGB;		// no compression
        theBMI.biSizeImage = 0;
        theBMI.biXPelsPerMeter = 0;
        theBMI.biYPelsPerMeter = 0;
        theBMI.biClrUsed = 0;				// use the whole palette
        theBMI.biClrImportant = 0;

        BYTE*	theSrcBits;
        HBITMAP	theBmpSrc;

        // Create DIB section in shared memory
        theBmpSrc = CreateDIBSection( inDCSrc, ( BITMAPINFO* ) &theBMI, DIB_RGB_COLORS, ( void** )
&theSrcBits, 0, 0L );

        BYTE*	theDestBits;
        HBITMAP	theBmpDest;

        // Create DIB section in shared memory
        theBmpDest = CreateDIBSection( inDCDest, ( BITMAPINFO* ) &theBMI, DIB_RGB_COLORS, ( void** )
&theDestBits, 0, 0L );

        // Copy our source and destination bitmaps onto our DIBSections.
        // so we can get access to their bits using the BYTE*'s we used
        // in the CreateDIBSection()s above.

        HDC theDC = CreateCompatibleDC( nullptr );

        HBITMAP	theDCOld = ( HBITMAP ) SelectObject( theDC, theBmpSrc );

        if ( !StretchBlt( theDC, 0, 0, inWidth, inHeight, inDCSrc, inSourceX, inSourceY,
inSourceWidth, inSourceHeight, SRCCOPY ) )
        {
                SelectObject( theDC, theDCOld );
                DeleteDC( theDC );
                DeleteObject( theBmpSrc );
                DeleteObject( theBmpDest );
                return false;
        }

        SelectObject( theDC, theBmpDest );

        if (! StretchBlt( theDC, 0, 0, inWidth, inHeight, inDCDest, inX, inY, inWidth, inHeight,
SRCCOPY ) )
        {
                SelectObject( theDC, theDCOld );
                DeleteDC( theDC );
                DeleteObject( theBmpSrc );
                DeleteObject( theBmpDest );
                return true;
        }

        SelectObject( theDC, theDCOld );
        DeleteDC( theDC );

        short	theXLoop, theYLoop;

        for( theYLoop=0; theYLoop<inHeight; ++theYLoop )
        {
                LPBYTE	theDestRGB = ( LPBYTE ) &( ( DWORD* ) theDestBits)[theYLoop * inWidth];
                LPBYTE	theSrcRGB = ( LPBYTE ) &( ( DWORD* ) theSrcBits)[theYLoop * inWidth];

                for( theXLoop=0; theXLoop<inWidth; theXLoop++ )
                {
                        theSrcRGB[0] = ( BYTE ) ( ( theDestRGB[0] * ( 255 - inAlpha ) + theSrcRGB[0]
* inAlpha ) >> 8 );
                        theSrcRGB[1] = ( BYTE ) ( ( theDestRGB[1] * ( 255 - inAlpha ) + theSrcRGB[1]
* inAlpha ) >> 8 );
                        theSrcRGB[2] = ( BYTE ) ( ( theDestRGB[2] * ( 255 - inAlpha ) + theSrcRGB[2]
* inAlpha ) >> 8 );

                        theSrcRGB += 4;
                        theDestRGB += 4;
                }
        }

        theDC = CreateCompatibleDC( nullptr );

        theDCOld = ( HBITMAP ) SelectObject( theDC, theBmpSrc );

        if ( !BitBlt( inDCDest, inX, inY, inWidth, inHeight, theDC, 0, 0, SRCCOPY ) )
        {
                SelectObject( theDC, theDCOld );
                DeleteDC( theDC );
                DeleteObject( theBmpSrc );
                DeleteObject( theBmpDest );
                return false;
        }

        SelectObject( theDC, theDCOld );
        DeleteDC( theDC );

        DeleteObject( theBmpSrc );
        DeleteObject( theBmpDest );

        return true;
}*/

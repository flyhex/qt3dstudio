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
#include "stdafx.h"
#include "Strings.h"

#include "WinUtils.h"

//==============================================================================
/**
 *	DrawBitmap
 *
 *	Draw a bitmap given a device context, position and HBITMAP handle.
 *
 *	@param	inDC		Device context for drawing
 *	@param	inPos		CPoint position for drawing
 *	@param	inBitmap	Handle of the bitmap to draw
 */
//==============================================================================
void CWinUtils::DrawBitmap(CDC *inDC, CPoint inPos, HBITMAP inBitmap)
{
    CDC theMemDC;
    BITMAP theBitmapStruct;
    CBitmap *theOldBitmap;

    if ((inDC != nullptr) && (inBitmap != nullptr)) {
        GetObject(inBitmap, sizeof(BITMAP), &theBitmapStruct);

        theMemDC.CreateCompatibleDC(inDC);
        theOldBitmap = theMemDC.SelectObject(CBitmap::FromHandle(inBitmap));

        inDC->BitBlt(inPos.x, inPos.y, theBitmapStruct.bmWidth, theBitmapStruct.bmHeight, &theMemDC,
                     0, 0, SRCCOPY);

        theMemDC.SelectObject(theOldBitmap);
        theMemDC.DeleteDC();
    }
}

//==============================================================================
/**
 *	DrawTransparentBitmap
 *
 *	Draw a bitmap given a device context, position and HBITMAP handle using transparency
 *
 *	@param	inDC				Device context for drawing
 *	@param	inDestX				Destination X position
 *	@param	inDestY				Destination Y position
 *	@param	inBmp				Bitmap for drawing
 *	@param	inColorTransparent	Transparent color
 *	@param	inWidth				Destination width
 *	@param	inHeight			Destination height
 *	@param	inSourceX			Source X position
 *	@param	inSourceY			Source Y position
 */
//==============================================================================
void CWinUtils::DrawTransparentBitmap(CDC *inDC, long inDestX, long inDestY, HBITMAP inBmp,
                                      COLORREF inColorTransparent, long inWidth, long inHeight,
                                      long inSourceX, long inSourceY)
{
    CDC theMemDC;
    BITMAP theBitmap;
    CBitmap *theOldBmp;

    GetObject(inBmp, sizeof(BITMAP), &theBitmap);

    if (inWidth == -1)
        inWidth = theBitmap.bmWidth;
    if (inHeight == -1)
        inHeight = theBitmap.bmHeight;

    theMemDC.CreateCompatibleDC(inDC);
    theOldBmp = theMemDC.SelectObject(CBitmap::FromHandle(inBmp));

    TransparentBltExt(inDC->m_hDC, inDestX, inDestY, inWidth, inHeight, theMemDC.m_hDC, inSourceX,
                      inSourceY, inWidth, inHeight, inColorTransparent);

    theMemDC.SelectObject(theOldBmp);
    theMemDC.DeleteDC();
}

//==============================================================================
/**
 *	TransparentBltExt
 *
 *	Draw a bitmap given a device context, position and HBITMAP handle using transparency
 *
 *	@param	inDC				Device context for drawing
 *	@param	inDestX				Destination X position
 *	@param	inDestY				Destination Y position
 *	@param	inBmp				Bitmap for drawing
 *	@param	inColorTransparent	Transparent color
 *	@param	inWidth				Destination width
 *	@param	inHeight			Destination height
 *	@param	inSourceX			Source X position
 *	@param	inSourceY			Source Y position
 */
//==============================================================================
BOOL CWinUtils::TransparentBltExt(HDC inDCDest, long inXOriginDest, long inYOriginDest,
                                  long inWidthDest, long inHeightDest, HDC inDCSrc,
                                  long inXOriginSrc, long inYOriginSrc, long inWidthSrc,
                                  long inHeightSrc, UINT inColorTransparent)
{
    if ((inWidthDest < 1) || (inWidthSrc < 1) || (inHeightDest < 1) || (inHeightSrc < 1))
        return FALSE;

    HDC theDC, theMaskDC;
    HDC theNewMaskDC;
    HBITMAP theBmp, theOldBmp, theMaskBmp, theOldMask;
    HBITMAP theNewMask;

    theDC = CreateCompatibleDC(nullptr);
    theBmp = CreateBitmap(inWidthSrc, inHeightSrc, 1, GetDeviceCaps(theDC, BITSPIXEL), nullptr);

    if (theBmp == nullptr) {
        DeleteDC(theDC);
        return FALSE;
    }

    theOldBmp = (HBITMAP)SelectObject(theDC, theBmp);

    if (!BitBlt(theDC, 0, 0, inWidthSrc, inHeightSrc, inDCSrc, inXOriginSrc, inYOriginSrc,
                SRCCOPY)) {
        SelectObject(theDC, theOldBmp);
        DeleteObject(theBmp);
        DeleteDC(theDC);
        return FALSE;
    }

    theMaskDC = CreateCompatibleDC(nullptr);
    theMaskBmp = CreateBitmap(inWidthSrc, inHeightSrc, 1, 1, nullptr);

    if (theMaskBmp == nullptr) {
        SelectObject(theDC, theOldBmp);
        DeleteObject(theBmp);
        DeleteDC(theDC);
        DeleteDC(theMaskDC);
        return FALSE;
    }

    theOldMask = (HBITMAP)SelectObject(theMaskDC, theMaskBmp);
    SetBkColor(theMaskDC, RGB(0x00, 0x00, 0x00));
    SetTextColor(theMaskDC, RGB(0xFF, 0xFF, 0xFF));

    if (!BitBlt(theMaskDC, 0, 0, inWidthSrc, inHeightSrc, nullptr, 0, 0, BLACKNESS)) {
        SelectObject(theMaskDC, theOldMask);
        DeleteObject(theMaskBmp);
        DeleteDC(theMaskDC);
        SelectObject(theDC, theOldBmp);
        DeleteObject(theBmp);
        DeleteDC(theDC);
        return FALSE;
    }

    SetBkColor(theDC, inColorTransparent);
    BitBlt(theMaskDC, 0, 0, inWidthSrc, inHeightSrc, theDC, 0, 0, SRCINVERT);

    SetBkColor(theDC, RGB(0x00, 0x00, 0x00));
    SetTextColor(theDC, RGB(0xFF, 0xFF, 0xFF));
    BitBlt(theDC, 0, 0, inWidthSrc, inHeightSrc, theMaskDC, 0, 0, SRCAND);

    theNewMaskDC = CreateCompatibleDC(nullptr);
    theNewMask =
        CreateBitmap(inWidthDest, inHeightDest, 1, GetDeviceCaps(theNewMaskDC, BITSPIXEL), nullptr);
    if (theNewMask == nullptr) {
        SelectObject(theDC, theOldBmp);
        DeleteDC(theDC);
        SelectObject(theMaskDC, theOldMask);
        DeleteDC(theMaskDC);
        DeleteDC(theNewMaskDC);
        DeleteObject(theBmp);
        DeleteObject(theMaskBmp);
        return FALSE;
    }

    SetStretchBltMode(theNewMaskDC, COLORONCOLOR);
    HBITMAP theOldNewMask = (HBITMAP)SelectObject(theNewMaskDC, theNewMask);
    StretchBlt(theNewMaskDC, 0, 0, inWidthDest, inHeightDest, theMaskDC, 0, 0, inWidthSrc,
               inHeightSrc, SRCCOPY);

    SelectObject(theMaskDC, theOldMask);
    DeleteDC(theMaskDC);
    DeleteObject(theMaskBmp);

    HDC theNewImageDC = CreateCompatibleDC(nullptr);
    HBITMAP theNewImage =
        CreateBitmap(inWidthDest, inHeightDest, 1, GetDeviceCaps(theNewMaskDC, BITSPIXEL), nullptr);
    if (theNewImage == nullptr) {
        SelectObject(theDC, theOldBmp);
        DeleteDC(theDC);
        DeleteDC(theNewMaskDC);
        DeleteObject(theBmp);
        return FALSE;
    }

    HBITMAP theOldNewImage = (HBITMAP)SelectObject(theNewImageDC, theNewImage);
    StretchBlt(theNewImageDC, 0, 0, inWidthDest, inHeightDest, theDC, 0, 0, inWidthSrc, inHeightSrc,
               SRCCOPY);

    SelectObject(theDC, theOldBmp);
    DeleteDC(theDC);
    DeleteObject(theBmp);

    // Finally, do the final blitting to get the image to the destination device context.
    BitBlt(inDCDest, inXOriginDest, inYOriginDest, inWidthDest, inHeightDest, theNewMaskDC, 0, 0,
           SRCAND);
    BitBlt(inDCDest, inXOriginDest, inYOriginDest, inWidthDest, inHeightDest, theNewImageDC, 0, 0,
           SRCPAINT);

    SelectObject(theNewImageDC, theOldNewImage);
    DeleteDC(theNewImageDC);
    SelectObject(theNewMaskDC, theOldNewMask);
    DeleteDC(theNewMaskDC);
    DeleteObject(theNewImage);
    DeleteObject(theNewMask);

    return TRUE;
}

//==============================================================================
/**
 *	DrawAlphaBlendBitmap
 *
 *	Draw a bitmap given a device context, position and HBITMAP handle using
 *	alpha blending and transparency.
 *
 *	@param	inDC				Device context for drawing
 *	@param	inDestX				Destination X position
 *	@param	inDestY				Destination Y position
 *	@param	inBmp				Bitmap for drawing
 *	@param	inColorTransparent	Transparent color
 *	@param	inWidth				Destination width
 *	@param	inHeight			Destination height
 *	@param	inSourceX			Source X position
 *	@param	inSourceY			Source Y position
 */
//==============================================================================
void CWinUtils::DrawAlphaBlendBitmap(CDC *inDC, short inDestX, short inDestY, HBITMAP inBmp,
                                     short inWidth, short inHeight, short inSourceX,
                                     short inSourceY, short inAlpha, BOOL inTransparentFlag,
                                     COLORREF inColorTransparent)
{
    Q_UNUSED(inColorTransparent);

    CDC theMemDC;
    BITMAP theBitmap;
    CBitmap *theOldBmp;
    short theWidth = inWidth;
    short theHeight = inHeight;

    COLORREF theBackColor;

    // Get the size of the bitmap.
    GetObject(inBmp, sizeof(BITMAP), &theBitmap);

    if (inWidth == -1)
        theWidth = (short)theBitmap.bmWidth;
    if (inHeight == -1)
        theHeight = (short)theBitmap.bmHeight;

    theMemDC.CreateCompatibleDC(inDC);
    theOldBmp = theMemDC.SelectObject(CBitmap::FromHandle(inBmp));

    if (inTransparentFlag) {
        // Get the current background pixel at 0,0
        theBackColor = theMemDC.GetPixel(0, 0);

        // Create a fill brush and select into the device context
        CBrush theFillBrush(inColorTransparent);
        CBrush *theOldFillBrush;

        // Fill to replace the existing background color with the replacement color.
        theOldFillBrush = theMemDC.SelectObject(&theFillBrush);
        theMemDC.ExtFloodFill(0, 0, theBackColor, FLOODFILLSURFACE);
        theMemDC.ExtFloodFill(theWidth - 1, 0, theBackColor, FLOODFILLSURFACE);
        theMemDC.ExtFloodFill(theWidth - 1, theHeight - 1, theBackColor, FLOODFILLSURFACE);
        theMemDC.ExtFloodFill(0, theHeight - 1, theBackColor, FLOODFILLSURFACE);
        theMemDC.SelectObject(theOldFillBrush);
    }

    // Draw the alpha-blended image.
    AlphaBlendExt(inDC->m_hDC, inDestX, inDestY, theWidth, theHeight, theMemDC.m_hDC, inSourceX,
                  inSourceY, theWidth, theHeight, inAlpha);

    theMemDC.SelectObject(theOldBmp);
    theMemDC.DeleteDC();
}

//==============================================================================
/**
 *	AlphaBlendExt
 *
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
BOOL CWinUtils::AlphaBlendExt(HDC inDCDest, short inX, short inY, short inWidth, short inHeight,
                              HDC inDCSrc, short inSourceX, short inSourceY, short inSourceWidth,
                              short inSourceHeight, short inAlpha)
{
    BITMAPINFOHEADER theBMI;

    theBMI.biSize = sizeof(BITMAPINFOHEADER);
    theBMI.biWidth = inWidth;
    theBMI.biHeight = inHeight;
    theBMI.biPlanes = 1;
    theBMI.biBitCount = 32; // 24 bits + alpha channel
    theBMI.biCompression = BI_RGB; // no compression
    theBMI.biSizeImage = 0;
    theBMI.biXPelsPerMeter = 0;
    theBMI.biYPelsPerMeter = 0;
    theBMI.biClrUsed = 0; // use the whole palette
    theBMI.biClrImportant = 0;

    BYTE *theSrcBits;
    HBITMAP theBmpSrc;

    // Create DIB section in shared memory
    theBmpSrc = CreateDIBSection(inDCSrc, (BITMAPINFO *)&theBMI, DIB_RGB_COLORS,
                                 (void **)&theSrcBits, 0, 0L);

    BYTE *theDestBits;
    HBITMAP theBmpDest;

    // Create DIB section in shared memory
    theBmpDest = CreateDIBSection(inDCDest, (BITMAPINFO *)&theBMI, DIB_RGB_COLORS,
                                  (void **)&theDestBits, 0, 0L);

    // Copy our source and destination bitmaps onto our DIBSections.
    // so we can get access to their bits using the BYTE*'s we used
    // in the CreateDIBSection()s above.

    HDC theDC = CreateCompatibleDC(nullptr);
    HBITMAP theDCOld = (HBITMAP)SelectObject(theDC, theBmpSrc);

    if (!StretchBlt(theDC, 0, 0, inWidth, inHeight, inDCSrc, inSourceX, inSourceY, inSourceWidth,
                    inSourceHeight, SRCCOPY)) {
        SelectObject(theDC, theDCOld);
        DeleteDC(theDC);
        DeleteObject(theBmpSrc);
        DeleteObject(theBmpDest);
        return FALSE;
    }

    SelectObject(theDC, theBmpDest);

    if (!StretchBlt(theDC, 0, 0, inWidth, inHeight, inDCDest, inX, inY, inWidth, inHeight,
                    SRCCOPY)) {
        SelectObject(theDC, theDCOld);
        DeleteDC(theDC);
        DeleteObject(theBmpSrc);
        DeleteObject(theBmpDest);
        return FALSE;
    }

    SelectObject(theDC, theDCOld);
    DeleteDC(theDC);

    short theXLoop, theYLoop;

    for (theYLoop = 0; theYLoop < inHeight; ++theYLoop) {
        LPBYTE theDestRGB = (LPBYTE) & ((DWORD *)theDestBits)[theYLoop * inWidth];
        LPBYTE theSrcRGB = (LPBYTE) & ((DWORD *)theSrcBits)[theYLoop * inWidth];

        for (theXLoop = 0; theXLoop < inWidth; theXLoop++) {
            theSrcRGB[0] = (BYTE)((theDestRGB[0] * (255 - inAlpha) + theSrcRGB[0] * inAlpha) >> 8);
            theSrcRGB[1] = (BYTE)((theDestRGB[1] * (255 - inAlpha) + theSrcRGB[1] * inAlpha) >> 8);
            theSrcRGB[2] = (BYTE)((theDestRGB[2] * (255 - inAlpha) + theSrcRGB[2] * inAlpha) >> 8);

            theSrcRGB += 4;
            theDestRGB += 4;
        }
    }

    theDC = CreateCompatibleDC(nullptr);

    theDCOld = (HBITMAP)SelectObject(theDC, theBmpSrc);

    if (!BitBlt(inDCDest, inX, inY, inWidth, inHeight, theDC, 0, 0, SRCCOPY)) {
        SelectObject(theDC, theDCOld);
        DeleteDC(theDC);
        DeleteObject(theBmpSrc);
        DeleteObject(theBmpDest);
        return FALSE;
    }

    SelectObject(theDC, theDCOld);
    DeleteDC(theDC);

    DeleteObject(theBmpSrc);
    DeleteObject(theBmpDest);

    return TRUE;
}

//==============================================================================
/**
 *	GetStudioLogFont: Fill a LOGFONT structure with the default font information for Studio
 *objects.
 *
 *	Create a LOGFONT structure with data used by the various palettes and controls in Studio.
 *
 *	@param	outLogFont	LOGFONT structure returned by this function.
 */
//==============================================================================
void CWinUtils::GetStudioLogFont(LOGFONT &outLogFont)
{
    // Create the default Studio font - specified in string table so that it can be changed for
    // different languages
    memset(&outLogFont, 0, sizeof(LOGFONT));
    CString theFontSize = ::LoadResourceString(IDS_STUDIOFONT_SIZE);
    CString theFontFace = L"SegoeUI";
    outLogFont.lfHeight = _wtoi(theFontSize);
    wcscpy(outLogFont.lfFaceName, theFontFace);
    // Make sure that you set the character set to the system default or your strings won't be
    // localizable
    outLogFont.lfCharSet = DEFAULT_CHARSET;
}

//=============================================================================
/**
 * Copy the contents of a File into an Archive.
 * This will read the entire file into the archive.
 * The file should exist already.
 * @param inFilename the name of the file to read in.
 * @param inDestination the destination archive for the data.
 */
//=============================================================================
void CWinUtils::CopyFileToArchive(CString inFilename, CArchive &inDestination)
{
    CFile theFile(inFilename, CFile::modeRead);

    unsigned long theLength = (unsigned long)theFile.GetLength();
    unsigned long theBufferSize = 1024;
    while (theBufferSize < theLength / 10 && theBufferSize < 1e6)
        theBufferSize *= 2; // double until we have a buffer that is about %10 of file size
    char *theBuffer = new char[theBufferSize];

    unsigned long theReadCount = theBufferSize;
    unsigned long theTotalReadCount = 0;

    // Loop around reading the file in chunks
    while (theTotalReadCount < theLength) {
        if (theLength - theTotalReadCount < theReadCount)
            theReadCount = theLength - theTotalReadCount;

        theFile.Read(theBuffer, theReadCount);
        inDestination.Write(theBuffer, theReadCount);

        theTotalReadCount += theReadCount;
    }

    theFile.Close();
    delete[] theBuffer;
}

//=============================================================================
/**
 * Copy the contents of an Archive into a File.
 * If the file does not exist it will be created. If it does exist it will be
 * cleared.
 * @param inArchive the source of the data for the file.
 * @param inDestination the file the data is to be written to.
 * @param inLength the amount of data to copy to the file.
 */
//=============================================================================
void CWinUtils::CopyArchiveToFile(CArchive &inArchive, CString inDestination,
                                  unsigned long inLength)
{
    CFile theFile(inDestination, CFile::modeWrite | CFile::modeCreate);

    unsigned long theBufferSize = 1024;
    while (theBufferSize < inLength / 10 && theBufferSize < 1e6)
        theBufferSize *= 2; // double until we have a buffer that is about %10 of file size
    char *theBuffer = new char[theBufferSize];

    unsigned long theLength = inLength;
    unsigned long theReadCount = theBufferSize;
    unsigned long theTotalReadCount = 0;

    // Loop around reading from the archive and writing to the file.
    while (theTotalReadCount < theLength) {
        if (theLength - theTotalReadCount < theReadCount)
            theReadCount = theLength - theTotalReadCount;

        inArchive.Read(theBuffer, theReadCount);
        theFile.Write(theBuffer, theReadCount);

        theTotalReadCount += theReadCount;
    }
    theFile.Close();
    delete[] theBuffer;
}

//=============================================================================
/**
 * Used for StringToInt64 to avoid double errors in math.h's pow( ).
 *
 *	@param	inNumber	Base number
 *	@param	inPower		Power to raise the base
 *
 *	@return	Result of x^y
 */
//=============================================================================
unsigned __int64 CWinUtils::RaisePower(unsigned __int64 inNumber, unsigned long inPower)
{
    if (inPower == 0)
        return 1;
    return inNumber * RaisePower(inNumber, inPower - 1);
}

//=============================================================================
/**
 * Parse the string into an __int64.
 *
 * @param	inString the string to parse.
 * @param	inStringLen the length of the string.
 * @param	inBase the base of the character encoding (16 is Hex, 10 is decimal).
 *
 * @return <the parsed string.
 */
//=============================================================================
unsigned __int64 CWinUtils::StringToInt64(const char *inString, unsigned short inStringLen,
                                          unsigned short inBase)
{
    unsigned __int64 theKey = 0;
    char theChar;
    unsigned short i;

    for (i = 0; i < inStringLen; ++i) {
        theChar = inString[i];
        if (theChar < 'A')
            theChar -= '0';
        else if (theChar < 'a')
            theChar -= 'A' - 10;
        else
            theChar -= 'a' - 10;

        theKey +=
            ((unsigned __int64)theChar) * (unsigned __int64)RaisePower(inBase, inStringLen - 1 - i);
    }
    return theKey;
}

//=============================================================================
/**
 * Get a wchar_t from an MFC CString (used to go to Q3DStudio::CString without flattening unicode)
 *
 * @param	inMFCString the string convert
 *
 * @return <the null-terminated wchar_t buffer. caller must delete[]!!!!
 */
//=============================================================================
wchar_t *CWinUtils::WideFromMFC(::CString inMFCString)
{
    TCHAR *theBuffer = inMFCString.GetBuffer(0);
    long theMultiLength = inMFCString.GetLength();
    wchar_t *theDestBuffer = new wchar_t[theMultiLength * 2 + 1]; // times 2 so we know we are big
                                                                  // enough for 1 and 2 byte chars
                                                                  // from multibyte
    wcscpy(theDestBuffer, theBuffer);
    inMFCString.ReleaseBuffer();
    return theDestBuffer;
}
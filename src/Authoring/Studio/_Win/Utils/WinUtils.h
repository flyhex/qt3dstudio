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

#ifndef INCLUDED_WIN_UTILS_H
#define INCLUDED_WIN_UTILS_H 1

#pragma once

class CWinUtils
{
public:
    static BOOL AlphaBlendExt(HDC hDCDest, short x, short y, short cx, short cy, HDC hDCSrc,
                              short sx, short sy, short sw, short sh, short inAlpha);
    static void DrawAlphaBlendBitmap(CDC *inDC, short inDestX, short inDestY, HBITMAP inBmp,
                                     short inWidth, short inHeight, short inSourceX,
                                     short inSourceY, short inAlpha, BOOL inTransparentFlag = FALSE,
                                     COLORREF inColorTransparent = RGB(0x00, 0x00, 0x00));
    static void DrawBitmap(CDC *inDC, CPoint inPos, HBITMAP inBitmap);
    static void DrawTransparentBitmap(CDC *pDC, long x, long y, HBITMAP hBmp,
                                      COLORREF colorTransparent, long w = -1, long h = -1,
                                      long sx = 0, long sy = 0);
    static BOOL TransparentBltExt(HDC inDCDest, long inXOriginDest, long inYOriginDest,
                                  long inWidthDest, long inHeightDest, HDC inDCSrc,
                                  long inXOriginSrc, long inYOriginSrc, long inWidthSrc,
                                  long inHeightSrc, UINT inColorTransparent);

    static void GetStudioLogFont(LOGFONT &outLogFont);
    static unsigned __int64 StringToInt64(const char *inString, unsigned short inStringLen,
                                          unsigned short inBase);
    static void CopyFileToArchive(CString inFilename, CArchive &inDestination);
    static void CopyArchiveToFile(CArchive &inArchive, CString inDestination,
                                  unsigned long inLength);
    static wchar_t *WideFromMFC(::CString inMFCString);

protected:
    static unsigned __int64 RaisePower(unsigned __int64 inNumber, unsigned long inPower);
};

#endif // INCLUDED_WIN_UTILS_H

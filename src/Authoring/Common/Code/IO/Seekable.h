/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#ifndef STUDIOSEEKABLEH
#define STUDIOSEEKABLEH

#define SEEKABLE_METHOD_NOT_IMPLEMENTED -1

namespace Q3DStudio {
//==========================================================================
/**
  *	@class ISeekable
  * Abstract class to represent the ability to seek in a medium
  */
class ISeekable
{
public:
    enum ESeekPosition { EBEGIN = 1, ECURRENT = 2, EEND = 3 };
    //==========================================================================
    /**
      *	DTOR-does nothing.
      */
    virtual ~ISeekable() = 0;
    //==========================================================================
    /**
      *	Seeks to offset from position.
      *	@param inPosition The position to seek from.
      *	@param inOffset The offset to seek to.
      */
    virtual long Seek(ESeekPosition inPosition, long inOffset);
    //==========================================================================
    /**
      *	Returns the current offset of the input pointer from the beginning of the
      *	sink.
      */
    virtual long GetCurrentPosition();
};
}
#endif

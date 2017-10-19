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

#ifndef __UICRECTBASE_H__
#define __UICRECTBASE_H__

namespace Q3DStudio {
//==============================================================================
/**
*	@class	CRectBase	Base class that contains the data for a Q3DStudio::CRect.
*/
//==============================================================================
class CRectBase
{
public:
    CRectBase()
        : left(0)
        , top(0)
        , right(0)
        , bottom(0)
    {
    }

    long left;
    long top;
    long right;
    long bottom;
};

} // namespace Q3DStudio

#endif // __UICRECTBASE_H__

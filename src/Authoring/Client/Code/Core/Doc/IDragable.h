/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef __IDRAGABLE_H__
#define __IDRAGABLE_H__

//==============================================================================
//	Includes
//==============================================================================

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
//	Enums
//==============================================================================
enum EUIC_FLAVOR {
    EUIC_FLAVOR_FILE = 15,
    EUIC_FLAVOR_TEXT = 1,
    EUIC_FLAVOR_LISTBOX = 'LBOX',
    EUIC_FLAVOR_BASIC_OBJECTS = 'UIBO',
    EUIC_FLAVOR_ASSET_LIB = 'UILB',
    EUIC_FLAVOR_ASSET_TL = 'UITL',
    EUIC_FLAVOR_ASSET_UICFILE = 'UIFI',
};

//==============================================================================
/**
 *	@class
 *	@brief	IDragable provides an interface to abstract dragging from assets to be able
 *			to drag non-assets
 */
class IDragable
{
public:
    virtual long GetFlavor() const = 0;
};

#endif // #ifndef __IDRAGABLE_H__

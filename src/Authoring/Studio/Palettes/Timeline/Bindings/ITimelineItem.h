/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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

#ifndef INCLUDED_ITIMELINE_ITEM_H
#define INCLUDED_ITIMELINE_ITEM_H 1

#pragma once

#include "INamable.h"
#include "StudioObjectTypes.h"

class ITimelineTimebar;

//=============================================================================
/**
 * Abstraction of a data model item in the Scene. This might end up deriving from a more generic
 * interface, so that common
 * functions can be generalized for items in the different palettes.
 */
//=============================================================================
class ITimelineItem : public INamable
{
public:
    virtual ~ITimelineItem() {}

    virtual EStudioObjectType GetObjectType() const = 0;
    virtual bool IsMaster() const = 0;

    virtual bool IsShy() const = 0;
    virtual void SetShy(bool) = 0;
    virtual bool IsLocked() const = 0;
    virtual void SetLocked(bool) = 0;
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool) = 0;
    virtual bool IsImported() const { return false; }
    virtual bool IsVisibilityControlled() const = 0;

    // Actions
    virtual bool HasAction(bool inMaster) = 0;
    virtual bool ChildrenHasAction(bool inMaster) = 0;
    virtual bool ComponentHasAction(bool inMaster) = 0;

    virtual ITimelineTimebar *GetTimebar() = 0;
};

#endif // INCLUDED_ITIMELINE_ITEM_H

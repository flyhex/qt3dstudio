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

#ifndef INCLUDED_CMD_H
#define INCLUDED_CMD_H 1

#pragma once

#include "Qt3DSString.h"

class CCmd
{
public:
    // All the types of commands that can be fired off.
    // This is mostly used in the CanMerge functions.
    enum ECmdType {
        REARRANGE,
        DROP_REARRANGE,
        DELETE_OBJECT,
        ADD_OBJECT,
        KEYFRAME_CHANGE,
        PROPERTY_CHANGE,
        VECTOR_CHANGE,
        COLOR_CHANGE,
        TIMERANGE_CHANGE,
        TIMERANGE_TRUNCATION,
        BATCH,
        REMOVE_TRACK,
        INTERPOLATION_CHANGE,
        GENERIC,
        ACTION_ADD,
        ACTION_DELETE,
        ACTION_MOVE,
        ACTION_CUT,
        ACTION_COPY,
        ACTION_PASTE,
        ACTIONPARAM_SET,
        REFRESH_RESOURCE,
        DATAMODEL_SET_PROPERTY,
        DATAMODEL_SET_PROPERTY_INSTANCE_GUID,
        DATAMODEL_CLEAR_PROPERTY_INSTANCE_GUID
    };

    virtual ~CCmd();

    //==========================================================================
    /**
     * Perform the action this command is supposed to do.
     * This is called by the CmdStack when the command is first issued and
     * when the command is to be redone.
     * The return code of this is used to refresh whichever views need to be
     * refreshed, and is used as an argument to StudioDoc::UpdateAllViews. This
     * is returned to avoid batched commands causing multiple refreshes.
     *
     * @return a long used for the UpdateViews mask.
     */
    //==========================================================================
    virtual unsigned long Do() = 0;

    //==========================================================================
    /**
     * Undo the action that was performed.
     * This only needs to be functional if the command can be undoable.
     * This function is called by the CmdStack when this command is to be undone.
     * The return code of this is used to refresh whichever views need to be
     * refreshed, and is used as an argument to StudioDoc::UpdateAllViews. This
     * is returned to avoid batched commands causing multiple refreshes.
     *
     * @return a long used for the UpdateViews mask.
     */
    //==========================================================================
    virtual unsigned long Undo() = 0;

    virtual ECmdType GetType();

    virtual bool CanMerge(CCmd *inCommand);
    virtual void Merge(CCmd *inCommand);

    virtual void SetUndoable(bool inUndoable);
    virtual bool IsUndoable();

    virtual bool IsCommitted();
    virtual void SetCommitted(bool inCommitted);

    virtual void SetModifiedFlag(bool inShouldSetModifiedFlag);
    virtual bool ShouldSetModifiedFlag();

    virtual QString ToString();

    virtual bool IsInvalidated() { return false; }

protected:
    CCmd();
    bool m_Committed;
    bool m_Undoable;
    bool m_ShouldSetModified;
};

//==============================================================================
/**
 * Commands that do not modify the uip file, thus should not dirty the presentation and not
 * undoable.
 */
class CNonModifyingCmd : public CCmd
{
public:
    CNonModifyingCmd()
    {
        m_ShouldSetModified = false;
        m_Undoable = false;
    };
};

#endif // INCLUDED_CMD_H

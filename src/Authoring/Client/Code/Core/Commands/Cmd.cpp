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
#include "Cmd.h"

//=============================================================================
/**
 * Constructor: By default, all commands should set the modified flag.
 */
//=============================================================================
CCmd::CCmd()
    : m_Committed(false)
    , m_Undoable(true)
    , m_ShouldSetModified(true)
{
}

//=============================================================================
/**
 * Destructor
 */
//=============================================================================
CCmd::~CCmd()
{
}

//=============================================================================
/**
 * Set this command as being committed.
 * This is used by the CmdStack to avoid merging commands that should not be.
 * A command is automatically committed when it is undone (to avoid merges after
 * a redo command).
 *
 * @param inCommitted the new value for the committed flag.
 */
//=============================================================================
void CCmd::SetCommitted(bool inCommitted)
{
    m_Committed = inCommitted;
}

//=============================================================================
/**
 * Returns true if this command is committed.
 * If the command is committed no commands should be merged into it, but it
 * can be merged into others. After the merge the committed flag should be
 * carried over to the other command.
 */
//=============================================================================
bool CCmd::IsCommitted()
{
    return m_Committed;
}

//=============================================================================
/**
 * @return a semi-descriptive string of this command.
 */
//=============================================================================
QString CCmd::ToString()
{
    Q3DStudio::CString theString;
    theString.Format(_LSTR("Command - %d"), GetType());

    return theString.toQString();
}

//=============================================================================
/**
 * Allows you to dynamically specify whether or not a command should mark the
 * doc as dirty or not.
 *
 * @param inShouldSetModifiedFlag true if the modified flag should be set on the doc
 */
//=============================================================================
void CCmd::SetModifiedFlag(bool inShouldSetModifiedFlag)
{
    m_ShouldSetModified = inShouldSetModifiedFlag;
}

//=============================================================================
/**
 * This tells the Command Stack whether or not this command is modifying the
 * document. It is used to turn on/off the document's save flag.
 *
 * @return true if the document was modified.
 */
//=============================================================================
bool CCmd::ShouldSetModifiedFlag()
{
    return m_ShouldSetModified;
}

//=============================================================================
/**
 * Set whether this command can be undone or not.
 *
 * @param inUndoable true if this command is to be undoable.
 */
//=============================================================================
void CCmd::SetUndoable(bool inUndoable)
{
    m_Undoable = inUndoable;
}

//=============================================================================
/**
 * Checks to see if this command can be undone.
 * If it cannot be undone then the CmdStack will not put it into the Undo
 * list and will not modify the Redo list when this is performed. The CmdStack
 * will delete this command immediately.
 *
 * @return true if this command can be undone.
 */
//=============================================================================
bool CCmd::IsUndoable()
{
    return m_Undoable;
}

//=============================================================================
/**
 * Checks to see if the incoming command can be merged with this command.
 * The incoming command and this command should not be modified in any way.
 * @param inCommand the command to see if this can be merged with
 * @return true if the command can be merged.
 */
//=============================================================================
bool CCmd::CanMerge(CCmd *inCommand)
{
    Q_UNUSED(inCommand);
    return false;
}

//=============================================================================
/**
 * Merge the incoming command into this one.
 * The incoming command should not be modified in any way, this command will
 * reflect the merge.
 * This method should only be called after CanMerge has returned successfully
 * and this command is not committed.
 * @param inCommand the command to be merged into this one.
 */
//=============================================================================
void CCmd::Merge(CCmd *inCommand)
{
    Q_UNUSED(inCommand);
}

//=============================================================================
/**
 * Get the type of command this is.
 * The primary useage is for CanMerge to check the type of command.
 * @return the type of command this is.
 */
//=============================================================================
CCmd::ECmdType CCmd::GetType()
{
    return GENERIC;
}

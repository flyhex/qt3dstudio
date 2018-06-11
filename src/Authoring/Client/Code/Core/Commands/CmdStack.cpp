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

#include "Qt3DSCommonPrecompile.h"
#include "CmdStack.h"
#include "Cmd.h"
#include "CmdStackModifier.h"

CCmdStack::CCmdStack()
{
    m_Listener = NULL;
    m_MaxUndoStackSize = 100;
    m_CommandStackModifier = NULL;
}

CCmdStack::~CCmdStack()
{
    // Delete all the commands that are in the queues.
    Clear();
}

//=============================================================================
/**
 * Add a command to be executed.
 * If the command can be undone then it will be added to the undo/redo list.
 * Any commands that are in the redo list will be deleted and the redo list will
 * be flushed. If this command can be merged with the previous command then it
 * will be, after it is executed. If it is merged then it will be deleted
 * immediately. If the command cannot be undone then it will be deleted
 * immediately.
 *
 * @param inCommand the command to be executed.
 * @return bool true if inCommand was deleted, false otherwise.
 */
//=============================================================================
bool CCmdStack::ExecuteCommand(CCmd *inCommand)
{
    // Default the delete to true.
    bool shouldDeleteCommand = true;
    // Execute the command.
    unsigned long theUpdateMask = inCommand->Do();


    // If the listener is not null then do the notifications.
    if (m_Listener != NULL) {
        m_Listener->CommandUpdate(theUpdateMask);

        // Set the modified flag if it needs to be set.
        if (inCommand->ShouldSetModifiedFlag()) {
            m_Listener->SetCommandModifiedFlag(TRUE);
        }
    }

    // If the command can be undone then see if it needs to be merged or added to the undo list.
    if (inCommand->IsUndoable()) {
        // If there are previous commands then see if the command should be merged.
        if (m_UndoList.size() != 0) {
            CCmd *thePreviousCommand = m_UndoList.back();
            // If the previous command is committed then add the command.
            if (thePreviousCommand->IsCommitted()) {
                // Add the command and don't delete it.
                m_UndoList.push_back(inCommand);
                shouldDeleteCommand = false;
            }
            // Check if it can be merged.
            else if (thePreviousCommand->CanMerge(inCommand)) {
                inCommand->Merge(thePreviousCommand);
                delete thePreviousCommand;
                m_UndoList.pop_back();
                m_UndoList.push_back(inCommand);
                shouldDeleteCommand = false;
            } else {
                // Add the command and don't delete it.
                m_UndoList.push_back(inCommand);
                shouldDeleteCommand = false;
            }
        } else {
            // This is the first command in the list.
            m_UndoList.push_back(inCommand);
            shouldDeleteCommand = false;
        }

        // Cannot redo after a new command that can be undone.
        EmptyRedoStack();
    }

    // Delete the command if it needs to be.
    if (shouldDeleteCommand) {
        delete inCommand;
    }

    // Keep the undo list below it's max size.
    while (m_UndoList.size() > m_MaxUndoStackSize) {
        CCmd *theLastCommand = m_UndoList.front();
        delete theLastCommand;
        m_UndoList.erase(m_UndoList.begin());
    }

    return shouldDeleteCommand;
}

//=============================================================================
/**
 * Perform a single undo operation.
 * This will take the last command to be executed and undo it then push it onto
 * the redo stack. The command is set to committed so it will not be merged with
 * any other commands.
 */
//=============================================================================
void CCmdStack::Undo()
{

    if (m_CommandStackModifier) {
        if (m_CommandStackModifier->PreUndo() == false)
            return;
    }
    if (m_UndoList.size() > 0) {
        CCmd *theLastCommand = m_UndoList.back();
        m_UndoList.pop_back();

        unsigned long theUpdateMask = theLastCommand->Undo();


        // Once a command is undone then it is considered committed. Prevents merging after this has
        // been redone.
        theLastCommand->SetCommitted(true);

        m_RedoList.push_back(theLastCommand);

        // If the listener is not null then do the notifications.
        if (m_Listener != NULL) {
            m_Listener->CommandUpdate(theUpdateMask);

            // Set the modified flag if it needs to be set.
            if (theLastCommand->ShouldSetModifiedFlag()) {
                m_Listener->SetCommandModifiedFlag(TRUE);
            }
        }
    }
}

//=============================================================================
/**
 * Perform a single redo operation.
 * This will take the last command to be undone and redo it, then push it onto
 * the undo stack.
 */
//=============================================================================
void CCmdStack::Redo()
{
    if (m_RedoList.size() > 0) {
        CCmd *theLastCommand = m_RedoList.back();
        m_RedoList.pop_back();

        unsigned long theUpdateMask = theLastCommand->Do();

        m_UndoList.push_back(theLastCommand);

        // If the listener is not null then do the notifications.
        if (m_Listener != NULL) {
            m_Listener->CommandUpdate(theUpdateMask);

            // Set the modified flag if it needs to be set.
            if (theLastCommand->ShouldSetModifiedFlag()) {
                m_Listener->SetCommandModifiedFlag(TRUE);
            }
        }
    }
}

//=============================================================================
/**
 * @return true if an undo operation can be done.
 */
//=============================================================================
bool CCmdStack::CanUndo()
{
    if (m_CommandStackModifier && m_CommandStackModifier->CanUndo())
        return true;
    return m_UndoList.size() > 0;
}

//=============================================================================
/**
 * @return true if a redo operation can be done.
 */
//=============================================================================
bool CCmdStack::CanRedo()
{
    return m_RedoList.size() > 0;
}

//=============================================================================
/**
 * Commit the last command on the undo stack.
 * This marks the last command as committed and prevents it from merging with
 * any future commands.
 */
//=============================================================================
void CCmdStack::CommitLastCommand()
{
    if (m_UndoList.size() > 0) {
        CCmd *theLastCommand = m_UndoList.back();
        theLastCommand->SetCommitted(true);
    }
}

//=============================================================================
/**
 * Clear the entire undo/redo stack.
 * This is used when a new presentation is loaded and all the previous commands
 * are no longer needed. All commands will be deleted.
 */
//=============================================================================
void CCmdStack::Clear()
{
    EmptyRedoStack();
    EmptyUndoStack();
}

//=============================================================================
/**
 * Clear any invalid commands
 * This is called when commands are invalidated due objects being deleted.
 */
//=============================================================================
void CCmdStack::ClearInvalid()
{
    TCmdList::iterator thePos = m_UndoList.begin();
    while (thePos != m_UndoList.end()) {
        if ((*thePos)->IsInvalidated()) // continue with the next item that comes after.
        {
            delete (*thePos);
            thePos = m_UndoList.erase(thePos);
        } else
            ++thePos;
    }
    thePos = m_RedoList.begin();
    while (thePos != m_RedoList.end()) {
        if ((*thePos)->IsInvalidated()) // continue with the next item that comes after.
        {
            delete (*thePos);
            thePos = m_RedoList.erase(thePos);
        } else
            ++thePos;
    }
}

//=============================================================================
/**
 * Clears out the undo stack and deletes all the commands within it.
 * This is used on delete and when a new presentation is loaded up.
 */
//=============================================================================
void CCmdStack::EmptyUndoStack()
{
    TCmdList::iterator thePos;
    for (thePos = m_UndoList.begin(); thePos != m_UndoList.end(); ++thePos) {
        delete (*thePos);
    }

    m_UndoList.clear();
}

//=============================================================================
/**
 * Clears out the redo stack and deletes all the commands within it.
 * This is used after a new command is added redo can no longer be done. The
 * commands can be deleted because they can never be executed again.
 */
//=============================================================================
void CCmdStack::EmptyRedoStack()
{
    TCmdList::iterator thePos;
    for (thePos = m_RedoList.begin(); thePos != m_RedoList.end(); ++thePos) {
        delete (*thePos);
    }

    m_RedoList.clear();
}

//=============================================================================
/**
 * Gets the description of the next Undo command.
 *
 * @return the description or "" if no undo command is available.
 */
//=============================================================================
QString CCmdStack::GetUndoDescription()
{
    QString theDescription;
    if (m_UndoList.size() > 0) {
        theDescription = m_UndoList.back()->ToString();
    }
    return theDescription;
}

//=============================================================================
/**
 * Gets the description of the next Redo command.
 *
 * @return the description of "" if no redo command is available.
 */
//=============================================================================
QString CCmdStack::GetRedoDescription()
{
    QString theDescription;
    if (m_RedoList.size() > 0) {
        theDescription = m_RedoList.back()->ToString();
    }
    return theDescription;
}

void CCmdStack::SetCommandStackModifier(ICmdStackModifier *inModifier)
{
    m_CommandStackModifier = inModifier;
}

//=============================================================================
/**
 * Set a listener on this command stack which will be notified every time a
 * command has executed which should set the modified flag.
 *
 * @param inListener the listener to be notified.
 */
//=============================================================================
void CCmdStack::SetModificationListener(CModificationListener *inListener)
{
    m_Listener = inListener;
}

//=============================================================================
/**
 * Get the list of commands that can be undone.
 */
CCmdStack::TCmdList CCmdStack::GetUndoStack()
{
    return m_UndoList;
}

//=============================================================================
/**
 * Get the list of commands that can be redone.
 */
CCmdStack::TCmdList CCmdStack::GetRedoStack()
{
    return m_RedoList;
}

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

//==============================================================================
//	Prefix
//==============================================================================

#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================

//#ifndef INCLUDED_CMD_BATCH_H
#include "CmdBatch.h"
#include "Doc.h"
#include "UICDMStudioSystem.h"
#include "StudioFullSystem.h"
#include "UICDMSignals.h"
#include "Core.h"
#include "Dispatch.h"
//#endif

//=============================================================================
/**
 * Constructor. Nothing new here...
 */
//=============================================================================
CCmdBatch::CCmdBatch(CDoc *inDoc)
    : m_ExecutionCount(0)
    , m_Dispatch(inDoc->GetCore()->GetDispatch())
{
    m_Undoable = true;
}

CCmdBatch::CCmdBatch(CCore *inCore)
    : m_ExecutionCount(0)
    , m_Dispatch(inCore->GetDispatch())
{
    m_Undoable = true;
}

//=============================================================================
/**
 * Destructor, deletes all commands added to this.
 */
//=============================================================================
CCmdBatch::~CCmdBatch()
{
    this->Clear();
}

//=============================================================================
/**
 * Add a command to this batch.
 * Any time comparable batches are created the commands MUST be added in the
 * same order so that they can merge properly. All commands added to this will
 * be deleted by this object when it is deleted itself.
 *
 * @param inCommand the command to be added to this batch.
 */
//=============================================================================
void CCmdBatch::AddCommand(CCmd *inCommand, bool inAddFront)
{
    if (inAddFront)
        m_CommandList.insert(m_CommandList.begin(), inCommand);
    else
        m_CommandList.push_back(inCommand);
}

//=============================================================================
/**
 * Calls Do on every sub command.
 */
//=============================================================================
unsigned long CCmdBatch::Do()
{
    unsigned long theUpdateFlags = 0;

    CDispatchDataModelNotificationScope __dispatchScope(*m_Dispatch);

    TCmdList::iterator thePos = m_CommandList.begin();
    thePos += m_ExecutionCount;

    for (; thePos != m_CommandList.end(); ++thePos) {
        theUpdateFlags |= (*thePos)->Do();
        ++m_ExecutionCount;
    }

    return theUpdateFlags;
}

//=============================================================================
/**
 * Calls undo on every sub command.
 */
//=============================================================================
unsigned long CCmdBatch::Undo()
{
    unsigned long theUpdateFlags = 0;

    CDispatchDataModelNotificationScope __dispatchScope(*m_Dispatch);

    TCmdList::reverse_iterator thePos = m_CommandList.rbegin();
    for (; thePos != m_CommandList.rend(); ++thePos) {
        theUpdateFlags |= (*thePos)->Undo();
    }

    m_ExecutionCount = 0;

    return theUpdateFlags;
}

//=============================================================================
/**
 * @return the type of command that this is.
 */
//=============================================================================
CCmd::ECmdType CCmdBatch::GetType()
{
    return CCmd::BATCH;
}

//=============================================================================
/**
 * If inCommand is a Batch command and both batches have the same number of
 * sub commands and each sub command can merge with it's counterpart then this
 * will return true. Sub commands are merged to the sub command in the other
 * batch at the same index.
 *
 * @param inCommand the command to be check if it can be merged.
 * @return true if a merge can be done.
 */
//=============================================================================
bool CCmdBatch::CanMerge(CCmd *inCommand)
{
    bool theRetVal = false;

    // Check the type to make sure it is the same as this.
    if (inCommand->GetType() == this->GetType()) {
        CCmdBatch *theCommand = reinterpret_cast<CCmdBatch *>(inCommand);
        // Make sure both commands are the same size.
        if (theCommand->m_CommandList.size() == m_CommandList.size()) {
            // Go through all the sub commands and compare each one to the one at the same index on
            // the other command.
            TCmdList::iterator myPos = m_CommandList.begin();
            TCmdList::iterator itsPos = theCommand->m_CommandList.begin();

            theRetVal = true;
            for (; myPos != m_CommandList.end(); ++myPos, ++itsPos) {
                // Stop checking if one cannot merge.
                if (!(*myPos)->CanMerge(*itsPos)) {
                    // Set the return value to false, cannot merge.
                    theRetVal = false;
                    break;
                }
            }
        }
    }
    return theRetVal;
}

//=============================================================================
/**
 * Perform the merge of the two batch commands.
 * This will call Merge on each sub command of this with the command at the same
 * index on inCommand.
 *
 * @param inCommand the command to be merged.
 */
//=============================================================================
void CCmdBatch::Merge(CCmd *inCommand)
{
    CCmdBatch *theCommand = reinterpret_cast<CCmdBatch *>(inCommand);

    // Go through all the sub commands and merge each one with the one at the same index on the
    // other command.
    TCmdList::iterator myPos = m_CommandList.begin();
    TCmdList::iterator itsPos = theCommand->m_CommandList.begin();

    for (; myPos != m_CommandList.end(); ++myPos, ++itsPos) {
        (*myPos)->Merge(*itsPos);
    }
}

//=============================================================================
/**
 * This just uses the description of the first command within it, or blank if
 * there are no sub commands.
 *
 * @return a description of this command.
 */
//=============================================================================
QString CCmdBatch::ToString()
{
    if (m_CommandList.size() > 0) {
        CCmd *theFirstCommand = m_CommandList.at(0);
        return theFirstCommand->ToString();
    }
    return {};
}

//=============================================================================
/**
 * Set the committed state of this command.
 * This will set committed on all sub commands as well.
 *
 * @param inCommitted the new status of the committed flag.
 */
//=============================================================================
void CCmdBatch::SetCommitted(bool inCommitted)
{
    CCmd::SetCommitted(inCommitted);

    TCmdList::iterator thePos = m_CommandList.begin();
    for (; thePos != m_CommandList.end(); ++thePos) {
        (*thePos)->SetCommitted(inCommitted);
    }
}

//=============================================================================
/**
 * Checks to see if this command is undoable.
 * This will return true if it's undoable flag is true and if it has sub commands
 * and if all the sub commands can be undone.
 *
 * @return true if this can be undone.
 */
//=============================================================================
bool CCmdBatch::IsUndoable()
{
    bool isUndoable = m_Undoable;

    /* I am commenting this out because I feel this is more correct.
    if ( m_CommandList.size( ) == 0 )
    {
            isUndoable = false;
    }
    else
    {
    */
    TCmdList::iterator thePos = m_CommandList.begin();
    for (; thePos != m_CommandList.end(); ++thePos) {
        isUndoable &= (*thePos)->IsUndoable();
    }
    //}
    return isUndoable;
}

//=============================================================================
/**
 * Deletes all the commands within this and empties the list.
 */
//=============================================================================
void CCmdBatch::Clear()
{
    TCmdList::iterator thePos = m_CommandList.begin();
    for (; thePos != m_CommandList.end(); ++thePos) {
        delete (*thePos);
    }

    m_CommandList.clear();
    m_ExecutionCount = 0;
}

//=============================================================================
/**
 * Get the number of command objects within this batch, nice for testing.
 * @return the number of command objects in this batch, not recursive.
 */
//=============================================================================
size_t CCmdBatch::GetCommandCount()
{
    return m_CommandList.size();
}

//==============================================================================
/**
 * Go through the command list to erase any invalid commands
 * @return true if there command list is empty
 */
bool CCmdBatch::IsInvalidated()
{
    TCmdList::iterator thePos = m_CommandList.begin();
    while (thePos != m_CommandList.end()) {
        if ((*thePos)->IsInvalidated()) {
            delete (*thePos);
            thePos = m_CommandList.erase(thePos);
        } else
            ++thePos;
    }
    // if there are no more commands than, this batch is considered invalidated.
    return (m_CommandList.empty());
}

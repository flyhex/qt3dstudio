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

#ifndef INCLUDED_CMD_STACK_H
#define INCLUDED_CMD_STACK_H 1

#pragma once

#include "Qt3DSString.h"

class CCmd;
class ICmdStackModifier;

//=============================================================================
/**
 * Class to listen for changes on the command listener.
 */
//=============================================================================
class CModificationListener
{
public:
    //==========================================================================
    /**
     * This is called every time a command is executed which modifies the
     * modified flag on the command stack. It is used to notify the interested
     * document that a change has occurred.
     */
    //==========================================================================
    virtual void SetCommandModifiedFlag(bool inModified) = 0;

    //==========================================================================
    /**
     * Called when a command modifies one or more of the views and the views
     * need to be redrawn.
     *
     * @param inUpdateFlags flags of what needs to be redrawn.
     */
    //==========================================================================
    virtual void CommandUpdate(unsigned long inUpdateFlags) = 0;
};

class CCmdStack
{
public:
    typedef std::vector<CCmd *> TCmdList;

    CCmdStack();
    virtual ~CCmdStack();

    bool ExecuteCommand(CCmd *inCommand);
    void Undo();
    void Redo();

    bool CanUndo();
    bool CanRedo();

    QString GetUndoDescription();
    QString GetRedoDescription();

    void CommitLastCommand();

    void Clear();
    void ClearInvalid();

    void SetCommandStackModifier(ICmdStackModifier *inModifier);
    void SetModificationListener(CModificationListener *inListener);

    TCmdList GetUndoStack();
    TCmdList GetRedoStack();
    void EmptyRedoStack();

protected:
    void EmptyUndoStack();

    ICmdStackModifier *m_CommandStackModifier;

    TCmdList m_UndoList;
    TCmdList m_RedoList;

    unsigned long m_MaxUndoStackSize;

    CModificationListener *m_Listener;
};
#endif // INCLUDED_CMD_STACK_H

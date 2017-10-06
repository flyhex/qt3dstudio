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
#ifndef INCLUDED_CMD_GENERIC_H
#define INCLUDED_CMD_GENERIC_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#ifndef INCLUDED_CMD_H
#include "Cmd.h"
#endif
#include "StudioConst.h"

class CNull
{
};
class CDoc;

template <typename TClass, typename TArgType = CNull>
class CCmdGeneric : public CCmd
{
    typedef void (TClass::*TDoUndoMethod)();
    typedef void (TClass::*TOneArgDoUndoMethod)(TArgType);

public:
    //==========================================================================
    /**
     * Create a Generic command that calls the specified methods on Do and Undo.
     * This is a class for simple commands, makes it a bit quicker to make stuff.
     * The class takes in a do method with 0 or 1 args defined by each constructor
     * and a similar undo method
     *
     * @param inObject the object the methods are being called on.
     * @param inDoMethod the method to be called on Do.
     * @param inUndoMethod the method to be called on Undo.
     * @param inDescription the description for this command.
     */
    //==========================================================================
    CCmdGeneric(TClass *inObject, TDoUndoMethod inDoMethod, TDoUndoMethod inUndoMethod,
                Q3DStudio::CString inDescription, long inUpdateFlags = 0)
    {
        m_Object = inObject;
        m_DoMethod = inDoMethod;
        m_UndoMethod = inUndoMethod;
        m_Description = inDescription;
        m_NumArgs = 0;
        m_UpdateFlags = inUpdateFlags;
    }

    CCmdGeneric(TClass *inObject, TOneArgDoUndoMethod inDoMethod, TOneArgDoUndoMethod inUndoMethod,
                TArgType inDoArg, TArgType inUndoArg, const QString &inDescription,
                long inUpdateFlags = 0)
    {
        m_Object = inObject;
        m_OneArgDoMethod = inDoMethod;
        m_OneArgUndoMethod = inUndoMethod;
        m_DoArg = inDoArg;
        m_UndoArg = inUndoArg;
        m_Description = inDescription;
        m_NumArgs = 1;
        m_UpdateFlags = inUpdateFlags;
        m_DoMethod = NULL;
        m_UndoMethod = NULL;
    }

    CCmdGeneric(TClass *inObject, TDoUndoMethod inDoMethod)
    {
        m_Object = inObject;
        m_DoMethod = inDoMethod;
        m_UndoMethod = NULL;
        m_NumArgs = 0;
        m_UpdateFlags = 0;

        SetUndoable(false);
    }

    CCmdGeneric(TClass *inObject, TOneArgDoUndoMethod inDoMethod, TArgType inDoArg)
    {
        m_Object = inObject;
        m_OneArgDoMethod = inDoMethod;
        m_OneArgUndoMethod = NULL;
        m_DoArg = inDoArg;
        m_NumArgs = 1;
        m_UpdateFlags = 0;

        SetUndoable(false);
    }

    virtual ~CCmdGeneric() {}

    //==========================================================================
    /**
     * Performs this command, just calls the Do method on the object.
     *
     * @return 0.
     */
    //==========================================================================
    unsigned long Do() override
    {
        if (m_NumArgs == 0) {
            (m_Object->*m_DoMethod)();
        } else if (m_NumArgs == 1) {
            (m_Object->*m_OneArgDoMethod)(m_DoArg);
        }

        return m_UpdateFlags;
    }

    //==========================================================================
    /**
     * Undoes this command, just calls the Undo method on the object.
     *
     * @return 0.
     */
    //==========================================================================
    unsigned long Undo() override
    {
        if (m_NumArgs == 0) {
            (m_Object->*m_UndoMethod)();
        } else if (m_NumArgs == 1) {
            (m_Object->*m_OneArgUndoMethod)(m_UndoArg);
        }

        return m_UpdateFlags;
    }

    //==========================================================================
    /**
     * @return CCmd::GENERIC
     */
    //==========================================================================
    CCmd::ECmdType GetType() override { return CCmd::GENERIC; }

    //==========================================================================
    /**
     * NOP.
     */
    //==========================================================================
    bool CanMerge(CCmd *inCommand) override
    {
        Q_UNUSED(inCommand);
        return false;
    }

    //==========================================================================
    /**
     * NOP.
     */
    //==========================================================================
    void Merge(CCmd *inCommand) override { Q_UNUSED(inCommand); }

    //==========================================================================
    /**
     * @return the description from the constructor.
     */
    //==========================================================================
    QString ToString() override { return m_Description; }

protected:
    TClass *m_Object;
    TDoUndoMethod m_DoMethod;
    TDoUndoMethod m_UndoMethod;
    QString m_Description;
    TOneArgDoUndoMethod m_OneArgDoMethod;
    TOneArgDoUndoMethod m_OneArgUndoMethod;
    TArgType m_DoArg;
    TArgType m_UndoArg;
    long m_NumArgs;
    long m_UpdateFlags;
};

#endif // INCLUDED_CMD_GENERIC_H

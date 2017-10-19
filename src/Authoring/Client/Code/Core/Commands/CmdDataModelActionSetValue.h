/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_CMD_DATAMODEL_ACTION_SETVALUE_H
#define INCLUDED_CMD_DATAMODEL_ACTION_SETVALUE_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "Cmd.h"
#include "CmdDataModel.h"
#include "Doc.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMActionCore.h"
#include "Qt3DSDMActionSystem.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"

using namespace qt3dsdm;

class CCmdDataModelActionSetValue : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    CUICDMActionHandle m_Action;
    QString m_NiceText;

public: // Construction
    CCmdDataModelActionSetValue(CDoc *inDoc, CUICDMActionHandle inAction,
                                const QString &inNiceText)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Action(inAction)
        , m_NiceText(inNiceText)
    {
    }

    virtual void DoOperation() = 0;

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
            DoOperation();
        } else {
            DataModelRedo();
        }
        return 0;
    }

    //======================================================================
    //	Undo
    //======================================================================
    unsigned long Undo() override
    {
        if (ConsumerExists()) {
            DataModelUndo();
        }
        return 0;
    }

    //======================================================================
    //	ToString
    //======================================================================
    QString ToString() override { return m_NiceText; }
};

class CCmdDataModelActionSetTriggerObject : public CCmdDataModelActionSetValue
{
protected: // Members
    SObjectRefType m_Object;

    bool m_ResetEvent;
    wstring m_Event;

public: // Construction
    CCmdDataModelActionSetTriggerObject(CDoc *inDoc, CUICDMActionHandle inAction,
                                        const SObjectRefType &inObject)
        : CCmdDataModelActionSetValue(inDoc, inAction, "Set Trigger Object")
        , m_Object(inObject)
        , m_ResetEvent(false)
    {
    }

    void DoOperation() override
    {
        m_Doc->GetStudioSystem()->GetActionCore()->SetTriggerObject(m_Action, m_Object);
        if (m_ResetEvent) {
            m_Doc->GetStudioSystem()->GetActionCore()->SetEvent(m_Action, m_Event);
        }
    }

    virtual void ResetEvent(wstring inEvent)
    {
        m_ResetEvent = true;
        m_Event = inEvent;
    }
};

class CCmdDataModelActionSetEvent : public CCmdDataModelActionSetValue
{
protected: // Members
    wstring m_Event;

public: // Construction
    CCmdDataModelActionSetEvent(CDoc *inDoc, CUICDMActionHandle inAction, const wstring &inEvent)
        : CCmdDataModelActionSetValue(inDoc, inAction, "Set Event")
        , m_Event(inEvent)
    {
    }

    void DoOperation() override
    {
        m_Doc->GetStudioSystem()->GetActionCore()->SetEvent(m_Action, m_Event);
    }
};

class CCmdDataModelActionResetHandler : public CCmdDataModelActionSetValue
{
protected: // Members
    bool m_ResetHandler;
    wstring m_Handler;

protected: // helper functions
    virtual void ResetHandlerHandle()
    {
        m_Doc->GetStudioSystem()->GetActionCore()->SetHandler(m_Action, m_Handler);
    }

    virtual void ResetHandlerArguments(const wstring &inHandler)
    {
        m_Handler = inHandler;
        m_Doc->GetStudioSystem()->GetClientDataModelBridge()->ResetHandlerArguments(m_Action,
                                                                                    m_Handler);
    }

public: // Construction
    CCmdDataModelActionResetHandler(CDoc *inDoc, CUICDMActionHandle inAction,
                                    const QString &inNiceText)
        : CCmdDataModelActionSetValue(inDoc, inAction, inNiceText)
        , m_ResetHandler(false)
    {
    }

    // Changing Trigger Object may make the current Action Name to be unapplicable.
    // Changing Action Name may make the current Action Args to be unapplicable.
    // If that's the case, Reset the Action Name and/or Action Args.
    virtual void ResetHandler(const wstring &inHandler)
    {
        m_ResetHandler = true;
        m_Handler = inHandler;
    }
};

class CCmdDataModelActionSetTargetObject : public CCmdDataModelActionResetHandler
{
protected: // Members
    SObjectRefType m_Object;

public: // Construction
    CCmdDataModelActionSetTargetObject(CDoc *inDoc, CUICDMActionHandle inAction,
                                       const SObjectRefType &inObject)
        : CCmdDataModelActionResetHandler(inDoc, inAction, "Set Target Object")
        , m_Object(inObject)
    {
    }

    void DoOperation() override
    {
        m_Doc->GetStudioSystem()->GetActionCore()->SetTargetObject(m_Action, m_Object);
        if (m_ResetHandler) {
            ResetHandlerHandle();
            ResetHandlerArguments(m_Handler);
        }
    }
};

class CCmdDataModelActionSetHandler : public CCmdDataModelActionResetHandler
{
protected: // Members
    qt3dsdm::wstring m_Handler;

public: // Construction
    CCmdDataModelActionSetHandler(CDoc *inDoc, CUICDMActionHandle inAction,
                                  qt3dsdm::wstring inHandler)
        : CCmdDataModelActionResetHandler(inDoc, inAction, "Set Handler")
        , m_Handler(inHandler)
    {
    }

    void DoOperation() override
    {
        m_Doc->GetStudioSystem()->GetActionCore()->SetHandler(m_Action, m_Handler);
        if (m_ResetHandler)
            ResetHandlerArguments(m_Handler);
    }
};

class CCmdDataModelActionSetArgumentValue : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    CUICDMHandlerArgHandle m_HandlerArgument;
    SValue m_Value;

public: // Construction
    CCmdDataModelActionSetArgumentValue(CDoc *inDoc, CUICDMHandlerArgHandle inHandlerArgument,
                                        SValue inValue)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_HandlerArgument(inHandlerArgument)
        , m_Value(inValue)
    {
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
            m_Doc->GetStudioSystem()->GetClientDataModelBridge()->SetHandlerArgumentValue(
                m_HandlerArgument, m_Value);
        } else {
            DataModelRedo();
        }
        return 0;
    }

    //======================================================================
    //	Undo
    //======================================================================
    unsigned long Undo() override
    {
        if (ConsumerExists()) {
            DataModelUndo();
        }
        return 0;
    }

    //======================================================================
    //	ToString
    //======================================================================
    QString ToString() override
    {
        return QObject::tr("Set Action Argument Value");
    }
};

class CCmdDataModelActionSetEyeball : public CCmdDataModelActionSetValue
{
protected: // Members
    bool m_Value;
    Qt3DSDMSlideHandle m_ActiveSlide;

public: // Construction
    CCmdDataModelActionSetEyeball(CDoc *inDoc, Qt3DSDMSlideHandle inActiveSlide,
                                  CUICDMActionHandle inAction, bool inValue)
        : CCmdDataModelActionSetValue(inDoc, inAction, "Set Action Eyeball")
        , m_Value(inValue)
        , m_ActiveSlide(inActiveSlide)
    {
    }

    void DoOperation() override
    {
        m_Doc->GetStudioSystem()->GetActionSystem()->SetActionEyeballValue(m_ActiveSlide, m_Action,
                                                                           m_Value);
    }
};

#endif

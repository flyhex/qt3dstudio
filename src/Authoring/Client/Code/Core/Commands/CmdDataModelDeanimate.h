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

#ifndef INCLUDED_CMD_DATAMODEL_DEANIMATE_H
#define INCLUDED_CMD_DATAMODEL_DEANIMATE_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "Cmd.h"
#include "CmdDataModel.h"
#include "Doc.h"
#include "Qt3DSDMHandles.h"
#include "CmdDataModelBase.h"
#include "Qt3DSDMPropertyDefinition.h"
#include "Qt3DSDMDataCore.h"

// Keyframes are always set at the property level, never for individual animation handle.
class CCmdDataModelAnimate : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    qt3dsdm::Qt3DSDMPropertyHandle m_Property;

public: // Construction
    CCmdDataModelAnimate(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                         qt3dsdm::Qt3DSDMPropertyHandle inProperty)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Instance(inInstance)
        , m_Property(inProperty)
    {
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            using namespace qt3dsdm;

            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
            IStudioAnimationSystem *theAnimationSystem =
                m_Doc->GetStudioSystem()->GetAnimationSystem();
            theAnimationSystem->Animate(m_Instance, m_Property);
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
        return QObject::tr("Animate");
    }
};

// Keyframes are always set at the property level, never for individual animation handle.
class CCmdDataModelDeanimate : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    qt3dsdm::Qt3DSDMPropertyHandle m_Property;

public: // Construction
    CCmdDataModelDeanimate(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Instance(inInstance)
        , m_Property(inProperty)
    {
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            using namespace qt3dsdm;

            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);

            IStudioAnimationSystem *theAnimationSystem =
                m_Doc->GetStudioSystem()->GetAnimationSystem();
            theAnimationSystem->Deanimate(m_Instance, m_Property);
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
        return QObject::tr("Deanimate");
    }
};

#endif

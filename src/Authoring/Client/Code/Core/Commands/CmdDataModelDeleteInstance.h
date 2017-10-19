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

#ifndef INCLUDED_CMD_DATAMODEL_DELETEINSTANCE_H
#define INCLUDED_CMD_DATAMODEL_DELETEINSTANCE_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "Cmd.h"
#include "CmdDataModel.h"
#include "Doc.h"
#include "Qt3DSDMHandles.h"

class CCmdDataModelDeleteInstance : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;

public: // Construction
    CCmdDataModelDeleteInstance(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Instance(inInstance)
    {
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            SetConsumer();
            m_Doc->GetStudioSystem()->GetPropertySystem()->DeleteInstance(m_Instance);
            ReleaseConsumer();
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
        return QObject::tr("Delete Instance");
    }
};

class CCmdDataModelDeleteComponentInstance : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;

public: // Construction
    CCmdDataModelDeleteComponentInstance(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance)
        : CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Instance(inInstance)
    {
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            SetConsumer();
            qt3dsdm::ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
            qt3dsdm::Qt3DSDMSlideHandle theSlide = theSlideSystem->GetSlideByInstance(m_Instance);
            theSlideSystem->DeleteSlideByIndex(theSlide, 0);

            ReleaseConsumer();
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
        return QObject::tr("Delete Component Instance");
    }
};
#endif

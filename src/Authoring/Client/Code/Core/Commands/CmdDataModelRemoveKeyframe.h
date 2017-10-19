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

#ifndef INCLUDED_CMD_DATAMODEL_REMOVEKEYFRAME_H
#define INCLUDED_CMD_DATAMODEL_REMOVEKEYFRAME_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "Cmd.h"
#include "CmdDataModel.h"
#include "Doc.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMAnimation.h"
#include "CmdDataModelBase.h"
#include "Qt3DSDMStudioSystem.h"

class CCmdDataModelRemoveKeyframe : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    std::vector<qt3dsdm::CUICDMKeyframeHandle> m_Keyframes; // 1..n keyframes to be deleted

public: // Construction
    //@param inTime is in secs
    CCmdDataModelRemoveKeyframe(CDoc *inDoc, qt3dsdm::CUICDMKeyframeHandle inKeyframe)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
    {
        AddKeyframeHandle(inKeyframe);
    }
    ~CCmdDataModelRemoveKeyframe() {}

    void AddKeyframeHandle(qt3dsdm::CUICDMKeyframeHandle inKeyframe)
    {
        m_Keyframes.push_back(inKeyframe);
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
            qt3dsdm::IAnimationCore *theAnimationCore = m_Doc->GetStudioSystem()->GetAnimationCore();
            for (size_t i = 0; i < m_Keyframes.size(); ++i)
                theAnimationCore->EraseKeyframe(m_Keyframes[i]);
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
        return QObject::tr("Delete Keyframe");
    }
};

#endif

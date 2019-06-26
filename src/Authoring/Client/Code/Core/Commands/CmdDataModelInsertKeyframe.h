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

#ifndef INCLUDED_CMD_DATAMODEL_INSERTKEYFRAME_H
#define INCLUDED_CMD_DATAMODEL_INSERTKEYFRAME_H 1

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
#include "Qt3DSDMAnimation.h"

// This will animate the property if it is not already animated
class CCmdDataModelInsertKeyframe : public CCmd, public qt3dsdm::CmdDataModel
{
public:
    struct STimeKeyframeData
    {
        qt3dsdm::Qt3DSDMPropertyHandle m_Property;
        float m_KeyframeTime;
        qt3dsdm::SGetOrSetKeyframeInfo m_Infos[4];
        size_t m_ValidInfoCount;

        STimeKeyframeData(qt3dsdm::Qt3DSDMPropertyHandle inProperty, float inKeyframeTime,
                          qt3dsdm::SGetOrSetKeyframeInfo *inInfos, size_t inInfoCount)
            : m_Property(inProperty)
            , m_KeyframeTime(inKeyframeTime)
        {
            m_ValidInfoCount = inInfoCount <= 4 ? inInfoCount : 4;
            for (size_t idx = 0, end = m_ValidInfoCount; idx < end; ++idx)
                m_Infos[idx] = inInfos[idx];
        }
    };

protected:
    typedef std::vector<STimeKeyframeData> TKeyframeDataList;

protected: // Members
    CDoc *m_Doc;
    qt3dsdm::Qt3DSDMInstanceHandle m_Instance;
    TKeyframeDataList m_KeyframeDataList;

public: // Construction
    //@param inTime is in secs
    CCmdDataModelInsertKeyframe(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                qt3dsdm::Qt3DSDMPropertyHandle inProperty, float inKeyframeTime,
                                qt3dsdm::SGetOrSetKeyframeInfo *inInfos, size_t inInfoCount)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Instance(inInstance)
    {
        AddKeyframeData(inProperty, inKeyframeTime, inInfos, inInfoCount);
    }
    ~CCmdDataModelInsertKeyframe() {}

    void AddKeyframeData(qt3dsdm::Qt3DSDMPropertyHandle inProperty, float inTime,
                         qt3dsdm::SGetOrSetKeyframeInfo *inInfos, size_t inInfoCount)
    {
        m_KeyframeDataList.push_back(STimeKeyframeData(inProperty, inTime, inInfos, inInfoCount));
    }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
            qt3dsdm::IStudioAnimationSystem *theAnimationSystem =
                m_Doc->GetStudioSystem()->GetAnimationSystem();
            // if there are existing keyframes exist at the same times, the values are overridden. (
            // That's how it always work in studio anyways )
            for (size_t i = 0; i < m_KeyframeDataList.size(); ++i)
                theAnimationSystem->SetOrCreateKeyframe(
                    m_Instance, m_KeyframeDataList[i].m_Property,
                    m_KeyframeDataList[i].m_KeyframeTime, m_KeyframeDataList[i].m_Infos,
                    m_KeyframeDataList[i].m_ValidInfoCount);
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
        return QObject::tr("Insert Keyframe");
    }
};

#endif

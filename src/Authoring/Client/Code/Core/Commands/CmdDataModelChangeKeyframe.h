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

#ifndef INCLUDED_CMD_DATAMODEL_CHANGEKEYFRAME_H
#define INCLUDED_CMD_DATAMODEL_CHANGEKEYFRAME_H 1

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

class CCmdDataModelSetKeyframeTime : public CCmdDataModelBase<long>
{

public:
    CCmdDataModelSetKeyframeTime(CDoc *doc, qt3dsdm::Qt3DSDMKeyframeHandle keyframe, long time)
        : CCmdDataModelBase(doc, time)
        , m_Keyframe(keyframe)
    {
    }
    ~CCmdDataModelSetKeyframeTime() {}

    void DoOperation() override
    {
        qt3dsdm::IAnimationCore *theAnimationCore = m_Doc->GetStudioSystem()->GetAnimationCore();
        qt3dsdm::TKeyframe theKeyframe = theAnimationCore->GetKeyframeData(m_Keyframe);
        theKeyframe = setKeyframeTime(theKeyframe, m_Value);
        theAnimationCore->SetKeyframeData(m_Keyframe, theKeyframe);
    }

    QString ToString() override
    {
        return QObject::tr("Set Keyframe Time");
    }

protected:
    qt3dsdm::Qt3DSDMKeyframeHandle m_Keyframe;
};

class CCmdDataModelSetKeyframeValue : public CCmdDataModelBase<float>
{
protected:
    qt3dsdm::Qt3DSDMKeyframeHandle m_Keyframe;

public:
    CCmdDataModelSetKeyframeValue(CDoc *inDoc, qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                                  float inValue)
        : CCmdDataModelBase(inDoc, inValue)
        , m_Keyframe(inKeyframe)
    {
    }
    ~CCmdDataModelSetKeyframeValue() {}

    void DoOperation() override
    {
        qt3dsdm::IAnimationCore *theAnimationCore = m_Doc->GetStudioSystem()->GetAnimationCore();
        qt3dsdm::TKeyframe theKeyframe = theAnimationCore->GetKeyframeData(m_Keyframe);
        theKeyframe = setKeyframeValue(theKeyframe, m_Value);
        theAnimationCore->SetKeyframeData(m_Keyframe, theKeyframe);
    }

    QString ToString() override
    {
        return QObject::tr("Set Keyframe Value");
    }
};

class CCmdDataModelChangeDynamicKeyframe : public CCmd, public qt3dsdm::CmdDataModel
{
protected: // Members
    typedef std::set<qt3dsdm::Qt3DSDMAnimationHandle> TTrackList;
    CDoc *m_Doc;
    TTrackList m_Tracks; // 1..n tracks affected, unique list
    bool m_Dynamic;

public: // Construction
    CCmdDataModelChangeDynamicKeyframe(CDoc *inDoc, qt3dsdm::Qt3DSDMAnimationHandle inHandle,
                                       bool inDynamic)
        : qt3dsdm::CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Dynamic(inDynamic)
    {
        AddHandle(inHandle);
    }
    ~CCmdDataModelChangeDynamicKeyframe() {}

    void AddHandle(qt3dsdm::Qt3DSDMAnimationHandle inHandle) { m_Tracks.insert(inHandle); }

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            qt3dsdm::SScopedDataModelConsumer __scopedConsumer(*this);
            qt3dsdm::IAnimationCore *theAnimationCore = m_Doc->GetStudioSystem()->GetAnimationCore();
            TTrackList::iterator theIter = m_Tracks.begin();
            for (; theIter != m_Tracks.end(); ++theIter)
                theAnimationCore->SetFirstKeyframeDynamic(*theIter, m_Dynamic);
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
        return QObject::tr("Dynamic Keyframe");
    }
};

#endif

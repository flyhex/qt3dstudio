/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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

#pragma once

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "ITimelineTimebar.h"
#include "Qt3DSDMHandles.h"
#include "IDocumentEditor.h"

///////////////////////////////////////////////////////////////////////////////
// Forwards
class CTimelineTranslationManager;

namespace Q3DStudio {
class IDocumentEditor;
}

namespace qt3dsdm {
class IPropertySystem;
class ISignalConnection;
}

//=============================================================================
/**
 * General timebar implementation for UICDM objects
 */
class CUICDMTimelineTimebar : public ITimelineTimebar, public Q3DStudio::CUpdateableDocumentEditor
{
public:
    CUICDMTimelineTimebar(CTimelineTranslationManager *inTimelineTranslationManager,
                          qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);
    virtual ~CUICDMTimelineTimebar();

protected:
    CTimelineTranslationManager *m_TimelineTranslationManager;
    qt3dsdm::IPropertySystem *m_PropertySystem;
    qt3dsdm::Qt3DSDMInstanceHandle m_DataHandle; // The Instance Handle for this Timeline Timeber.
    qt3dsdm::Qt3DSDMPropertyHandle m_StartTime;
    qt3dsdm::Qt3DSDMPropertyHandle m_EndTime;
    ::CColor m_Color; // Timebar color
    Q3DStudio::CString m_Comment; // Timebar comment text
    std::shared_ptr<qt3dsdm::ISignalConnection> m_PropertyChangedSignal;
    void OnPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty);

public:
    // ITimelineTimebar
    long GetStartTime() const override;
    long GetEndTime() const override;
    long GetDuration() const override;
    bool ShowHandleBars() const override;
    void OnBeginDrag() override;
    void OffsetTime(long inDiff) override;
    void ChangeTime(long inTime, bool inSetStart) override;
    void CommitTimeChange() override;
    void RollbackTimeChange() override;
    ::CColor GetTimebarColor() override { return m_Color; }
    void SetTimebarColor(const ::CColor &inColor) override;
    Q3DStudio::CString GetTimebarComment() override { return m_Comment; }
    void SetTimebarComment(const Q3DStudio::CString &inComment) override;
    void SetTimebarTime(ITimeChangeCallback *inCallback = nullptr) override;
};

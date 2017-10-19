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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_LAYER_TIMELINEITEM_BINDING_H
#define INCLUDED_LAYER_TIMELINEITEM_BINDING_H 1

#pragma once

#include "Qt3DSDMTimelineItemBinding.h"

namespace qt3dsdm {
class CStudioSystem;
}

//=============================================================================
/**
 * Binding to generic UICDM object
 */
class CLayerTimelineItemBinding : public Qt3DSDMTimelineItemBinding
{
public: // Types
    typedef std::tuple<qt3dsdm::TCharStr, qt3dsdm::TCharStr, qt3dsdm::Qt3DSDMPropertyHandle>
        TNameFormalNamePair;
    typedef std::vector<TNameFormalNamePair> TNameFormalNamePairList;

protected: // Members
    TNameFormalNamePairList m_ImageNameFormalNamePairs;

public: // Construction
    CLayerTimelineItemBinding(CTimelineTranslationManager *inMgr,
                              qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);
    virtual ~CLayerTimelineItemBinding();

public: // Qt3DSDMTimelineItemBinding
    EStudioObjectType GetObjectType() const override;
    // Hierarchy
    ITimelineItemBinding *GetChild(long inIndex) override;
    void OnAddChild(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    // Event callback
    void OnPropertyChanged(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle) override;

protected:
    qt3dsdm::Qt3DSDMInstanceHandle GetImage(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle);
    ITimelineItemBinding *GetOrCreateImageBinding(qt3dsdm::Qt3DSDMPropertyHandle inPropertyHandle,
                                                  const wchar_t *inName);
};

#endif // INCLUDED_LAYER_TIMELINEITEM_BINDING_H

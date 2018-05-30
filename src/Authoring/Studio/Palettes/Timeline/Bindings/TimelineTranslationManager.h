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
#ifndef INCLUDED_TIMELINE_TRANSLATIONMANAGER_H
#define INCLUDED_TIMELINE_TRANSLATIONMANAGER_H 1

#pragma once

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMTimeline.h"

#include "Doc.h"

class ITimelineItemBinding;
class Qt3DSDMTimelineItemBinding;

// DataModel
namespace qt3dsdm {
class CStudioSystem;
}

class CDoc;

/**
 * There is a TranslationManager per presentation (project)
 */
class CTimelineTranslationManager
{
protected: // Typedefs
    // DataModel support
    typedef std::map<qt3dsdm::Qt3DSDMInstanceHandle, Qt3DSDMTimelineItemBinding *>
        TInstanceHandleBindingMap;

    // Store expanded state
    typedef std::map<qt3dsdm::Qt3DSDMInstanceHandle, bool> TInstanceHandleExpandedMap; // DataModel support

protected: // Properties
    // DataModel support
    TInstanceHandleBindingMap m_InstanceHandleBindingMap;

public:
    CTimelineTranslationManager();
    ~CTimelineTranslationManager();

public:
    ITimelineItemBinding *GetOrCreate(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void Clear();

    Qt3DSDMTimelineItemBinding *GetBinding(qt3dsdm::Qt3DSDMInstanceHandle inHandle) const;

    qt3dsdm::CStudioSystem *GetStudioSystem() const;

    CDoc *GetDoc() const;
};

#endif // INCLUDED_TIMELINE_TRANSLATIONMANAGER_H

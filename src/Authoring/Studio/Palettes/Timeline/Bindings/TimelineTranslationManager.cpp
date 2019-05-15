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

#include "TimelineTranslationManager.h"
#include "SlideTimelineItemBinding.h"
#include "GroupTimelineItemBinding.h"
#include "BehaviorTimelineItemBinding.h"
#include "MaterialTimelineItemBinding.h"
#include "ImageTimelineItemBinding.h"
#include "PathAnchorPointTimelineItemBinding.h"
#include "PathTimelineItemBinding.h"
#include "LayerTimelineItemBinding.h"
#include "Qt3DSDMStudioSystem.h"
#include "StudioObjectTypes.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "ClientDataModelBridge.h"

using namespace qt3dsdm;

CTimelineTranslationManager::CTimelineTranslationManager()
{
}

CTimelineTranslationManager::~CTimelineTranslationManager()
{
    // clean up all bindings
    Clear();
}

ITimelineItemBinding *CTimelineTranslationManager::GetOrCreate(Qt3DSDMInstanceHandle inInstance)
{
    ITimelineItemBinding *theBinding = GetBinding(inInstance);
    if (!theBinding) {
        Qt3DSDMTimelineItemBinding *theReturn = nullptr;

        EStudioObjectType objType = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()
                                    ->GetClientDataModelBridge()->GetObjectType(inInstance);

        if (objType & OBJTYPE_IS_MATERIAL) {
            theReturn = new CMaterialTimelineItemBinding(this, inInstance);
        } else if (objType == OBJTYPE_IMAGE) {
            theReturn = new CImageTimelineItemBinding(this, inInstance);
        } else if (objType & (OBJTYPE_GROUP | OBJTYPE_COMPONENT)) {
            theReturn = new CGroupTimelineItemBinding(this, inInstance);
        } else if (objType == OBJTYPE_BEHAVIOR) {
            theReturn = new CBehaviorTimelineItemBinding(this, inInstance);
        } else if (objType == OBJTYPE_SLIDE) {
            theReturn = new CSlideTimelineItemBinding(this, inInstance);
        } else if (objType == OBJTYPE_PATHANCHORPOINT) {
            theReturn = new CPathAnchorPointTimelineItemBinding(this, inInstance);
        } else if (objType == OBJTYPE_PATH) {
            theReturn = new CPathTimelineItemBinding(this, inInstance);
        } else if (objType == OBJTYPE_LAYER) {
            theReturn = new CLayerTimelineItemBinding(this, inInstance);
        } else if (objType & (OBJTYPE_MODEL | OBJTYPE_TEXT | OBJTYPE_CAMERA | OBJTYPE_EFFECT
                              | OBJTYPE_LIGHT | OBJTYPE_RENDERPLUGIN | OBJTYPE_ALIAS
                              | OBJTYPE_SUBPATH))
            theReturn = new Qt3DSDMTimelineItemBinding(this, inInstance);
        else {
            // Add support for additional DataModel types here.
            Q_ASSERT(0);
        }

        m_InstanceBindingMap.insert({theReturn->GetInstanceHandle(), theReturn});
        theBinding = theReturn;
    }

    return theBinding;
}

/**
 * Clear all bindings, typically when a presentation is closed.
 */
void CTimelineTranslationManager::Clear()
{
    // clean up all bindings
    m_InstanceBindingMap.clear();
}

/**
 * @return the Binding object that corresponds to this instance.
 */
Qt3DSDMTimelineItemBinding *
CTimelineTranslationManager::GetBinding(Qt3DSDMInstanceHandle inHandle) const
{
    auto it = m_InstanceBindingMap.find(inHandle);
    if (it != m_InstanceBindingMap.end())
        return it->second;

    return nullptr;
}

CDoc *CTimelineTranslationManager::GetDoc() const
{
    return dynamic_cast<CDoc *>(g_StudioApp.GetCore()->GetDoc());
}

CStudioSystem *CTimelineTranslationManager::GetStudioSystem() const
{
    return GetDoc()->GetStudioSystem();
}


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

#include "stdafx.h"

#include "CmdActivateSlide.h"
#include "Doc.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMStudioSystem.h"
#include "SlideSystem.h"
#include "ClientDataModelBridge.h"

#include <QObject>

CCmdActivateSlide::CCmdActivateSlide(CDoc *inDoc, qt3dsdm::CUICDMSlideHandle inSlideHandle)
    : m_Doc(inDoc)
    , m_SlideHandle(inSlideHandle)
    , m_ForceRefresh(true)
{
}

CCmdActivateSlide::CCmdActivateSlide(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    : m_Doc(inDoc)
    , m_ForceRefresh(true)
{
    CClientDataModelBridge *theBridge = m_Doc->GetStudioSystem()->GetClientDataModelBridge();
    qt3dsdm::ISlideSystem *theSlideSystem = m_Doc->GetStudioSystem()->GetSlideSystem();
    Q3DStudio::CId theId = theBridge->GetGUID(inInstance);
    qt3dsdm::CUICDMSlideHandle theMasterSlide =
        theSlideSystem->GetMasterSlideByComponentGuid(GuidtoSLong4(theId));
    m_SlideHandle = theSlideSystem->GetActiveSlide(theMasterSlide);
}

CCmdActivateSlide::~CCmdActivateSlide()
{
}

unsigned long CCmdActivateSlide::Do()
{
    if (m_Doc->IsPlaying())
        m_Doc->SetPlayMode(PLAYMODE_STOP);
    m_Doc->NotifyActiveSlideChanged(m_SlideHandle, m_ForceRefresh);
    return 0;
}

unsigned long CCmdActivateSlide::Undo()
{
    return 0;
}

QString CCmdActivateSlide::ToString()
{
    return QObject::tr("Activate Slide");
}

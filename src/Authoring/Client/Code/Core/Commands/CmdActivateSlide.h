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

#ifndef INCLUDED_CMD_ACTIVATE_SLIDE_H
#define INCLUDED_CMD_ACTIVATE_SLIDE_H 1

#pragma once

#include "Cmd.h"
#include "UICDMHandles.h"

class CDoc;

namespace qt3dsdm {
class ISlideSystem;
class CUICDMSlideHandle;
};

class CCmdActivateSlide : public CNonModifyingCmd
{
public:
    CCmdActivateSlide(CDoc *inDoc, qt3dsdm::CUICDMSlideHandle inSlideHandle);
    CCmdActivateSlide(CDoc *inDoc, qt3dsdm::CUICDMInstanceHandle inInstance);
    virtual ~CCmdActivateSlide();

    unsigned long Do() override;
    unsigned long Undo() override;

    QString ToString() override;

    void SetForceRefresh(bool inForce) { m_ForceRefresh = inForce; }

protected:
    CDoc *m_Doc;
    qt3dsdm::CUICDMSlideHandle m_SlideHandle;
    bool m_ForceRefresh;
};
#endif // INCLUDED_CMD_ACTIVATE_SLIDE_H

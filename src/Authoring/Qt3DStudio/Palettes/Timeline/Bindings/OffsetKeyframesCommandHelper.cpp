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

#include "Qt3DSCommonPrecompile.h"
#include "OffsetKeyframesCommandHelper.h"
#include "Core.h"

// Data model specific
#include "IDoc.h"
#include "CmdDataModelChangeKeyframe.h"
#include "IDocumentEditor.h"

#include "Qt3DSDMTimelineKeyframe.h" //TODO: remove once we resolve the precision issue

using namespace qt3dsdm;

COffsetKeyframesCommandHelper::COffsetKeyframesCommandHelper(CDoc &inDoc)
    : Q3DStudio::CUpdateableDocumentEditor(inDoc)
    , m_Doc(inDoc)
{
}

COffsetKeyframesCommandHelper::~COffsetKeyframesCommandHelper()
{
    Finalize();
}

//@param inTime time in millisecs
void COffsetKeyframesCommandHelper::SetCommandTime(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                                                   long inTime)
{
    // The DataModel system will take care of merging these under the hood.
    ENSURE_EDITOR(QObject::tr("Set Keyframe Time")).SetKeyframeTime(inKeyframe, inTime);
}

// equivalent to commit (onmouseup)
void COffsetKeyframesCommandHelper::Finalize()
{
    CommitEditor();
}

/****************************************************************************
**
** Copyright (C) 1999-2003 NVIDIA Corporation.
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
//	Includes
//==============================================================================
#ifndef INCLUDED_DROPSOURCE
#define INCLUDED_DROPSOURCE

#include "Qt3DSDMHandles.h"
#include "Pt.h"
#include "Cmd.h"
#include <QMimeData>

typedef enum _EDROPDESTINATION {
    EDROPDESTINATION_ON, ///< drop occurs on the target asset
    EDROPDESTINATION_ABOVE, ///< drop occurs above the target asset
    EDROPDESTINATION_BELOW ///< drop occurs below the target asset
} EDROPDESTINATION;

class CDropTarget;
class IDragable;
class CStudioApp;

class CDropSource : public QMimeData
{
protected:
    long m_Flavor;
    unsigned long m_Size;

    qt3dsdm::TInstanceHandleList m_Instances;
    long m_ObjectType = 0;
    int m_FileType = 0;
    bool m_HasValidTarget = false;
    CPt m_CurrentPoint;
    Qt::KeyboardModifiers m_CurrentFlags = 0;

public:
    CDropSource(long inFlavor, unsigned long inSize = 0);
    virtual ~CDropSource();

    virtual bool CanMove() = 0;
    virtual bool CanCopy() = 0;
    long GetObjectType() const { return m_ObjectType; }
    long getFileType() const { return m_FileType; }
    long GetFlavor()  const { return m_Flavor; }
    virtual bool ValidateTarget(CDropTarget *) = 0;

    virtual bool GetHasValidTarget() const { return m_HasValidTarget; }
    virtual void SetHasValidTarget(bool inValid) { m_HasValidTarget = inValid; }
    virtual void InterpretKeyFlags(long) {}

    virtual void SetCurrentPoint(CPt &inPoint) { m_CurrentPoint = inPoint; }
    virtual CPt GetCurrentPoint() const { return m_CurrentPoint; }

    virtual void SetCurrentFlags(Qt::KeyboardModifiers inFlags) { m_CurrentFlags = inFlags; }
    virtual Qt::KeyboardModifiers GetCurrentFlags() const { return m_CurrentFlags; }

    virtual CCmd *GenerateAssetCommand(qt3dsdm::Qt3DSDMInstanceHandle, EDROPDESTINATION,
                                       qt3dsdm::Qt3DSDMSlideHandle)
    {
        return nullptr;
    }
    virtual CCmd *GenerateSlideCommand(long) { return nullptr; }
};

class CDropSourceFactory
{
public:
    static CDropSource *Create(long inFlavor, IDragable *inDragable);
    static CDropSource *Create(long inFlavor, const QString &filePath);
};

#endif

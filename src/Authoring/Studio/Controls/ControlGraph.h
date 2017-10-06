/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#ifndef CONTROLGRAPHH
#define CONTROLGRAPHH
#include "Multicaster.h"
#include "ControlGraphIterators.h"

class CControl;
class CControlWindowListener;
class CChildFocusListener;
class CRenderer;

namespace Q3DStudio {
namespace Graph {

    struct SGraphPosition; // GraphPosition.h"
}

namespace Control {
    class ControlData;

    namespace ControlGraph {
        void AddNode(std::shared_ptr<CControlData> inData);
        void RemoveNode(CControl &inData);
        void AddChild(CControl &inParent, CControl &inChild, CControl *inNextSibling);
        // inParent is supplied for error checking purposes.
        void RemoveChild(CControl &inParent, CControl &inChild);
        void RemoveAllChildren(CControl &inParent);
        void MoveTo(CControl &inParent, CControl &inChild, const Graph::SGraphPosition &inPosition);
        long GetNumChildren(CControl &inControl);
        long GetChildIndex(CControl &inParent, CControl &inChild);
        std::shared_ptr<CControlData> GetChild(CControl &inParent, long inIndex);
        std::shared_ptr<CControlData> GetParent(CControl &inControl);

        SReverseIterator GetRChildren(CControl &inControl);
        SIterator GetChildren(CControl &inControl);
    };
}
}

#endif

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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMHandles.h"
#include "IEasyInspectorRowListener.h"
#include "IDataDrivenChangeListener.h"
#include "DispatchListeners.h"
#include "Qt3DSDMMetaDataTypes.h"
#include "CmdBatch.h"

//==============================================================================
//	Forwards
//==============================================================================
class CDoc;
class CEasyInspectorRow;

// DataModel
namespace qt3dsdm {
class ISignalConnection;
}

class CGenericEdit;
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 * This is a binding between a DataModelInspectable and an EasyInspectorRow
 */
class Qt3DSDMInspectorRow
{
    //==============================================================================
    //	Members
    //==============================================================================
protected:
    qt3dsdm::Qt3DSDMMetaDataPropertyHandle m_MetaProperty;
    qt3dsdm::SMetaDataPropertyInfo m_MetaDataPropertyInfo;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    Qt3DSDMInspectorRow(CDoc *inDoc, qt3dsdm::Qt3DSDMMetaDataPropertyHandle inProperty);
    virtual ~Qt3DSDMInspectorRow();

private: // Disabled parameterless construction
    Qt3DSDMInspectorRow();

public: // Use
    qt3dsdm::Qt3DSDMMetaDataPropertyHandle GetMetaDataProperty() const
    {
      return m_MetaProperty;
    }
    const qt3dsdm::SMetaDataPropertyInfo &GetMetaDataPropertyInfo() const
    {
      return m_MetaDataPropertyInfo;
    }
};

} // namespace Q3DStudio

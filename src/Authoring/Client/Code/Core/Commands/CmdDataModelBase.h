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

#ifndef INCLUDED_CMD_DATAMODEL_BASE_H
#define INCLUDED_CMD_DATAMODEL_BASE_H 1

#pragma once

//==============================================================================
//	Include
//==============================================================================
#include "Cmd.h"
#include "CmdDataModel.h"
#include "Doc.h"
#include "UICDMHandles.h"

//==============================================================================
/**
 * Base class that provide basic Update/Finalize functionality, when changing UICDM data.
 */
template <class TDataType>
class CCmdDataModelBase : public CCmd, public UICDM::CmdDataModel
{
protected: // Members
    CDoc *m_Doc;
    TDataType m_Value;

public: // Construction
    CCmdDataModelBase(CDoc *inDoc, TDataType inValue)
        : CmdDataModel(*inDoc)
        , m_Doc(inDoc)
        , m_Value(inValue)
    {
    }

    virtual void DoOperation() = 0;

    //======================================================================
    //	Do/Redo
    //======================================================================
    unsigned long Do() override
    {
        if (!ConsumerExists()) {
            SetConsumer();
            DoOperation();
        } else {
            DataModelRedo();
        }

        return 0;
    }

    virtual void Update(const TDataType &inValue)
    {
        if (!ConsumerExists())
            SetConsumer();
        m_Value = inValue;
        DoOperation();
    }

    void Finalize(const TDataType &inValue)
    {
        if (ConsumerExists()) {
            m_Value = inValue;
            DoOperation();
        }
        ReleaseConsumer();
    }

    //======================================================================
    //	Undo
    //======================================================================
    unsigned long Undo() override
    {
        if (ConsumerExists()) {
            DataModelUndo();
        }

        return 0;
    }

    //======================================================================
    //	ToString
    //======================================================================
    QString ToString() override = 0;

    TDataType GetValue() const { return m_Value; }
};

#endif

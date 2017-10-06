/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#ifndef CMDDATAMODELH
#define CMDDATAMODELH
#include "UICDMHandles.h"
#include "UICString.h"

class CDoc;
class CDispatch;

namespace UICDM {
class ITransactionConsumer;
class ITransactionProducer;
struct CTransactionConsumer;

struct SApplicationState
{
    bool m_Dirty;
    UICDM::CUICDMInstanceHandle m_SelectedInstance;
    UICDM::CUICDMSlideHandle m_ActiveSlide;
    UICDM::CUICDMInstanceHandle m_ActiveLayer;

    SApplicationState()
        : m_Dirty(false)
    {
    }
    void Store(CDoc &inDoc);
    void PreNotify(const SApplicationState &inOther, CDoc &inDoc);
    void Notify(const SApplicationState &inOther, CDoc &inDoc);
};

class CmdDataModel
{
public:
    Q3DStudio::CString m_Name;
    Q3DStudio::CString m_File;
    int m_Line;

    CmdDataModel(CDoc &inDoc);
    ~CmdDataModel();
    void SetName(const Q3DStudio::CString &inName);
    Q3DStudio::CString GetName() const;
    bool HasTransactions();
    bool ConsumerExists();
    void SetConsumer();
    void ReleaseConsumer(bool inRunNotifications = true);
    void DataModelUndo();
    void DataModelRedo();
    void DataModelRollback();
    void RunDoNotifications();
    void RunUndoNotifications();
    void CheckForSelectionChange(CUICDMInstanceHandle inOldInstance,
                                 CUICDMInstanceHandle inNewInstance);

protected:
    std::shared_ptr<CTransactionConsumer> m_Consumer;
    CDoc &m_Doc;

    // The application state before this command
    SApplicationState m_BeforeDoAppState;
    // The application state after this command.
    SApplicationState m_AfterDoAppState;

private:
    void SetConsumer(ITransactionProducer *inProducer);
    void ReleaseConsumer(ITransactionProducer *inProducer, bool inRunNotifications);
};

struct SScopedDataModelConsumer
{
    CmdDataModel &m_Cmd;
    bool m_Opened;
    SScopedDataModelConsumer(CmdDataModel &cmd)
        : m_Cmd(cmd)
        , m_Opened(false)
    {
        if (cmd.ConsumerExists() == false) {
            m_Cmd.SetConsumer();
            m_Opened = true;
        }
    }
    ~SScopedDataModelConsumer()
    {
        if (m_Opened)
            m_Cmd.ReleaseConsumer();
    }
};
};

#endif
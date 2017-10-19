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
#include "stdafx.h"
#include "CmdDataModel.h"
#include <boost/bind.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/bind/apply.hpp>
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMStudioSystem.h"
#include "Doc.h"
#include "Core.h"
#include "Dispatch.h"
#include <boost/make_shared.hpp>
#include "foundation/Qt3DSLogging.h"

using namespace boost;

namespace qt3dsdm {

void SApplicationState::Store(CDoc &inDoc)
{
    m_Dirty = inDoc.IsModified();
    m_SelectedInstance = inDoc.GetSelectedInstance();
    m_ActiveSlide = inDoc.GetActiveSlide();
    m_ActiveLayer = inDoc.GetActiveLayer();
}

void SApplicationState::Notify(const SApplicationState &inOther, CDoc &inDoc)
{
    if (m_Dirty != inOther.m_Dirty)
        inDoc.SetModifiedFlag(m_Dirty);

    if (m_ActiveLayer != inOther.m_ActiveLayer)
        inDoc.SetActiveLayer(m_ActiveLayer);

    if (m_ActiveSlide != inOther.m_ActiveSlide)
        inDoc.NotifyActiveSlideChanged(m_ActiveSlide, true);

    if (m_SelectedInstance != inOther.m_SelectedInstance)
        inDoc.SelectUICDMObject(m_SelectedInstance);
}

// We clear the selection just as a precaution because we want to be sure that
// if this command tends to delete things that there is nothing referring
// to a deleted item
void SApplicationState::PreNotify(const SApplicationState &inOther, CDoc &inDoc)
{
    if (m_SelectedInstance != inOther.m_SelectedInstance) {
        inDoc.DeselectAllItems(false);
    }
}

CmdDataModel::CmdDataModel(CDoc &inDoc)
    : m_Doc(inDoc)
{
}
CmdDataModel::~CmdDataModel()
{
}

void CmdDataModel::SetName(const Q3DStudio::CString &inName)
{
    m_Name = inName;
}
Q3DStudio::CString CmdDataModel::GetName() const
{
    return m_Name;
}
bool CmdDataModel::HasTransactions()
{
    return m_Consumer != NULL
        && ((CTransactionConsumer *)m_Consumer.get())->m_TransactionList.size() > 0;
}

bool CmdDataModel::ConsumerExists()
{
    return m_Consumer != NULL;
}

void CmdDataModel::SetConsumer(ITransactionProducer *inProducer)
{
    if (!ConsumerExists())
        m_Consumer = std::make_shared<CTransactionConsumer>();
    inProducer->SetConsumer(m_Consumer);
}

void CmdDataModel::ReleaseConsumer(ITransactionProducer *inProducer, bool inRunNotifications)
{
    inProducer->SetConsumer(TTransactionConsumerPtr());
    if (inRunNotifications)
        RunDoNotifications();
}

void CmdDataModel::SetConsumer()
{
    if (!ConsumerExists()) {
        m_Doc.GetCore()->GetDispatch();
        SetConsumer(m_Doc.GetStudioSystem());
        m_Doc.GetAssetGraph()->SetConsumer(m_Consumer);
        m_BeforeDoAppState.Store(m_Doc);
    }
}

void CmdDataModel::ReleaseConsumer(bool inRunNotifications)
{
    if (ConsumerExists()) {
        m_Doc.GetAssetGraph()->SetConsumer(TTransactionConsumerPtr());
        if (HasTransactions())
            m_Doc.SetModifiedFlag(true);
        m_AfterDoAppState.Store(m_Doc);
        ReleaseConsumer(m_Doc.GetStudioSystem(), inRunNotifications);
    }
}

void CmdDataModel::DataModelUndo()
{
    if (ConsumerExists() && HasTransactions()) {
        qCInfo(qt3ds::TRACE_INFO) << "Undoing " << m_Name.GetCharStar()
                  << " generated from: " << m_File.GetCharStar() << "(" << m_Line << ")";
        m_AfterDoAppState.Store(m_Doc);
        m_AfterDoAppState.PreNotify(m_BeforeDoAppState, m_Doc);
        Undo(m_Consumer->m_TransactionList);
        RunUndoNotifications();
    }
}

void CmdDataModel::RunUndoNotifications()
{
    if (ConsumerExists()) {
        CDispatchDataModelNotificationScope __dispatchScope(*m_Doc.GetCore()->GetDispatch());
        NotifyReverse(m_Consumer->m_UndoNotifications);
        m_BeforeDoAppState.Notify(m_AfterDoAppState, m_Doc);
    }
}

void CmdDataModel::DataModelRedo()
{
    if (ConsumerExists() && HasTransactions()) {
        qCInfo(qt3ds::TRACE_INFO) << "Redoing " << m_Name.GetCharStar()
                  << " generated from: " << m_File.GetCharStar() << "(" << m_Line << ")";
        m_BeforeDoAppState.Store(m_Doc);
        m_BeforeDoAppState.PreNotify(m_AfterDoAppState, m_Doc);
        Redo(m_Consumer->m_TransactionList);
        RunDoNotifications();
    }
}

void CmdDataModel::RunDoNotifications()
{
    if (ConsumerExists()) {
        CDispatchDataModelNotificationScope __dispatchScope(*m_Doc.GetCore()->GetDispatch());
        Notify(m_Consumer->m_DoNotifications);
        m_AfterDoAppState.Notify(m_BeforeDoAppState, m_Doc);
    }
}

void CmdDataModel::DataModelRollback()
{
    if (ConsumerExists()) {
        DataModelUndo();
        m_Consumer->m_DoNotifications.clear();
        m_Consumer->m_UndoNotifications.clear();
        m_Consumer->m_TransactionList.clear();
    }
}
}

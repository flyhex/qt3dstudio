/****************************************************************************
**
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

#include "SlideModel.h"

#include "CmdActivateSlide.h"
#include "Core.h"
#include "Doc.h"
#include "StudioApp.h"

#include "IDocumentEditor.h"

#include "ClientDataModelBridge.h"
#include "UICDMStudioSystem.h"
#include "UICDMSlides.h"

SlideModel::SlideModel(int slideCount, QObject *parent) : QAbstractListModel(parent)
  , m_slides(slideCount)
{

}

QVariant SlideModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(),index.parent()))
        return {};

    const auto row = index.row();

    switch (role) {
    case NameRole:
        return slideName(m_slides[row]);
    case SelectedRole:
        return row == m_selectedRow;
    }

    return {};
}

bool SlideModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!hasIndex(index.row(), index.column(),index.parent()))
        return false;

    auto &slideHandle = m_slides[index.row()];

    switch (role) {
    case NameRole: {
        setSlideName(slideHandle, value.toString());
        Q_EMIT dataChanged(index, index, {role});
        break;
    }
    case HandleRole: {
        slideHandle = value.value<qt3dsdm::CUICDMSlideHandle>();
        Q_EMIT dataChanged(index, index, {HandleRole, NameRole});
        break;
    }
    case SelectedRole: {
        m_selectedRow = value.toBool() ? index.row() : -1;

        if (m_selectedRow != -1) {
            CCmdActivateSlide *theCmd = new CCmdActivateSlide(GetDoc(), m_slides[m_selectedRow]);
            g_StudioApp.GetCore()->ExecuteCommand(theCmd);
        }

        Q_EMIT dataChanged(this->index(0, 0), this->index(rowCount() - 1, 0), {role});
        return true;
    }
    default:
        return false;
    }

    return true;
}

int SlideModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_slides.count();
}

QHash<int, QByteArray> SlideModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(NameRole, "name");
    names.insert(SelectedRole, "selected");

    return names;
}

bool SlideModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > m_slides.count())
        return false;

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; i++)
        m_slides.insert(row, {});
    endInsertRows();

    setData(index(row + count - 1), true, SelectedRole);
    return true;
}

bool SlideModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row + count > m_slides.count())
        return false;

    bool selectionRemoved = false;
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; i++) {
        if (m_selectedRow == row)
            selectionRemoved = true;
        m_slides.removeAt(row);
    }
    endRemoveRows();

    auto newSelectedRow = -1;
    if (selectionRemoved) {
        if (row > 0)
            newSelectedRow = row - 1;
        else
            newSelectedRow = row + count - 1;
    } else if (m_selectedRow >= m_slides.count()) {
        newSelectedRow = m_slides.count() - 1;
    }
    if (newSelectedRow != -1)
        setData(index(newSelectedRow), true, SelectedRole);

    return true;
}

void SlideModel::duplicateRow(int row)
{
    const auto handle = m_slides[row];

    beginInsertRows({}, row, row);
    m_slides.insert(row + 1, Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(),
                                                               QObject::tr("Duplicate Slide"))
                    ->DuplicateSlide(handle));
    endInsertRows();
    setData(index(row + 1), true, SelectedRole);

    Q_EMIT dataChanged(index(row, 0), index(row + 1, 0), {});
}

void SlideModel::move(int fromRow, int toRow)
{
    if (fromRow == toRow)
        return;

    auto handle = m_slides[fromRow];
    // toRow + 1 as DocumentEditor uses 1 based indexes for slides
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Rearrange Slide"))
            ->RearrangeSlide(handle, toRow + 1);

    if (fromRow > toRow)
        beginMoveRows({}, fromRow, fromRow, {}, toRow);
    else
        beginMoveRows({}, fromRow, fromRow, {}, toRow + 1);
    m_slides.move(fromRow, toRow);
    endMoveRows();
}

void SlideModel::clear()
{
    beginResetModel();
    m_slides.clear();
    endResetModel();
}

void SlideModel::addNewSlide(int row)
{
    const auto handle = (row < m_slides.size()) ? m_slides[row] : m_slides.last();

    const auto instanceHandle = GetBridge()->GetOwningComponentInstance(handle);
    qt3dsdm::CUICDMSlideHandle theMasterSlide = GetBridge()->GetComponentSlide(instanceHandle, 0);

    beginInsertRows({}, row, row);
    m_slides.insert(row, Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(),
                                                           QObject::tr("Create Slide"))
                    ->AddSlide(theMasterSlide, row + 1));
    endInsertRows();

    setData(index(row), true, SelectedRole);
}

void SlideModel::removeSlide(int row)
{
    const auto handle = m_slides[row];

    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Delete Slide"))->DeleteSlide(handle);
    removeRows(row, 1);
}

bool SlideModel::hasSlideWithName(const QString &name) const
{
    for (const auto &slide: m_slides) {
        if (slideName(slide) == name)
            return true;
    }
    return false;
}

QString SlideModel::slideName(const qt3dsdm::CUICDMSlideHandle &handle) const
{
    auto doc = GetDoc();
    if (!doc->IsValid())
        return {};
    const auto instanceHandle = doc->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(handle);
    return GetBridge()->GetName(instanceHandle).toQString();
}

void SlideModel::setSlideName(const qt3dsdm::CUICDMSlideHandle &handle, const QString &name)
{
    const auto oldName = slideName(handle);
    if (oldName != name && !name.trimmed().isEmpty()) {
        using namespace qt3dsdm;
        CDoc *theDoc = GetDoc();
        CClientDataModelBridge *theBridge = GetBridge();
        if (!theBridge)
            return;
        const auto instanceHandle = GetDoc()->GetStudioSystem()->
                GetSlideSystem()->GetSlideInstance(handle);
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*theDoc, QObject::tr("Set Slide Name"))
                ->SetSlideName(instanceHandle, theBridge->GetNameProperty(),
                               Q3DStudio::CString::fromQString(oldName),
                               Q3DStudio::CString::fromQString(name));
    }
}

CDoc *SlideModel::GetDoc() const
{
    return g_StudioApp.GetCore()->GetDoc();
}

CClientDataModelBridge *SlideModel::GetBridge() const
{
    auto doc = GetDoc();
    if (!doc->IsValid())
        return nullptr;
    return doc->GetStudioSystem()->GetClientDataModelBridge();
}



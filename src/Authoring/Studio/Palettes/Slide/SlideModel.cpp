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
#include "SlideSystem.h"
#include "IDocumentEditor.h"

#include "ClientDataModelBridge.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"

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
    case VariantsRole:
        int slideIdx = GetDoc()->GetStudioSystem()->GetSlideSystem()->GetSlideIndex(m_slides[row]);
        if (slideIdx < m_variantsModel.size()) {
            const auto variantsDef = g_StudioApp.GetCore()->getProjectFile().variantsDef();
            const auto keys = m_variantsModel[slideIdx].keys();
            QString templ = QString::fromWCharArray(L"<font color='%1'>\u25A0</font>");
            QString slideVariants;
            for (auto g : keys) // variants groups
                slideVariants.append(templ.arg(variantsDef[g].m_color));

            return slideVariants;
        }
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
        slideHandle = value.value<qt3dsdm::Qt3DSDMSlideHandle>();
        qt3dsdm::Qt3DSDMInstanceHandle instanceHandle
                = GetDoc()->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(slideHandle);
        m_slideLookupHash.insert(instanceHandle, slideHandle);
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
        break;
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
    names.insert(VariantsRole, "variants");
    names.insert(SelectedRole, "selected");

    return names;
}

bool SlideModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > m_slides.count())
        return false;

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
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
    for (int i = 0; i < count; ++i) {
        if (m_selectedRow == row + i)
            selectionRemoved = true;
        m_slides.removeAt(row);
    }
    endRemoveRows();

    auto newSelectedRow = -1;
    if (selectionRemoved) {
        if (row > 0)
            newSelectedRow = row - 1;
        else
            newSelectedRow = 0;
    } else if (m_selectedRow > row) {
        newSelectedRow = m_selectedRow - count;
    }
    if (newSelectedRow != -1)
        setData(index(newSelectedRow), true, SelectedRole);

    return true;
}

void SlideModel::duplicateRow(int row)
{
    const auto handle = m_slides[row];
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Duplicate Slide"))
            ->DuplicateSlide(handle);
}

void SlideModel::startRearrange(int row)
{
    m_rearrangeStartRow = row;
    m_rearrangeEndRow = -1;
}

void SlideModel::move(int fromRow, int toRow)
{
    if (fromRow == toRow)
        return;

    onSlideRearranged({}, fromRow + 1, toRow + 1);
}

void SlideModel::finishRearrange(bool commit)
{
    if (m_rearrangeEndRow != m_rearrangeStartRow
            && m_rearrangeEndRow >= 0 && m_rearrangeStartRow >= 0) {
        // Restore state before committing the actual change
        // +1 added as DocumentEditor uses 1 based indexes for slides
        int endRow = m_rearrangeEndRow + 1;
        onSlideRearranged({}, endRow, m_rearrangeStartRow + 1);

        if (commit) {
            auto handle = m_slides[m_rearrangeStartRow];
            m_rearrangeStartRow = -1;
            Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Rearrange Slide"))
                    ->RearrangeSlide(handle, endRow);
        }
    }
    m_rearrangeEndRow = -1;
    m_rearrangeStartRow = -1;
}

void SlideModel::clear()
{
    beginResetModel();
    m_slides.clear();
    m_slideLookupHash.clear();
    endResetModel();
}

void SlideModel::addNewSlide(int row)
{
    const auto handle = (row < m_slides.size()) ? m_slides[row] : m_slides.last();
    const auto instanceHandle = GetBridge()->GetOwningComponentInstance(handle);
    qt3dsdm::Qt3DSDMSlideHandle theMasterSlide = GetBridge()->GetComponentSlide(instanceHandle, 0);
    Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Create Slide"))
            ->AddSlide(theMasterSlide, row + 1);
}

void SlideModel::removeSlide(int row)
{
    // Don't allow deleting of the last slide
    if (m_slides.size() > 1) {
        const auto handle = m_slides[row];
        Q3DStudio::SCOPED_DOCUMENT_EDITOR(*GetDoc(), QObject::tr("Delete Slide"))->DeleteSlide(
                    handle);
    }
}

void SlideModel::onNewSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    qt3dsdm::ISlideSystem &theSlideSystem(*GetDoc()->GetStudioSystem()->GetSlideSystem());

    // Ignore new slides added to random different components
    if (m_slides.size() && theSlideSystem.GetMasterSlide(inSlide)
            != theSlideSystem.GetMasterSlide(m_slides[0])) {
        return;
    }

    finishRearrange(false); // Cancel any uncommitted rearrange

    // Find the slide index
    int row = int(slideIndex(inSlide));

    // Slide index zero indicates master slide. We can't add master slides
    Q_ASSERT(row > 0);

    --row;

    beginInsertRows({}, row, row);
    m_slides.insert(row, inSlide);
    qt3dsdm::Qt3DSDMInstanceHandle instanceHandle
            = GetDoc()->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(inSlide);
    m_slideLookupHash.insert(instanceHandle, inSlide);
    endInsertRows();

    setData(index(row), true, SelectedRole);
}

void SlideModel::onDeleteSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    for (int i = 0; i < m_slides.size(); ++i) {
        if (m_slides[i] == inSlide) {
            if (m_rearrangeStartRow >= 0) {
                finishRearrange(false); // Cancel any uncommitted rearrange
                // We need to re-resolve the index after rearrange cancel
                for (int j = 0; j < m_slides.size(); ++j) {
                    if (m_slides[j] == inSlide) {
                        i = j;
                        break;
                    }
                }
            }
            QList<qt3dsdm::Qt3DSDMInstanceHandle> keys = m_slideLookupHash.keys(inSlide);
            for (auto key : keys)
                m_slideLookupHash.remove(key);
            removeRows(i, 1);
            break;
        }
    }
}

void SlideModel::onSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int fromRow,
                                   int toRow)
{
    if (inMaster.Valid()) {
        // If imMaster is valid, this was triggered by either a rearrange commit or
        // undo/redo of a rearrange, so we need to cancel any uncommitted rearrange.
        finishRearrange(false);
        // Check we are working on correct slide set
        qt3dsdm::ISlideSystem &theSlideSystem(*GetDoc()->GetStudioSystem()->GetSlideSystem());
        if (fromRow - 1 >= m_slides.size()
                || inMaster != theSlideSystem.GetMasterSlide(m_slides[fromRow - 1]))
            return;
    } else {
        // Do not do uncommitted rearranges if we haven't started a rearrange (or more likely
        // an uncommitted rearrange was canceled while in progress by undo/redo operation)
        if (m_rearrangeStartRow < 0)
            return;
    }

    // -1 because internal slide model has 1-based indexing for non-master slides
    if (fromRow > toRow)
        beginMoveRows({}, fromRow - 1, fromRow - 1, {}, toRow - 1);
    else
        beginMoveRows({}, fromRow - 1, fromRow - 1, {}, toRow);
    m_slides.move(fromRow - 1, toRow - 1);
    m_rearrangeEndRow = toRow - 1;

    endMoveRows();
}

bool SlideModel::hasSlideWithName(const QString &name) const
{
    for (const auto &slide: m_slides) {
        if (slideName(slide) == name)
            return true;
    }
    return false;
}

QString SlideModel::slideName(const qt3dsdm::Qt3DSDMSlideHandle &handle) const
{
    auto doc = GetDoc();
    if (!doc->IsValid())
        return {};
    const auto instanceHandle = doc->GetStudioSystem()->GetSlideSystem()->GetSlideInstance(handle);
    return GetBridge()->GetName(instanceHandle).toQString();
}

void SlideModel::setSlideName(const qt3dsdm::Qt3DSDMSlideHandle &handle, const QString &name)
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

void SlideModel::refreshVariants(const QVector<QHash<QString, QStringList>> &vModel)
{
    m_variantsModel.clear();

    if (vModel.isEmpty()) {
        const auto *slideSystem = GetDoc()->GetStudioSystem()->GetSlideSystem();
        int slideCount = slideSystem->GetSlideCount(slideSystem->GetMasterSlide(
                                                                 GetDoc()->GetActiveSlide()));
        m_variantsModel.resize(slideCount);

        const auto propertySystem = GetDoc()->GetPropertySystem();
        const auto layers = GetDoc()->getLayers();
        for (auto layer : layers) {
            int slideIdx = slideIndex(slideSystem->GetAssociatedSlide(layer));
            qt3dsdm::SValue sValue;
            if (propertySystem->GetInstancePropertyValue(layer, GetBridge()->GetLayer().m_variants,
                                                         sValue)) {
                QString propVal = qt3dsdm::get<qt3dsdm::TDataStrPtr>(sValue)->toQString();
                if (!propVal.isEmpty()) {
                    QStringList tagPairs = propVal.split(QLatin1Char(','));
                    for (int i = 0; i < tagPairs.size(); ++i) {
                        QStringList pair = tagPairs[i].split(QLatin1Char(':'));
                        if (!m_variantsModel[slideIdx][pair[0]].contains(pair[1]))
                            m_variantsModel[slideIdx][pair[0]].append(pair[1]);
                    }
                }
            }
        }

        // add master slide variants to other slides
        const auto keys = m_variantsModel[0].keys();
        for (int i = 1; i < slideCount; ++i) {
            for (auto g : keys) {
                for (int j = 0; j < m_variantsModel[0][g].length(); ++j) {
                    if (!m_variantsModel[i][g].contains(m_variantsModel[0][g][j]))
                        m_variantsModel[i][g].append(m_variantsModel[0][g][j]);
                }
            }
        }
    } else {
        m_variantsModel = vModel;
    }

    Q_EMIT dataChanged(this->index(0, 0), this->index(rowCount() - 1, 0), {VariantsRole});
}

CDoc *SlideModel::GetDoc() const
{
    return g_StudioApp.GetCore()->GetDoc();
}

long SlideModel::slideIndex(const qt3dsdm::Qt3DSDMSlideHandle &handle)
{
    return GetDoc()->GetStudioSystem()->GetSlideSystem()->GetSlideIndex(handle);
}

CClientDataModelBridge *SlideModel::GetBridge() const
{
    auto doc = GetDoc();
    if (!doc->IsValid())
        return nullptr;
    return doc->GetStudioSystem()->GetClientDataModelBridge();
}

QVector<QHash<QString, QStringList> > SlideModel::variantsModel() const
{
    return m_variantsModel;
}

QHash<QString, QStringList> SlideModel::variantsSlideModel(int row) const
{
    int slideIdx = GetDoc()->GetStudioSystem()->GetSlideSystem()->GetSlideIndex(m_slides[row]);
    return m_variantsModel[slideIdx];
}

void SlideModel::refreshSlideLabel(qt3dsdm::Qt3DSDMInstanceHandle instanceHandle,
                                   qt3dsdm::Qt3DSDMPropertyHandle propertyHandle)
{
    if (m_slideLookupHash.contains(instanceHandle)
            && propertyHandle == GetBridge()->GetNameProperty()) {
        qt3dsdm::Qt3DSDMSlideHandle slideHandle = m_slideLookupHash.value(instanceHandle);
        for (int i = 0; i < m_slides.size(); ++i) {
            if (m_slides[i] == slideHandle) {
                setData(index(i, 0), GetBridge()->GetName(instanceHandle).toQString(),
                        SlideModel::NameRole);
                break;
            }
        }
    }
}

// Set selected slide highlight on UI
void SlideModel::setSelectedSlideIndex(const QModelIndex &index)
{
    if (m_selectedRow == index.row() ||
            !hasIndex(index.row(), index.column(), index.parent()))
        return;

    m_selectedRow = index.row();
    Q_EMIT dataChanged(this->index(0, 0), this->index(rowCount() - 1, 0), {SelectedRole});
}




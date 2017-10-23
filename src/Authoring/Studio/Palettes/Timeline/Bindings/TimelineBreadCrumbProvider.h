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

#ifndef INCLUDED_BREADCRUMBPROVIDER_H
#define INCLUDED_BREADCRUMBPROVIDER_H 1

#pragma once

#include "IBreadCrumbProvider.h"
#include "Qt3DSDMSignals.h"

// Link to data model
class CDoc;
class CTimelineBreadCrumbProvider;

//=============================================================================
/**
 * Bread crumb provider for displaying a trail of time contexts
 */
class CTimelineBreadCrumbProvider : public IBreadCrumbProvider
{
public:
    CTimelineBreadCrumbProvider(CDoc *inDoc);
    virtual ~CTimelineBreadCrumbProvider();

    TTrailList GetTrail(bool inRefresh = true) override;
    void OnBreadCrumbClicked(long inTrailIndex) override;

    QPixmap GetRootImage() const override;
    QPixmap GetBreadCrumbImage() const override;
    QPixmap GetSeparatorImage() const override;
    QPixmap GetActiveBreadCrumbImage() const override;

    void RefreshSlideList();
    void OnNameDirty();

protected:
    void ClearSlideList();
    void FillSlideList(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

protected:
    std::vector<qt3dsdm::Qt3DSDMInstanceHandle> m_BreadCrumbList;
    CDoc *m_Doc;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>>
        m_Connections; /// connections to the DataModel
};

#endif // INCLUDED_BREADCRUMBPROVIDER_H

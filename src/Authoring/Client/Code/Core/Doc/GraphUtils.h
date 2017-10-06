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

#ifndef INCLUDED_GRAPH_UTILS_H
#define INCLUDED_GRAPH_UTILS_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Graph.h"
#include "StudioObjectTypes.h"
#include "UICDMHandles.h"
#include "IGraphable.h"

//==============================================================================
//	Forwards
//==============================================================================
class CDoc;

typedef std::shared_ptr<Q3DStudio::CGraph> TAssetGraphPtr;
typedef Q3DStudio::CGraph TAssetGraph;

//==============================================================================
//	Filters
//==============================================================================
bool FilterForGraphableType(CDoc *inDoc, EGraphableType inGraphableType,
                            Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForGraphableType(CDoc *inDoc, EGraphableType inGraphableType);

bool FilterForAssetType(CDoc *inDoc, EStudioObjectType inObjectType,
                        Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForAssetType(CDoc *inDoc, EStudioObjectType inObjectType);

bool FilterAwayAssetType(CDoc *inDoc, EStudioObjectType inObjectType,
                         Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterAwayAssetType(CDoc *inDoc, EStudioObjectType inObjectType);

bool FilterForNodeType(CDoc *inDoc, Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForNodeType(CDoc *inDoc);

bool FilterForSlide(CDoc *inDoc, UICDM::CUICDMSlideHandle inSlide,
                    Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForSlide(CDoc *inDoc, UICDM::CUICDMSlideHandle inSlide);

bool FilterForControllingTimeParent(CDoc *inDoc,
                                    UICDM::CUICDMInstanceHandle inControllingTimeParent,
                                    Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter
FilterForControllingTimeParent(CDoc *inDoc, UICDM::CUICDMInstanceHandle inControllingTimeParent);

bool FilterForActiveComponent(CDoc *inDoc, UICDM::CUICDMSlideHandle inActiveComponentSlide,
                              Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForActiveComponent(CDoc *inDoc,
                                                  UICDM::CUICDMSlideHandle inActiveComponentSlide);

bool FilterForCurrentSlide(CDoc *inDoc, UICDM::CUICDMInstanceHandle inTimeParent,
                           Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForCurrentSlide(CDoc *inDoc,
                                               UICDM::CUICDMInstanceHandle inTimeParent);

//==============================================================================
//	Helper functions to get children from the graph
//==============================================================================
void GetAssetChildren(CDoc *inDoc, UICDM::CUICDMInstanceHandle inInstance,
                      Q3DStudio::CGraphIterator &outIterator,
                      EStudioObjectType inObjectType = OBJTYPE_UNKNOWN);
void GetAssetChildrenInSlide(CDoc *inDoc, UICDM::CUICDMInstanceHandle inInstance,
                             UICDM::CUICDMSlideHandle inSlide,
                             Q3DStudio::CGraphIterator &outIterator,
                             EStudioObjectType inObjectType = OBJTYPE_UNKNOWN);
void GetAssetChildrenInTimeParent(UICDM::CUICDMInstanceHandle inInstance, CDoc *inDoc,
                                  bool inIsAssetTimeParent, Q3DStudio::CGraphIterator &outIterator,
                                  UICDM::CUICDMSlideHandle inActiveComponentSlide = 0);
void GetAssetChildrenInCurrentSlide(CDoc *inDoc, UICDM::CUICDMInstanceHandle inInstance,
                                    Q3DStudio::CGraphIterator &outIterator,
                                    EStudioObjectType inObjectType = OBJTYPE_UNKNOWN);

//==============================================================================
//	Other useful functions related to graph traversal
//==============================================================================
bool IsAscendant(Q3DStudio::TIdentifier inDescendant, Q3DStudio::TIdentifier inAncestor,
                 Q3DStudio::TGraphPtr inGraph);

Q3DStudio::TIdentifier GetSibling(const Q3DStudio::TIdentifier inNode, bool inAfter,
                                  const Q3DStudio::CGraphIterator &inIterator);

void PrintAssetSubTree(UICDM::CUICDMInstanceHandle inInstance, CDoc *m_Doc, char *prefix);
void PrintSlideInfo(CDoc *m_Doc, char *prefix, UICDM::CUICDMSlideHandle theMasterSlide);

#endif // INCLUDED_GRAPH_UTILS_H

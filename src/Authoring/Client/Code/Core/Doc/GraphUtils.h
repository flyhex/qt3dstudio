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
#include "Qt3DSDMHandles.h"
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

bool FilterForSlide(CDoc *inDoc, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                    Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForSlide(CDoc *inDoc, qt3dsdm::Qt3DSDMSlideHandle inSlide);

bool FilterForControllingTimeParent(CDoc *inDoc,
                                    qt3dsdm::Qt3DSDMInstanceHandle inControllingTimeParent,
                                    Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter
FilterForControllingTimeParent(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inControllingTimeParent);

bool FilterForActiveComponent(CDoc *inDoc, qt3dsdm::Qt3DSDMSlideHandle inActiveComponentSlide,
                              Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForActiveComponent(CDoc *inDoc,
                                                  qt3dsdm::Qt3DSDMSlideHandle inActiveComponentSlide);

bool FilterForCurrentSlide(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inTimeParent,
                           Q3DStudio::TIdentifier inGraphable);
const Q3DStudio::TFilter FilterForCurrentSlide(CDoc *inDoc,
                                               qt3dsdm::Qt3DSDMInstanceHandle inTimeParent);

//==============================================================================
//	Helper functions to get children from the graph
//==============================================================================
void GetAssetChildren(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                      Q3DStudio::CGraphIterator &outIterator,
                      EStudioObjectType inObjectType = OBJTYPE_UNKNOWN);
void GetAssetChildrenInSlide(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                             qt3dsdm::Qt3DSDMSlideHandle inSlide,
                             Q3DStudio::CGraphIterator &outIterator,
                             EStudioObjectType inObjectType = OBJTYPE_UNKNOWN);
void GetAssetChildrenInTimeParent(qt3dsdm::Qt3DSDMInstanceHandle inInstance, CDoc *inDoc,
                                  bool inIsAssetTimeParent, Q3DStudio::CGraphIterator &outIterator,
                                  qt3dsdm::Qt3DSDMSlideHandle inActiveComponentSlide = 0);
void GetAssetChildrenInCurrentSlide(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                    Q3DStudio::CGraphIterator &outIterator,
                                    EStudioObjectType inObjectType = OBJTYPE_UNKNOWN);

//==============================================================================
//	Other useful functions related to graph traversal
//==============================================================================
bool IsAscendant(Q3DStudio::TIdentifier inDescendant, Q3DStudio::TIdentifier inAncestor,
                 Q3DStudio::TGraphPtr inGraph);

Q3DStudio::TIdentifier GetSibling(const Q3DStudio::TIdentifier inNode, bool inAfter,
                                  const Q3DStudio::CGraphIterator &inIterator);
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
void PrintAssetSubTree(qt3dsdm::Qt3DSDMInstanceHandle inInstance, CDoc *m_Doc, char *prefix);
void PrintSlideInfo(CDoc *m_Doc, char *prefix, qt3dsdm::Qt3DSDMSlideHandle theMasterSlide);
#endif

#endif // INCLUDED_GRAPH_UTILS_H

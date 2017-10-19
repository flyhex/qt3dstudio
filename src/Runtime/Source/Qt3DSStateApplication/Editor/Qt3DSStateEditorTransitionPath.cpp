/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Qt3DSStateEditorTransitionPath.h"

namespace {

using namespace uic::state::editor;
using namespace uic::state;

bool inBounds(QT3DSF32 item, QT3DSF32 lower, QT3DSF32 upper)
{
    if (item >= lower && item <= upper)
        return true;
    return false;
}

DirectionTypes::Enum EdgeTypeToDirectionType(EdgeTypes::Enum inEdgeType)
{
    switch (inEdgeType) {
    case EdgeTypes::Bottom:
    case EdgeTypes::Top:
        return DirectionTypes::Vertical;
    default:
        return DirectionTypes::Horizontal;
    }
}

static inline SEndPoint DoGetActualEndPoint(const SEndPoint &inPoint, const SRect &inMyRect,
                                            const QT3DSVec2 &inOtherCenter)
{
    SEndPoint theEndPoint = inPoint;
    if (inPoint.m_EdgeType == EdgeTypes::UnsetEdgeType)
        theEndPoint = CEditorTransitionPath::CalculateDefaultEndPoint(inMyRect, inOtherCenter);
    return theEndPoint;
}

static inline eastl::pair<QT3DSVec2, EdgeTypes::Enum>
GetEndpointPoint(const SEndPoint &inPoint, const SRect &inMyRect, const QT3DSVec2 &inOtherCenter)
{
    SEndPoint theEndPoint = DoGetActualEndPoint(inPoint, inMyRect, inOtherCenter);
    SLine theRectLine;
    switch (theEndPoint.m_EdgeType) {
    case EdgeTypes::Top:
        theRectLine = inMyRect.topEdge();
        break;
    case EdgeTypes::Bottom:
        theRectLine = inMyRect.bottomEdge();
        break;
    case EdgeTypes::Left:
        theRectLine = inMyRect.leftEdge();
        break;
    default:
        QT3DS_ASSERT(false);
    // fallthrough intentional
    case EdgeTypes::Right:
        theRectLine = inMyRect.rightEdge();
        break;
    }

    return eastl::make_pair(theRectLine.toPoint(theEndPoint.m_Interp), theEndPoint.m_EdgeType);
}

inline bool RectDiffers(const Option<SRect> &inLhs, const SRect &inRhs)
{
    if (inLhs.isEmpty())
        return true;
    SRect lhs = *inLhs;
    return CEditorTransitionPath::AreAlmostEqual(lhs.m_TopLeft, inRhs.m_TopLeft) == false
        || CEditorTransitionPath::AreAlmostEqual(lhs.m_WidthHeight, inRhs.m_WidthHeight) == false;
}
}

namespace uic {
namespace state {
    namespace editor {

        bool SRect::contains(const QT3DSVec2 &inPoint) const
        {
            if (inPoint.x >= left() && inPoint.x <= right() && inPoint.y >= top()
                && inPoint.y <= bottom())
                return true;
            return false;
        }

        void CEditorTransitionPath::SetPathType(TransitionPathTypes::Enum inType)
        {
            m_TransitionPathType = inType;
            if (m_TransitionPathType == TransitionPathTypes::BeginToBegin
                || m_TransitionPathType == TransitionPathTypes::Targetless)
                m_ControlPoints.clear();
            MarkDirty();
        }

        bool CEditorTransitionPath::UpdateBeginEndRects(const SRect &inStartRect,
                                                        const SRect &inEndRect)
        {
            bool dataChanged = false;

            if (m_BeginRect.hasValue() && m_EndRect.hasValue()) {
                QT3DSVec2 beginDiff = inStartRect.center() - m_BeginRect->center();
                QT3DSVec2 endDiff = inEndRect.center() - m_EndRect->center();

                if (AreAlmostEqual(beginDiff, QT3DSVec2(0, 0)) == false
                    && AreAlmostEqual(beginDiff, endDiff, 1)) {
                    dataChanged = m_ControlPoints.empty() == false;
                    // Move all the control points.
                    for (size_t idx = 0, end = m_ControlPoints.size(); idx < end; ++idx) {
                        SControlPoint &thePoint(m_ControlPoints[idx]);
                        QT3DSF32 diffComponent =
                            SControlPoint::GetComponent(beginDiff, thePoint.m_Direction);
                        thePoint.m_Position += diffComponent;
                    }
                }
            }
            bool rectDiffers =
                RectDiffers(m_BeginRect, inStartRect) || RectDiffers(m_EndRect, inEndRect);
            m_BeginRect = inStartRect;
            m_EndRect = inEndRect;
            if (rectDiffers)
                MarkDirty();
            m_TransitionPathType = TransitionPathTypes::BeginToEnd;
            return dataChanged;
        }

        bool CEditorTransitionPath::UpdateBeginEndRects(const SRect &inStartRect,
                                                        TransitionPathTypes::Enum inPathType)
        {
            bool dataChanged = m_ControlPoints.empty() == false;
            QT3DS_ASSERT(inPathType == TransitionPathTypes::BeginToBegin
                      || inPathType == TransitionPathTypes::Targetless);
            bool rectDiffers = m_EndRect.hasValue() || RectDiffers(m_BeginRect, inStartRect);
            m_BeginRect = inStartRect;
            m_EndRect = Empty();
            if (rectDiffers || dataChanged)
                MarkDirty();
            m_TransitionPathType = inPathType;
            return dataChanged;
        }

        eastl::pair<QT3DSVec2, EdgeTypes::Enum> CEditorTransitionPath::GetBeginPointAndEdge() const
        {
            eastl::pair<QT3DSVec2, EdgeTypes::Enum> theStartPoint(QT3DSVec2(0, 0),
                                                               EdgeTypes::UnsetEdgeType);
            if (m_BeginRect.hasValue()) {
                QT3DSVec2 center(m_BeginRect->center());
                center.x += 1;
                if (m_EndRect.hasValue())
                    center = m_EndRect->center();
                theStartPoint = GetEndpointPoint(m_Begin, *m_BeginRect, center);
            }
            return theStartPoint;
        }

        eastl::pair<QT3DSVec2, QT3DSVec2> CEditorTransitionPath::GetBeginEndPoints() const
        {
            eastl::pair<QT3DSVec2, EdgeTypes::Enum> theStartPoint(GetBeginPointAndEdge());
            eastl::pair<QT3DSVec2, EdgeTypes::Enum> theEndPoint(QT3DSVec2(0, 0),
                                                             EdgeTypes::UnsetEdgeType);
            if (m_EndRect.hasValue())
                theEndPoint = GetEndpointPoint(m_End, *m_EndRect, m_BeginRect->center());

            return eastl::make_pair(theStartPoint.first, theEndPoint.first);
        }

        SEndPoint CEditorTransitionPath::CalculateEndPoint(const SRect &inRect,
                                                           const QT3DSVec2 &inPoint,
                                                           QT3DSF32 inEdgeBoundary)
        {
            if (inRect.width() == 0 || inRect.height() == 0) {
                QT3DS_ASSERT(false);
                return SEndPoint();
            }

            SLine centerToPoint = SLine(inRect.center(), inPoint);
            SLine leftOrRight;
            SLine topOrBottom;
            Option<QT3DSF32> isect;
            EdgeTypes::Enum theEdge = EdgeTypes::UnsetEdgeType;
            // If line runs right, test against right edge
            QT3DSF32 distance = 0;
            SLine theRectLine;
            if (centerToPoint.dx() > 0) {
                theRectLine = inRect.rightEdge();
                isect = theRectLine.intersect(centerToPoint);
                // If we are out of range for the right edge
                if (isect.hasValue() && inBounds(*isect, 0.0f, 1.0f)) {
                    distance = theRectLine.dy();
                    theEdge = EdgeTypes::Right;
                }
            } else {
                theRectLine = inRect.leftEdge();
                isect = theRectLine.intersect(centerToPoint);
                if (isect.hasValue() && inBounds(*isect, 0.0f, 1.0f)) {
                    distance = theRectLine.dy();
                    theEdge = EdgeTypes::Left;
                }
            }
            // If we haven't resolved the edge type
            if (theEdge == EdgeTypes::UnsetEdgeType) {
                if (centerToPoint.dy() < 0) {
                    theRectLine = inRect.topEdge();
                    isect = theRectLine.intersect(centerToPoint);
                    theEdge = EdgeTypes::Top;
                    distance = theRectLine.dx();
                } else {
                    theRectLine = inRect.bottomEdge();
                    isect = theRectLine.intersect(centerToPoint);
                    theEdge = EdgeTypes::Bottom;
                    distance = theRectLine.dx();
                }
            }
            // Now drop a perpendicular from the point to the rect line.
            SLine normalLine(inPoint, inPoint + QT3DSVec2(theRectLine.dy(), -theRectLine.dx()));
            isect = theRectLine.intersect(normalLine);
            if (isect.isEmpty()) {
                theEdge = EdgeTypes::Right;
                isect = .5f;
            }
            SEndPoint retval(theEdge, *isect);
            QT3DSF32 normalizedBoundary =
                NVMax(0.0f, inEdgeBoundary / static_cast<float>(fabs(distance)));

            QT3DSF32 edgeLowerBound = NVMax(0.0f, normalizedBoundary);
            QT3DSF32 edgeUpperBound = NVMin(1.0f, 1.0f - normalizedBoundary);
            retval.m_Interp = NVMax(edgeLowerBound, retval.m_Interp);
            retval.m_Interp = NVMin(edgeUpperBound, retval.m_Interp);
            return retval;
        }

        // Default end points always are in the middle of the rect edge
        SEndPoint CEditorTransitionPath::CalculateDefaultEndPoint(const SRect &inRect,
                                                                  const QT3DSVec2 &inPoint)
        {
            SEndPoint ep = CalculateEndPoint(inRect, inPoint, 0.0f);
            return SEndPoint(ep.m_EdgeType, .5f);
        }

        void CEditorTransitionPath::SetEndPoint(const QT3DSVec2 &inWorldPoint)
        {
            QT3DS_ASSERT(m_EndRect.hasValue());
            m_End = CalculateEndPoint(*m_EndRect, inWorldPoint, m_StateEdgeBuffer);
            MarkDirty();
        }

        void CEditorTransitionPath::SetBeginPoint(const QT3DSVec2 &inWorldPoint)
        {
            QT3DS_ASSERT(m_BeginRect.hasValue());
            m_Begin = CalculateEndPoint(*m_BeginRect, inWorldPoint, m_StateEdgeBuffer);
            MarkDirty();
        }

        SEndPoint CEditorTransitionPath::GetActualBeginPoint() const
        {
            return DoGetActualEndPoint(m_Begin, *m_BeginRect, m_EndRect->center());
        }

        SEndPoint CEditorTransitionPath::GetActualEndPoint() const
        {
            return DoGetActualEndPoint(m_End, *m_EndRect, m_BeginRect->center());
        }

        void CEditorTransitionPath::SetControlPoint(QT3DSI32 idx, const QT3DSVec2 &inPosition,
                                                    bool inMoveAdjacentEndPoint)
        {
            if (idx < 0 || idx >= (QT3DSI32)m_ControlPoints.size()) {
                QT3DS_ASSERT(false);
                return;
            }
            m_ControlPoints[idx].Set(inPosition);
            if (inMoveAdjacentEndPoint) {
                // Move the end points adjacent to the handle.
                QT3DSI32 possiblePoint = MapControlPointToPossibleControlPoint(idx);
                size_t numPossible = m_PossibleControlPoints.size();
                if (possiblePoint == 0)
                    SetBeginPoint(inPosition);
                if (possiblePoint == ((QT3DSI32)numPossible - 1))
                    SetEndPoint(inPosition);
            }

            MarkDirty();
        }

        void CEditorTransitionPath::SetControlPoints(const TPointList &inList)
        {
            QT3DS_ASSERT(inList.size() == m_Path.size());
            for (size_t idx = 0, end = m_ControlPoints.size(); idx < end; ++idx) {
                SControlPoint &theControlPoint(m_ControlPoints[idx]);
                if (theControlPoint.m_PathIndex >= 0
                    && theControlPoint.m_PathIndex < (QT3DSI32)inList.size())
                    theControlPoint.Set(inList[theControlPoint.m_PathIndex]);
                else {
                    QT3DS_ASSERT(false);
                }
            }
        }

        TPointList CEditorTransitionPath::GetPath() const
        {
            MaybeRegeneratePath();
            return m_Path;
        }

        TControlPointList CEditorTransitionPath::GetPossibleControlPoints() const
        {
            MaybeRegeneratePath();
            return m_PossibleControlPoints;
        }

        SControlPoint CEditorTransitionPath::GetPossibleControlPoint(QT3DSI32 inIdx) const
        {
            MaybeRegeneratePath();
            if (inIdx >= 0 && inIdx < (QT3DSI32)m_PossibleControlPoints.size())
                return m_PossibleControlPoints[inIdx];
            QT3DS_ASSERT(false);
            return SControlPoint();
        }

        // This may create a new control point thus invalidating the path and possible control
        // points.
        QT3DSI32
        CEditorTransitionPath::MapPossibleControlPointToControlPoint(QT3DSI32 inPossiblePointIndex)
        {
            if (inPossiblePointIndex < 0
                || inPossiblePointIndex >= (QT3DSI32)m_PossibleControlPoints.size()) {
                QT3DS_ASSERT(false);
                return -1;
            }
            const SControlPoint &thePossiblePoint(m_PossibleControlPoints[inPossiblePointIndex]);
            TControlPointList::iterator iter = eastl::lower_bound(
                m_ControlPoints.begin(), m_ControlPoints.end(), thePossiblePoint);
            QT3DSI32 retval = (QT3DSI32)m_ControlPoints.size();
            if (iter != m_ControlPoints.end()) {
                retval = (QT3DSI32)(iter - m_ControlPoints.begin());
                if (iter->m_PathIndex == thePossiblePoint.m_PathIndex)
                    return retval;
            }

            m_ControlPoints.insert(iter, thePossiblePoint);
            return retval;
        }

        QT3DSI32 CEditorTransitionPath::MapControlPointToPossibleControlPoint(QT3DSI32 inPointIndex)
        {
            if (inPointIndex < 0 || inPointIndex >= (QT3DSI32)m_ControlPoints.size()) {
                QT3DS_ASSERT(false);
                return -1;
            }
            const SControlPoint &theControlPoint(m_ControlPoints[inPointIndex]);
            TControlPointList::iterator iter = eastl::lower_bound(
                m_PossibleControlPoints.begin(), m_PossibleControlPoints.end(), theControlPoint);
            if (iter != m_PossibleControlPoints.end()) {
                QT3DSI32 retval = (QT3DSI32)(iter - m_PossibleControlPoints.begin());
                if (iter->m_PathIndex == theControlPoint.m_PathIndex)
                    return retval;
            }
            QT3DS_ASSERT(false);
            return -1;
        }

        bool CEditorTransitionPath::DoesControlPointExist(QT3DSI32 inPossiblePointIndex) const
        {
            if (inPossiblePointIndex < 0
                || inPossiblePointIndex >= (QT3DSI32)m_PossibleControlPoints.size()) {
                QT3DS_ASSERT(false);
                return false;
            }
            const SControlPoint &thePossiblePoint(m_PossibleControlPoints[inPossiblePointIndex]);
            TControlPointList::const_iterator iter = eastl::lower_bound(
                m_ControlPoints.begin(), m_ControlPoints.end(), thePossiblePoint);
            if (iter != m_ControlPoints.end() && iter->m_PathIndex == thePossiblePoint.m_PathIndex)
                return true;
            return false;
        }

        // Run through the control point list and if you find another control point on the line,
        // return it's index.  Else
        // bail.
        // If you are finding the remove algorithm is too specific or hard to use increase the 2.0f
        // numbers below.
        inline Option<size_t> NextParallelControlPointOnLine(const SControlPoint &inItem,
                                                             const SControlPoint &inEndPoint,
                                                             const TControlPointList &inList,
                                                             size_t inStartIdx)
        {
            for (size_t idx = inStartIdx, end = inList.size(); idx < end; ++idx) {
                const SControlPoint &theTestPoint(inList[idx]);
                if (theTestPoint.m_Direction == inItem.m_Direction) {
                    if (CEditorTransitionPath::AreAlmostEqual(inItem.m_Position,
                                                              theTestPoint.m_Position, 2.0f))
                        return idx + 1;
                    else
                        return Empty();
                }
            }

            // Check if beginning and end lie in the same path, or within just a couple pixels.
            if (inItem.m_Direction == inEndPoint.m_Direction
                && CEditorTransitionPath::AreAlmostEqual(inItem.m_Position, inEndPoint.m_Position,
                                                         2.0f))
                return inList.size();

            return Empty();
        }

        // We try to find control point that point the same direction and lie on the same line with
        // only control points with
        // orthogonal directions in between.  If we find points that fullfill this criteria, we know
        // we can remove all intermediate
        // points because the transition path will end up making a straight line.
        bool CEditorTransitionPath::RemoveRedundantControlPoints()
        {
            if (m_ControlPoints.empty())
                return false;

            eastl::pair<QT3DSVec2, EdgeTypes::Enum> theStartPoint = GetBeginPointAndEdge();
            eastl::pair<QT3DSVec2, EdgeTypes::Enum> theEndPoint;
            if (m_EndRect.hasValue())
                theEndPoint = GetEndpointPoint(m_End, *m_EndRect, m_BeginRect->center());
            else
                theEndPoint = theStartPoint;
            // Find runs of control points in the same line.  Remove the points in the middle of the
            // line.
            SControlPoint theLastControlPoint(EdgeTypeToDirectionType(theStartPoint.second));
            theLastControlPoint.Set(theStartPoint.first);
            SControlPoint theEndControlPoint(EdgeTypeToDirectionType(theEndPoint.second));
            theEndControlPoint.Set(theEndPoint.first);
            size_t numControlPoints(m_ControlPoints.size());
            for (size_t idx = 0, end = numControlPoints; idx < end; ++idx) {
                Option<size_t> removeEnd = NextParallelControlPointOnLine(
                    theLastControlPoint, theEndControlPoint, m_ControlPoints, idx);
                if (removeEnd.isEmpty() == false) {
                    size_t lastItem = *removeEnd;
                    m_ControlPoints.erase(m_ControlPoints.begin() + idx,
                                          m_ControlPoints.begin() + lastItem);
                    --idx;
                    end = m_ControlPoints.size();
                } else
                    theLastControlPoint = m_ControlPoints[idx];
            }
            if (m_ControlPoints.size() != numControlPoints) {
                MarkDirty();
                return true;
            }
            return false;
        }

        void CEditorTransitionPath::RestoreAutoTransition()
        {
            m_ControlPoints.clear();
            m_Begin = SEndPoint();
            m_End = SEndPoint();
            MarkDirty();
        }

        bool CEditorTransitionPath::IsManualMode() const
        {
            return m_Begin.m_EdgeType != EdgeTypes::UnsetEdgeType
                || m_End.m_EdgeType != EdgeTypes::UnsetEdgeType || m_ControlPoints.empty() == false;
        }

        SPointQueryResult CEditorTransitionPath::Pick(QT3DSVec2 inPoint, QT3DSVec2 inControlBoxDims,
                                                      QT3DSVec2 inEndBoxDims)
        {
            MaybeRegeneratePath();

            if (inEndBoxDims.x && inEndBoxDims.y) {
                SRect endBox(QT3DSVec2(-1.0f * inEndBoxDims.x / 2, -1.0f * inEndBoxDims.y / 2),
                             inEndBoxDims);
                SRect testRect(endBox);
                testRect.translate(m_Path.front());
                if (testRect.contains(inPoint))
                    return SPointQueryResult(PointQueryResultType::Begin);
                testRect = SRect(endBox);
                testRect.translate(m_Path.back());
                if (testRect.contains(inPoint))
                    return SPointQueryResult(PointQueryResultType::End);
            }
            if (inControlBoxDims.x && inControlBoxDims.y) {
                SRect theControlBox(
                    QT3DSVec2(-1.0f * inControlBoxDims.x / 2.0f, -1.0f * inControlBoxDims.y / 2.0f),
                    inControlBoxDims);
                for (size_t idx = 0, end = m_PossibleControlPoints.size(); idx < end; ++idx) {
                    const SControlPoint &thePoint(m_PossibleControlPoints[idx]);
                    QT3DSVec2 startPoint = m_Path[thePoint.m_PathIndex];
                    QT3DSVec2 endPoint = m_Path[thePoint.m_PathIndex + 1];
                    // We stretch the rect to contain the entire line, not just where we display the
                    // point.
                    QT3DSVec2 lineDims = endPoint - startPoint;
                    QT3DSF32 lineRectHeight = SControlPoint::GetComponent(theControlBox.m_WidthHeight,
                                                                       thePoint.m_Direction);
                    QT3DSF32 lineRectLength =
                        fabs(SControlPoint::GetOrthogonalComponent(lineDims, thePoint.m_Direction));
                    QT3DSVec2 rectDims = SControlPoint::FromComponentToVector(
                        lineRectHeight, lineRectLength, thePoint.m_Direction);
                    QT3DSVec2 rectTopLeft =
                        QT3DSVec2(NVMin(startPoint.x, endPoint.x), NVMin(startPoint.y, endPoint.y));
                    QT3DSF32 rectComponent =
                        SControlPoint::GetComponent(rectTopLeft, thePoint.m_Direction);
                    rectComponent -= lineRectHeight / 2.0f; // Center the box about the line.
                    rectTopLeft = SControlPoint::SetComponent(rectTopLeft, rectComponent,
                                                              thePoint.m_Direction);
                    SRect testRect(rectTopLeft, rectDims);
                    if (testRect.contains(inPoint))
                        return SPointQueryResult(PointQueryResultType::Control, (QT3DSI32)idx);
                }
            }
            return PointQueryResultType::NoPoint;
        }

        SPointQueryResult
        CEditorTransitionPath::PickClosestControlPoint(QT3DSVec2 inPoint,
                                                       DirectionTypes::Enum inDirectionType)
        {
            MaybeRegeneratePath();
            QT3DSI32 closestIdx = -1;
            QT3DSF32 minDistance = QT3DS_MAX_F32;

            for (size_t idx = 0, end = m_PossibleControlPoints.size(); idx < end; ++idx) {
                const SControlPoint &thePoint(m_PossibleControlPoints[idx]);
                QT3DSVec2 startPoint = m_Path[thePoint.m_PathIndex];
                QT3DSVec2 endPoint = m_Path[thePoint.m_PathIndex + 1];
                SLine theLine(startPoint, endPoint);
                QT3DSF32 distance = theLine.distance(inPoint);

                if (distance < minDistance && thePoint.m_Direction == inDirectionType) {
                    closestIdx = idx;
                    minDistance = distance;
                }
            }
            if (closestIdx == -1)
                return SPointQueryResult();
            return SPointQueryResult(PointQueryResultType::Control, closestIdx);
        }

        // The output functions are both setup under these assumptions:
        // 1. The current point does *not* have representation in the possible control points list.
        // 2. The last point *does* have representation in the possible control points list.
        // 3. The algorithm should output all path elements up and to but not including the
        //    current point.
        // So, given straight line do not output any possible points.
        // Given zig-zag, output two points.
        SControlPoint
        CEditorTransitionPath::OutputParallelPoints(const SControlPoint &inLastPoint,
                                                    const SControlPoint &inCurrentPoint,
                                                    QT3DSF32 runWidth) const
        {
            const SControlPoint &theRunPoint(inCurrentPoint);
            const SControlPoint theLastControlPoint(inLastPoint);
            DirectionTypes::Enum runDirection(inLastPoint.m_Direction);
            if (AreAlmostEqual(theRunPoint.m_Position, theLastControlPoint.m_Position, 1.0)) {
                // Straigh line.  Perhaps remove this point?
                theRunPoint.m_PathIndex = theLastControlPoint.m_PathIndex;
                return theRunPoint;
            } else {
                // First, output a possible control point inline with inLastPoint
                SControlPoint possiblePoint(inLastPoint);
                possiblePoint.m_PathIndex = (QT3DSI32)(m_Path.size() - 1);
                m_PossibleControlPoints.push_back(possiblePoint);
                // Output zig-zag, we zig zag from last control point to theRunPoint.  We need to
                // push two points and two control points.
                QT3DSVec2 startPos(m_Path.back());
                QT3DSF32 startComponent = SControlPoint::GetComponent(startPos, runDirection);
                QT3DSF32 orthoStartComponent =
                    SControlPoint::GetOrthogonalComponent(startPos, runDirection);
                QT3DSF32 endComponent = theRunPoint.m_Position;
                QT3DSF32 orthoEndComponent = orthoStartComponent + runWidth;
                QT3DSVec2 endPos = SControlPoint::FromComponentToVector(
                    endComponent, orthoEndComponent, runDirection);
                QT3DSF32 zigZagOrthoPos = orthoStartComponent + runWidth / 2;
                QT3DSI32 crossbarIndex = (QT3DSI32)m_Path.size();
                QT3DSVec2 crossbarStart = SControlPoint::FromComponentToVector(
                    startComponent, zigZagOrthoPos, runDirection);
                m_Path.push_back(crossbarStart);
                m_PossibleControlPoints.push_back(
                    SControlPoint(SControlPoint::OppositeDirection(theRunPoint.m_Direction),
                                  orthoStartComponent, crossbarIndex));
                QT3DSVec2 crossbarEnd = SControlPoint::FromComponentToVector(
                    endComponent, zigZagOrthoPos, theRunPoint.m_Direction);
                m_Path.push_back(crossbarEnd);
                theRunPoint.m_PathIndex = crossbarIndex + 1;
                // Do not, however, output the run point.  This will happen in the next step.
                return inCurrentPoint;
            }
        }

        // Given right angle, output 1 point.
        SControlPoint
        CEditorTransitionPath::OutputOrthogonalPoints(const SControlPoint &inLastPoint,
                                                      const SControlPoint &inCurrentPoint) const
        {
            SLine lastLine = inLastPoint.ToLine();
            SLine currentLine = inCurrentPoint.ToLine();
            Option<QT3DSF32> isect = lastLine.intersect(currentLine);
            QT3DS_ASSERT(isect.hasValue());
            inLastPoint.m_PathIndex = (QT3DSI32)(m_Path.size() - 1);
            m_PossibleControlPoints.push_back(inLastPoint);
            if (isect.hasValue()) {
                QT3DSVec2 theIsectPoint = lastLine.toPoint(*isect);
                m_Path.push_back(theIsectPoint);
            }
            inCurrentPoint.m_PathIndex = (QT3DSI32)(m_Path.size() - 1);
            return inCurrentPoint;
        }

        void CEditorTransitionPath::MaybeRegeneratePath() const
        {
            if (IsDirty() == false)
                return;

            if (m_TransitionPathType == TransitionPathTypes::BeginToEnd) {
                // Ensure intermediate information is cleared.
                const_cast<CEditorTransitionPath *>(this)->MarkDirty();
                // We don't have the begin and end states.
                if (m_BeginRect.isEmpty() || m_EndRect.isEmpty()) {
                    QT3DS_ASSERT(false);
                    return;
                }
                // Find the start and end points.
                eastl::pair<QT3DSVec2, EdgeTypes::Enum> theStartPoint =
                    GetEndpointPoint(m_Begin, *m_BeginRect, m_EndRect->center());
                eastl::pair<QT3DSVec2, EdgeTypes::Enum> theEndPoint =
                    GetEndpointPoint(m_End, *m_EndRect, m_BeginRect->center());
                m_Path.push_back(theStartPoint.first);
                SControlPoint theLastControlPoint(EdgeTypeToDirectionType(theStartPoint.second));
                theLastControlPoint.Set(theStartPoint.first);
                theLastControlPoint.m_PathIndex = 0;
                SControlPoint theEndControlPoint(EdgeTypeToDirectionType(theEndPoint.second));
                theEndControlPoint.Set(theEndPoint.first);
                for (size_t idx = 0, end = m_ControlPoints.size(); idx < end; ++idx) {
                    const SControlPoint &thePoint(m_ControlPoints[idx]);
                    if (thePoint.m_Direction == theLastControlPoint.m_Direction) {
                        // zig zag.  Requires us to find the first point that *isn't a zig zag in
                        // order to
                        // calculate where the zig zag should be.  We could have a section composed
                        // of only
                        // parallel directions and we can't lay then out until we find how much
                        // distance we have
                        // to space each on out.
                        // Image you get to this point and you find you have a set of control points
                        // with vertical direction.
                        // Their positions will tell us how far left each one should sit.  But we
                        // don't have enough information
                        // to lay them out without knowing how much vertical space this section
                        // should fill.  So we would have to
                        // search forward until we can figure this out.
                        QT3DSVec2 runStart = m_Path.back();
                        // Search forward until either we run out of points or until we find a point
                        // who's direction
                        // does not match the current direction.  We call a contiguous set of
                        // control points who all
                        // have the same direction a 'run'.
                        size_t runEnd;
                        size_t zigzagCount = 1;
                        DirectionTypes::Enum runDirection = theLastControlPoint.m_Direction;
                        // Search forward till we find a point that is different.
                        for (runEnd = idx + 1; runEnd < end
                             && m_ControlPoints[runEnd].m_Direction == thePoint.m_Direction;
                             ++runEnd) {
                            // Skip items that are in line.  They shouldn't be counted towards our
                            // zigzag count.
                            if (AreAlmostEqual(m_ControlPoints[runEnd].m_Position,
                                               m_ControlPoints[runEnd - 1].m_Position)
                                == false)
                                ++zigzagCount;
                        }
                        // Two possible cases.  Either we find a control point that has a different
                        // direction in which case we then figure out
                        // how much space we need overall *or* we ran out of control points in which
                        // case we use the end point.
                        QT3DSVec2 runEndPoint(0, 0);
                        if (runEnd == end) {
                            // check if the end point direction is the same.  This could be the
                            // final zig zag.  Else it will be a righthand turn
                            if (EdgeTypeToDirectionType(theEndPoint.second) == runDirection
                                && AreAlmostEqual(theEndControlPoint.m_Position,
                                                  thePoint.m_Position)
                                    == false)
                                ++zigzagCount;

                            runEndPoint = theEndPoint.first;
                        } else {
                            SLine thePointLine(thePoint.ToLine());
                            Option<QT3DSF32> isect =
                                thePointLine.intersect(m_ControlPoints[runEnd].ToLine());
                            if (isect.hasValue())
                                runEndPoint = thePointLine.toPoint(*isect);
                            else {
                                QT3DS_ASSERT(false);
                            }
                        }
                        QT3DSF32 runOrthoStart =
                            SControlPoint::GetOrthogonalComponent(runStart, runDirection);
                        QT3DSF32 runOrthoEnd =
                            SControlPoint::GetOrthogonalComponent(runEndPoint, runDirection);
                        QT3DSF32 runRange = runOrthoEnd - runOrthoStart;
                        QT3DSF32 runWidth = runRange / (QT3DSF32)zigzagCount;
                        // Now we iterate through the run itself and output path elements.
                        for (; idx < runEnd; ++idx) {
                            theLastControlPoint = OutputParallelPoints(
                                theLastControlPoint, m_ControlPoints[idx], runWidth);
                        }
                        // Subtract one to account for the loop upate that happens next
                        --idx;
                    } else // right angle
                    {
                        theLastControlPoint =
                            OutputOrthogonalPoints(theLastControlPoint, m_ControlPoints[idx]);
                    }
                }
                // Finished iterating through the control points.  Now we have the sticky situation
                // of the very last point
                // and how it joins with the end point.
                QT3DSVec2 lastPoint(m_Path.back());
                if (theEndControlPoint.m_Direction == theLastControlPoint.m_Direction) {
                    QT3DSF32 lastPointOrthoComponent = SControlPoint::GetOrthogonalComponent(
                        lastPoint, theLastControlPoint.m_Direction);
                    QT3DSF32 endOrthoComponent = SControlPoint::GetOrthogonalComponent(
                        theEndPoint.first, theLastControlPoint.m_Direction);
                    QT3DSF32 runWidth = endOrthoComponent - lastPointOrthoComponent;
                    OutputParallelPoints(theLastControlPoint, theEndControlPoint, runWidth);
                } else {
                    theLastControlPoint =
                        OutputOrthogonalPoints(theLastControlPoint, theEndControlPoint);
                }
                // Now output the last possible point which matches the end control point's
                // direction and such.
                theEndControlPoint.m_PathIndex = (QT3DSI32)(m_Path.size() - 1);
#ifdef _DEBUG
                // The directly previous possible point in the list should not match this point.  In
                // fact, it really should be orthogonal.
                if (m_PossibleControlPoints.size()) {
                    QT3DS_ASSERT(m_PossibleControlPoints.back().m_Direction
                                  != theEndControlPoint.m_Direction
                              || AreAlmostEqual(m_PossibleControlPoints.back().m_Position,
                                                theEndControlPoint.m_Position)
                                  == false);
                }
#endif
                m_PossibleControlPoints.push_back(theEndControlPoint);
                // Finally push back the last item.
                m_Path.push_back(theEndPoint.first);
            } else if (m_TransitionPathType == TransitionPathTypes::BeginToBegin
                       || m_TransitionPathType == TransitionPathTypes::Targetless) {
                QT3DSVec2 beginCenter(m_BeginRect->center());
                beginCenter.x += 1;
                eastl::pair<QT3DSVec2, EdgeTypes::Enum> theStartPoint =
                    GetEndpointPoint(m_Begin, *m_BeginRect, beginCenter);
                QT3DSVec2 lineDir;
                m_Path.push_back(theStartPoint.first);
                if (m_TransitionPathType == TransitionPathTypes::BeginToBegin) {
                    switch (theStartPoint.second) {
                    case EdgeTypes::Top:
                        lineDir = QT3DSVec2(0, -1);
                        break;
                    case EdgeTypes::Bottom:
                        lineDir = QT3DSVec2(0, 1);
                        break;
                    case EdgeTypes::Left:
                        lineDir = QT3DSVec2(-1, 0);
                        break;
                    case EdgeTypes::Right:
                        lineDir = QT3DSVec2(1, 0);
                        break;
                    default:
                        QT3DS_ASSERT(false);
                        break;
                    }
                    QT3DSF32 squareDiagLen = 30;
                    QT3DSF32 halfDiag = squareDiagLen / 2.0f;
                    QT3DSVec2 theOppPoint = theStartPoint.first + lineDir * squareDiagLen;
                    QT3DSVec2 middle = (theStartPoint.first + theOppPoint) / 2.0f;
                    QT3DSVec2 orthoLineDir = QT3DSVec2(lineDir.y, lineDir.x);
                    QT3DSVec2 loopTop = middle + orthoLineDir * halfDiag;
                    QT3DSVec2 loopBottom = middle - orthoLineDir * halfDiag;
                    m_Path.push_back(loopTop);
                    m_Path.push_back(theOppPoint);
                    m_Path.push_back(loopBottom);
                    m_Path.push_back(theStartPoint.first);
                }
            } else {
                QT3DS_ASSERT(false);
            }
        }
    }
}
}
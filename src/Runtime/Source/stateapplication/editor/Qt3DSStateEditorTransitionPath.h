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
#ifndef QT3DS_STATE_EDITOR_TRANSITION_PATH_H
#define QT3DS_STATE_EDITOR_TRANSITION_PATH_H
#pragma once
#include "Qt3DSState.h"
#include "foundation/Qt3DSVec2.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSOption.h"

namespace qt3ds {
namespace state {
    namespace editor {

        struct EdgeTypes
        {
            enum Enum {
                UnsetEdgeType = 0,
                Left,
                Right,
                Top,
                Bottom,
            };
        };

        struct SEndPoint
        {
            // Which edge are we on
            EdgeTypes::Enum m_EdgeType;
            // How far from the start of the edge are we.
            QT3DSF32 m_Interp;
            SEndPoint(EdgeTypes::Enum inType = EdgeTypes::UnsetEdgeType, QT3DSF32 interp = .5f)
                : m_EdgeType(inType)
                , m_Interp(interp)
            {
            }
        };

        struct DirectionTypes
        {
            enum Enum {
                UnknownDirection = 0,
                Horizontal,
                Vertical,
            };
        };

        struct SLine
        {
            QT3DSVec2 m_Begin;
            QT3DSVec2 m_End;

            SLine(const QT3DSVec2 &beg = QT3DSVec2(0, 0), const QT3DSVec2 &end = QT3DSVec2(0, 0))
                : m_Begin(beg)
                , m_End(end)
            {
            }

            QT3DSF32 dx() const { return m_End.x - m_Begin.x; }
            QT3DSF32 dy() const { return m_End.y - m_Begin.y; }
            QT3DSVec2 begin() const { return m_Begin; }
            QT3DSVec2 end() const { return m_End; }
            QT3DSVec2 toPoint(QT3DSF32 interp) { return m_Begin + QT3DSVec2(dx() * interp, dy() * interp); }
            Option<QT3DSF32> slope() const
            {
                QT3DSF32 run = dx();
                if (fabs(run) > .0001f)
                    return dy() / dx();
                return Empty();
            }
            // If this function returns a value, it returns an interpolation value such that if you
            // call toPoint on the return value it gives you the intersection point.
            // http://tog.acm.org/resources/GraphicsGems/gemsiii/insectc.c
            // Simplifications taken from Qt's line intersect
            Option<QT3DSF32> intersect(const SLine &other) const
            {
                // ipmlementation is based on Graphics Gems III's "Faster Line Segment Intersection"
                QT3DSVec2 a = m_End - m_Begin;
                QT3DSVec2 b = other.m_End - other.m_Begin;
                QT3DSVec2 c = m_Begin - other.m_Begin;

                QT3DSF32 denominator = a.y * b.x - a.x * b.y;

                if (denominator == 0 || fabs(denominator) < .0001f)
                    return Empty();

                QT3DSF32 reciprocal = 1.0f / denominator;
                return (b.y * c.x - b.x * c.y) * reciprocal;
            }

            // Caculates the top half of the distance to line equation with the fabs or sqrt.
            QT3DSF32 distanceNumerator(const QT3DSVec2 &inPoint) const
            {
                QT3DSF32 x2 = inPoint.x;
                QT3DSF32 y2 = inPoint.y;

                QT3DSF32 x1 = m_End.x;
                QT3DSF32 y1 = m_End.y;

                QT3DSF32 x0 = m_Begin.x;
                QT3DSF32 y0 = m_Begin.y;
                return ((x2 - x1) * (y1 - y0)) - ((x1 - x0) * (y2 - y1));
            }

            QT3DSF32 distance(const QT3DSVec2 &inPoint) const
            {
                QT3DSF32 theDx = dx();
                QT3DSF32 theDy = dy();
                return fabs(distanceNumerator(inPoint)) / sqrtf(theDx * theDx + theDy * theDy);
            }
        };

        // Rect in same coordinate space as QT.
        struct SRect
        {
            QT3DSVec2 m_TopLeft;
            QT3DSVec2 m_WidthHeight;
            SRect(const QT3DSVec2 &tl = QT3DSVec2(0, 0), const QT3DSVec2 &wh = QT3DSVec2(0, 0))
                : m_TopLeft(tl)
                , m_WidthHeight(wh)
            {
            }
            QT3DSF32 left() const { return m_TopLeft.x; }
            QT3DSF32 top() const { return m_TopLeft.y; }
            QT3DSF32 right() const { return m_TopLeft.x + width(); }
            QT3DSF32 bottom() const { return m_TopLeft.y + height(); }
            QT3DSF32 width() const { return m_WidthHeight.x; }
            QT3DSF32 height() const { return m_WidthHeight.y; }
            QT3DSVec2 topLeft() const { return m_TopLeft; }
            QT3DSVec2 bottomLeft() const { return QT3DSVec2(left(), bottom()); }
            QT3DSVec2 bottomRight() const { return QT3DSVec2(right(), bottom()); }
            QT3DSVec2 topRight() const { return QT3DSVec2(right(), top()); }
            QT3DSVec2 center() const
            {
                return QT3DSVec2(left() + width() / 2.0f, top() + height() / 2.0f);
            }
            SLine leftEdge() const { return SLine(topLeft(), bottomLeft()); }
            SLine rightEdge() const { return SLine(topRight(), bottomRight()); }
            SLine topEdge() const { return SLine(topLeft(), topRight()); }
            SLine bottomEdge() const { return SLine(bottomLeft(), bottomRight()); }
            void translate(QT3DSF32 x, QT3DSF32 y)
            {
                m_TopLeft.x += x;
                m_TopLeft.y += y;
            }
            void translate(const QT3DSVec2 &vec)
            {
                m_TopLeft.x += vec.x;
                m_TopLeft.y += vec.y;
            }
            bool contains(const QT3DSVec2 &inPoint) const;
        };

        struct SControlPoint
        {
            DirectionTypes::Enum m_Direction;
            // World space position
            QT3DSF32 m_Position;
            // This is a calculated value.  Values set will be ignored in favor of recaculating by
            // re-deriving
            // the transition path.
            mutable QT3DSI32 m_PathIndex;
            SControlPoint(DirectionTypes::Enum inDir = DirectionTypes::UnknownDirection,
                          QT3DSF32 pos = 0.0f, QT3DSI32 idx = -1)
                : m_Direction(inDir)
                , m_Position(pos)
                , m_PathIndex(idx)
            {
            }
            bool operator<(const SControlPoint &inOther) const
            {
                return m_PathIndex < inOther.m_PathIndex;
            }
            static QT3DSF32 GetComponent(const QT3DSVec2 &inPoint, DirectionTypes::Enum inDir)
            {
                switch (inDir) {
                case DirectionTypes::Horizontal:
                    return inPoint.y;
                    break;
                case DirectionTypes::Vertical:
                    return inPoint.x;
                    break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
                return 0;
            }
            static DirectionTypes::Enum OppositeDirection(DirectionTypes::Enum inDir)
            {
                switch (inDir) {
                case DirectionTypes::Horizontal:
                    return DirectionTypes::Vertical;
                default:
                    return DirectionTypes::Horizontal;
                }
            }
            static QT3DSF32 GetOrthogonalComponent(const QT3DSVec2 &inPoint, DirectionTypes::Enum inDir)
            {
                switch (inDir) {
                case DirectionTypes::Horizontal:
                    return inPoint.x;
                default:
                    return inPoint.y;
                }
            }
            static QT3DSVec2 FromComponentToVector(QT3DSF32 inComponent, QT3DSF32 orthoComponent,
                                                DirectionTypes::Enum inDir)
            {
                switch (inDir) {
                case DirectionTypes::Horizontal:
                    return QT3DSVec2(orthoComponent, inComponent);
                default:
                    return QT3DSVec2(inComponent, orthoComponent);
                }
            }
            static QT3DSVec2 SetComponent(const QT3DSVec2 &inPoint, QT3DSF32 inValue,
                                       DirectionTypes::Enum inDir)
            {
                switch (inDir) {
                case DirectionTypes::Horizontal:
                    return QT3DSVec2(inPoint.x, inValue);
                default:
                    return QT3DSVec2(inValue, inPoint.y);
                }
            }
            void Set(const QT3DSVec2 &inPoint) { m_Position = GetComponent(inPoint, m_Direction); }
            SLine ToLine() const
            {
                switch (m_Direction) {
                case DirectionTypes::Horizontal:
                    return SLine(QT3DSVec2(0, m_Position), QT3DSVec2(1, m_Position));
                default:
                    return SLine(QT3DSVec2(m_Position, 0), QT3DSVec2(m_Position, 1));
                }
            }
        };

        typedef eastl::vector<SControlPoint> TControlPointList;
        typedef eastl::vector<QT3DSVec2> TPointList;

        struct PointQueryResultType
        {
            enum Enum {
                NoPoint = 0,
                Begin,
                End,
                Control,
            };
        };

        struct SPointQueryResult
        {
            PointQueryResultType::Enum m_QueryType;
            QT3DSI32 m_Index;
            SPointQueryResult(
                PointQueryResultType::Enum inResultType = PointQueryResultType::NoPoint,
                QT3DSI32 inIndex = -1)
                : m_QueryType(inResultType)
                , m_Index(inIndex)
            {
            }
        };

        struct TransitionPathTypes
        {
            enum Enum {
                UnknownPathType = 0,
                BeginToEnd = 1,
                BeginToBegin = 2,
                Targetless = 3,
            };
        };

        class CEditorTransitionPath
        {
            QT3DSF32 m_StateEdgeBuffer;
            Option<SRect> m_BeginRect;
            Option<SRect> m_EndRect;
            SEndPoint m_Begin;
            SEndPoint m_End;
            // Control points that the user has edited.
            TControlPointList m_ControlPoints;
            TransitionPathTypes::Enum m_TransitionPathType;

            // ephemeral data, can be derived from start,end points (along with start/end state) and
            // the
            // user control points.
            mutable TPointList m_Path;
            // The full set of possible control points can be derived from the path.
            mutable TControlPointList m_PossibleControlPoints;

        public:
            CEditorTransitionPath(QT3DSF32 inStateEdgeBuffer = 10.0f)
                : m_StateEdgeBuffer(inStateEdgeBuffer)
                , m_TransitionPathType(TransitionPathTypes::BeginToEnd)
            {
            }
            Option<SRect> GetBeginRect() const { return m_BeginRect; }
            void SetBeginRect(const Option<SRect> &inRect)
            {
                m_BeginRect = inRect;
                MarkDirty();
            }

            Option<SRect> GetEndRect() const { return m_EndRect; }
            void SetEndRect(const Option<SRect> &inRect)
            {
                m_EndRect = inRect;
                MarkDirty();
            }

            TransitionPathTypes::Enum GetPathType() const { return m_TransitionPathType; }
            // May delete all the control points.
            void SetPathType(TransitionPathTypes::Enum inType);
            // This may move control points if instart rect and inendrect have shifted by similar
            // amounts
            // Returns true if persistent data changed, false otherwise.
            bool UpdateBeginEndRects(const SRect &inStartRect, const SRect &inEndRect);
            bool UpdateBeginEndRects(const SRect &inStartRect,
                                     TransitionPathTypes::Enum inPathType);
            eastl::pair<QT3DSVec2, QT3DSVec2> GetBeginEndPoints() const;

            SEndPoint GetEndPoint() const { return m_End; }
            void SetEndPoint(const SEndPoint &pt)
            {
                m_End = pt;
                MarkDirty();
            }
            void SetEndPoint(const QT3DSVec2 &inWorldPoint);

            SEndPoint GetBeginPoint() const { return m_Begin; }
            void SetBeginPoint(const SEndPoint &pt)
            {
                m_Begin = pt;
                MarkDirty();
            }
            void SetBeginPoint(const QT3DSVec2 &inWorldPoint);
            // Return where the end point will be.  Will not return an invalid in point
            // or an end point where information is not set.
            SEndPoint GetActualBeginPoint() const;
            SEndPoint GetActualEndPoint() const;

            TControlPointList GetControlPoints() const { return m_ControlPoints; }
            SControlPoint GetControlPoint(QT3DSI32 inIndex) const
            {
                if (inIndex < (QT3DSI32)m_ControlPoints.size())
                    return m_ControlPoints[inIndex];
                return SControlPoint();
            }
            void SetControlPoints(const TControlPointList &list)
            {
                m_ControlPoints = list;
                MarkDirty();
            }
            void SetControlPoint(QT3DSI32 inControlPointIndex, const QT3DSVec2 &inPosition,
                                 bool inMoveAdjacentEndPoint);

            // Set the control points by updating the individual items in the path list.
            // This works if you do not add/remove points from the list.
            //
            // list = GetPath
            // move points, don't add,delete
            // setControlPoints(list)
            void SetControlPoints(const TPointList &inList);
            TPointList GetPath() const;
            TControlPointList GetPossibleControlPoints() const;
            SControlPoint GetPossibleControlPoint(QT3DSI32 inIdx) const;

            // This may create a new control point thus invalidating the path and possible control
            // points.
            QT3DSI32 MapPossibleControlPointToControlPoint(QT3DSI32 inPossiblePointIndex);
            QT3DSI32 MapControlPointToPossibleControlPoint(QT3DSI32 inPointIndex);
            // Returns true if MapPossibleControlPointToControlPoint will *not* create a new point
            // false otherwise.
            bool DoesControlPointExist(QT3DSI32 inPossiblePointIndex) const;

            // Returns true if any points were removed.
            bool RemoveRedundantControlPoints();

            void RestoreAutoTransition();

            bool IsManualMode() const;

            // Query against the dataset to see what got picked.  Includes the endoints.
            // The rects should be centered about the original and just describe the picking
            // rect to hit against.  Empty rects will not get hit.
            // Note that the result is the index into the possible point list where the hit
            // happened.
            // If you intend to manipulate the point, you need to call
            // MapPossibleControlPointToControlPoint,
            // Which may add a control point which you then can manipulate.
            SPointQueryResult Pick(QT3DSVec2 inPoint, QT3DSVec2 inControlBoxDims,
                                   QT3DSVec2 inEndBoxDims = QT3DSVec2(0, 0));
            SPointQueryResult PickClosestControlPoint(QT3DSVec2 inPoint,
                                                      DirectionTypes::Enum inDirectionType);

            void MarkDirty()
            {
                m_Path.clear();
                m_PossibleControlPoints.clear();
                for (size_t idx = 0, end = m_ControlPoints.size(); idx < end; ++idx)
                    m_ControlPoints[idx].m_PathIndex = -1;
            }

            bool IsDirty() const { return m_Path.empty(); }

            static SEndPoint CalculateEndPoint(const SRect &inRect, const QT3DSVec2 &inPoint,
                                               QT3DSF32 inEdgeBoundary);
            static SEndPoint CalculateDefaultEndPoint(const SRect &inRect, const QT3DSVec2 &inPoint);
            static bool AreAlmostEqual(QT3DSF32 lhs, QT3DSF32 rhs, QT3DSF32 error = .001f)
            {
                if (fabs(lhs - rhs) < error)
                    return true;
                return false;
            }

            static bool AreAlmostEqual(const QT3DSVec2 &lhs, const QT3DSVec2 &rhs, QT3DSF32 error = .001f)
            {
                return AreAlmostEqual(lhs.x, rhs.x, error) && AreAlmostEqual(lhs.y, rhs.y, error);
            }

            // Regenerates the path if this object is dirty.
            void MaybeRegeneratePath() const;

            eastl::pair<QT3DSVec2, EdgeTypes::Enum> GetBeginPointAndEdge() const;

        protected:
            // Output the path between points that have the same direction.
            SControlPoint OutputParallelPoints(const SControlPoint &inLastPoint,
                                               const SControlPoint &inCurrentPoint,
                                               QT3DSF32 inRunWidth) const;
            SControlPoint OutputOrthogonalPoints(const SControlPoint &inLastPoint,
                                                 const SControlPoint &inCurrentPoint) const;
        };
    }
}
}

#endif
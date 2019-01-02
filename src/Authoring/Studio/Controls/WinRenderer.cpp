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

#include "Qt3DSCommonPrecompile.h"

#include "WinRenderer.h"
#include "CoreUtils.h"

//=============================================================================
/**
 * Leaves the Renderer in an invalid state, only for subclasses.
 * In order for this renderer to be valid, you must do two things: (1) you must
 * create a CDC and set m_DC equal to it, and (2) you must call PushClippingRect.
 */
CWinRenderer::CWinRenderer()
    : m_painter(nullptr)
{
}

//=============================================================================
/**
 * Leaves the Renderer in an invalid state, only for subclasses.
 * PushClippingRect must be called in order for this to become valid.
 */
CWinRenderer::CWinRenderer(QPainter *inDC)
{
    m_painter = inDC;
}

CWinRenderer::CWinRenderer(QPainter *inDC, const QRect &inClippingRect)
{
    m_painter = inDC;

    PushClippingRect(inClippingRect);
}

CWinRenderer::~CWinRenderer()
{
    m_Pens.clear();
}

//=============================================================================
/**
 * Draws a rectangle and fills it with inColor.
 * The rectangle has no border and is solid.
 * The coordinates are converted to global space using the current translation.
 * @param inCoordinates the coordinates of the rectangle.
 * @param inColor the color of the rectangle to draw.
 */
void CWinRenderer::FillSolidRect(const QRect &inCoordinates, const QColor &inColor)
{
    QRect theRect(inCoordinates.topLeft() + m_Translation,
                  inCoordinates.size());

     m_painter->fillRect(theRect, inColor);
}

//=============================================================================
/**
 * Draws a rounded rectangle (which actually is just a line) and fills it with inColor.
 * The coordinates are converted to global space using the current translation.
 * @param inCoordinates the coordinates of the rectangle.
 * @param inColor the color of the rectangle to draw.
 */
void CWinRenderer::FillRoundedRect(const QRect &inCoordinates, const QColor &inColor,
                                   bool vertical)
{
    QPen previousPen = m_painter->pen();
    QRect theRect(inCoordinates.topLeft() + m_Translation, inCoordinates.size());
    QPointF startPoint = (theRect.bottomLeft() + theRect.topLeft()) / 2.0;
    QPointF endPoint = (theRect.bottomRight() + theRect.topRight()) / 2.0;
    qreal lineWidth = theRect.bottom() - theRect.top() + 1.0;

    if (vertical) {
        lineWidth = theRect.right() - theRect.left() + 1.0;
        startPoint = (theRect.topRight() + theRect.topLeft()) / 2.0;
        endPoint = (theRect.bottomRight() + theRect.bottomLeft()) / 2.0;
    }

    m_painter->setPen(QPen(inColor, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    m_painter->drawLine(startPoint, endPoint);

    // Restore previous pen
    m_painter->setPen(previousPen);
}

//=============================================================================
/**
 * Move the pen to the position.
 * This will put the pen at inPoint but will not draw a line there from the
 * current location.
 * @param inPoint the location to move to, in local coordinates.
 */
void CWinRenderer::MoveTo(const QPoint &inPoint)
{
    m_currentPos = inPoint + m_Translation;
}

//=============================================================================
/**
 * Move the pen to the position.
 * This will put the pen to the point but will not draw a line there from the
 * current location.
 * @param inX the X location to move to, in local coordinates.
 * @param inY the Y location to move to, in local coordinates.
 */
void CWinRenderer::MoveTo(long inX, long inY)
{
    MoveTo(QPoint(inX, inY));
}

//=============================================================================
/**
 * Draw a line from the current pen location to inPoint.
 * This will draw a line to inPoint and move the current location of the pen
 * to inPoint.
 * @param inPoint the location to draw to, in local coordinates.
 */
void CWinRenderer::LineTo(const QPoint &inPoint)
{
    const QPoint point = inPoint + m_Translation;
    m_painter->drawLine(m_currentPos, point);
    m_painter->drawLine(point, QPoint(point.x(), point.y() + 1));
    m_currentPos = point;
}

//=============================================================================
/**
 * Draw a line from the current pen location to the point.
 * This will draw a line to the point and move the current location of the pen
 * to inPoint.
 * @param inX the X coordinate of the point to draw to, in local coordinates.
 * @param inY the Y coordinate of the point to draw to, in local coordinates.
 */
void CWinRenderer::LineTo(long inX, long inY)
{
    LineTo(QPoint(inX, inY));
}

//=============================================================================
/**
 * Change the active pen color.
 * This acts as a stack so that it does not interfere with previous components.
 * Once the color is done being used PopPen should be called.
 * @param inColor the color to make the current pen.
 * @param inWidth the pen width, in pixels.
 */
void CWinRenderer::PushPen(const QColor &inColor, int inWidth)
{
    QPen thePen = GetPen(inColor, inWidth, Qt::SolidLine);

    QPen theCurrentPen = m_painter->pen();
    m_painter->setPen(thePen);
    m_PenList.push_back(theCurrentPen);
}

void CWinRenderer::PopPen()
{
    QPen thePen = m_PenList.back();
    m_PenList.pop_back();
    m_painter->setPen(thePen);
}

//=============================================================================
/**
 * Copy the bits from another renderer into this one.
 * This will copy the rect from inRenderer into this renderer.
 * inRect will be offset by this renderer's current translation. inXSrc and inYSrc
 * will be offset by inRenderer's current translation.
 * @param inRect the position and size of the area to be copied into this renderer.
 * @param inPixmap the renderer to copy from.
 * @param xSrc the x location of the location to copy from in inRenderer.
 * @param ySrc the y location of the location to copy from in inRenderer.
 */
void CWinRenderer::BitBltFrom(const QRect &inRect, CRenderer *inRenderer, long inXSrc, long inYSrc)
{
    const auto inTranslation = inRenderer->GetTranslation();
    inXSrc += inTranslation.x();
    inYSrc += inTranslation.y();

    auto srcRect = inRect;
    auto destRect = inRect;
    destRect.translate(m_Translation);
    srcRect.moveTo(inXSrc, inYSrc);
    m_painter->save();
    m_painter->setCompositionMode(QPainter::CompositionMode_DestinationOver);
    m_painter->drawPixmap(destRect, inRenderer->pixmap(), srcRect);
    m_painter->restore();
}

//=============================================================================
/**
 * Draws text out without clipping it.
 * @param inPoint the point at which to draw the text. (upper left corner)
 * @param inText the text to draw.
 */
void CWinRenderer::DrawText(float inX, float inY, const QString &inText)
{
    inX += m_Translation.x();
    inY += m_Translation.y();
    m_painter->drawText(QPointF(inX, inY), inText);
}

//=============================================================================
/**
 * Draws text out to a clipped rectangle.
 * If any text occurs outside the bounding box then it will be clipped.
 * @param inPoint the point at which to draw the text. (upper left corner)
 * @param inText the text to draw.
 * @param inBoundingBox the bounding box used to clip the text.
 * @param inColor color to draw the text in
 */
void CWinRenderer::DrawText(float inX, float inY, const QString &inText,
                            const QRect &inBoundingBox, const QColor &inColor)
{
    inX += m_Translation.x();
    inY += m_Translation.y();

    QRectF rect(inBoundingBox);
    rect.translate(inX, inY);
    m_painter->save();
    QPen pen(inColor);
    m_painter->setPen(pen);
    m_painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, inText);
    m_painter->restore();
}

//=============================================================================
/**
 * Draws BOLD text out to a clipped rectangle.
 * If any text occurs outside the bounding box then it will be clipped.
 * @param inPoint the point at which to draw the text. (upper left corner)
 * @param inText the text to draw.
 * @param inBoundingBox the bounding box used to clip the text.
 * @param inColor color to draw the text in
 */
void CWinRenderer::DrawBoldText(float inX, float inY, const QString &inText,
                                const QRect &inBoundingBox, const QColor &inColor)
{
    inX += m_Translation.x();
    inY += m_Translation.y();

    QRectF rect(inBoundingBox);
    rect.translate(inX, inY);
    m_painter->save();
    QPen pen(inColor);
    m_painter->setPen(pen);
    QFont font = m_painter->font();
    font.setBold(true);
    m_painter->setFont(font);
    m_painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, inText);
    m_painter->restore();
}

//=============================================================================
/**
 * Gets the dimensions of the text string as it would be written out to the
 * screen.
 * @param inText the text to check the length on.
 * @return the length of the text in pixels.
 */
QSize CWinRenderer::GetTextSize(const QString &inText)
{
    QFontMetrics fm = m_painter->fontMetrics();
    return fm.size(Qt::TextSingleLine, inText);
}

//=============================================================================
/**
 * Draws a a three-dimensional rectangle with the top and left sides in the
 * color specified by inTopLeftColor and the bottom and right sides in the color
 * specified by inBottomRightColor.
 *
 * @param inRect The rectangle to draw
 * @param inTopLeftColor Color for the top and left sides of the rect
 * @param inBottomRightColor Color for the bottom and right sides of the rect
 */
void CWinRenderer::Draw3dRect(const QRect &inRect, const QColor &inTopLeftColor,
                              const QColor &inBottomRightColor)
{
    auto rect = inRect;
    rect.translate(m_Translation);
    m_painter->drawRect(rect);
    m_painter->save();
    m_painter->setPen(inTopLeftColor);
    m_painter->drawLine(rect.topLeft(), rect.bottomLeft());
    m_painter->drawLine(rect.topLeft(), rect.topRight());
    m_painter->setPen(inBottomRightColor);
    m_painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    m_painter->drawLine(rect.bottomRight(), rect.topRight());
    m_painter->restore();
}

//==============================================================================
/**
 *	DrawBitmap
 *
 *	Draw a bitmap given a device context, position and HBITMAP handle.
 *
 *	@param	inDC		Device context for drawing
 *	@param	inPos		CPoint position for drawing
 *	@param	inBitmap	Handle of the bitmap to draw
 */
//==============================================================================
void CWinRenderer::DrawBitmap(const QPoint &inPos, const QPixmap &inImage)
{
    m_painter->save();
    m_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    m_painter->drawPixmap(inPos + m_Translation, inImage);
    m_painter->restore();;
}

void CWinRenderer::PushTranslation(const QPoint &inTranslation)
{
    m_Translation += inTranslation;

    m_Translations.push_back(inTranslation);
}

void CWinRenderer::PopTranslation()
{
    QPoint thePreviousTranslation = m_Translations.back();
    m_Translation -= thePreviousTranslation;

    m_Translations.pop_back();
}

QPoint CWinRenderer::GetTranslation()
{
    return m_Translation;
}

QPainter* CWinRenderer::GetPainter()
{
    return m_painter;
}

//==============================================================================
/**
 * Get the current clipping rect.
 * The clipping rect is the boundary of pixels that will be drawn to the DC.
 * This can be used to not draw non-visible objects.
 * @return the clipping rect in local coordinates.
 */
QRect CWinRenderer::GetClippingRect()
{
    QRect theClippingRect =  m_painter->clipBoundingRect().toRect();
    theClippingRect.translate(-m_Translation);

    return theClippingRect;
}

//==============================================================================
/**
 * Push a clipping rect onto the stack of clipping rects.
 * This will cause any drawing outside of the clipping rect to be ignored. The
 * Control class also uses this to not draw objects outside of this rect.
 * @param inClippingRect the new clipping rect, in local coordinates.
 */
void CWinRenderer::PushClippingRect(const QRect &inClippingRect)
{
    QRect clippingRect(inClippingRect);
    clippingRect.translate(m_Translation);

    const QRegion currentRegion = m_painter->clipRegion();
    m_painter->setClipRect(clippingRect);

    m_ClippingRegions.push_back(currentRegion);
}

//==============================================================================
/**
 * Pop the current clipping rect.
 * This will change the clipping rect to use the one previous to the current
 * one.
 */
void CWinRenderer::PopClippingRect()
{
    if (m_ClippingRegions.size() > 1) {
        QRegion thePreviousRegion = m_ClippingRegions.back();
        m_painter->setClipRegion(thePreviousRegion);
        m_ClippingRegions.pop_back();
    }
}

//==============================================================================
/**
 *	DrawGradiantBitmap
 *
 *	Draws a gradiant based on the begin color
 *
 *	@param	inRct		Rct tof the control
 *	@param	inBeginColor color to start with
 *	@param	inInverted	true if this is inverted
 */
void CWinRenderer::DrawGradientBitmap(const QRect &inRct, const QColor &inBeginColor, bool inInverted,
                                      double inScalingFactor)
{
    QRect rect(inRct);
    QRect theClippingRect = GetClippingRect();
    rect &= theClippingRect;
    rect.translate(m_Translation);

    long theHeight = rect.height();
    long theWidth = rect.width();

    QImage theCompatibleBitmap(1, theHeight,  QImage::Format_RGB32);

    int theR = inBeginColor.red();
    int theG = inBeginColor.green();
    int theB = inBeginColor.blue();

    double theExtraRModifier = inScalingFactor * (1.0 - (theR / 255.0));
    double theExtraGModifier = inScalingFactor * (1.0 - (theG / 255.0));
    double theExtraBModifier = inScalingFactor * (1.0 - (theB / 255.0));

    for (long thePixel = 0; thePixel < theHeight; ++thePixel) {
        double theNormPixel = (double)thePixel / (theHeight * 4.8);
        double theCos = 1.0 / (theNormPixel * theNormPixel + 1.0);
        double theRValue = (double)theR * (theCos + theExtraRModifier);
        double theGValue = (double)theG * (theCos + theExtraGModifier);
        double theBValue = (double)theB * (theCos + theExtraBModifier);

        QColor theTempColor(::dtoi(theRValue), ::dtoi(theGValue), ::dtoi(theBValue));
        if (inInverted) {
            theCompatibleBitmap.setPixelColor(0, qMax(0l, theHeight - thePixel - 2), theTempColor);
        } else {
            theCompatibleBitmap.setPixelColor(0, thePixel, theTempColor);
        }
        theExtraRModifier *= 0.3;
        theExtraGModifier *= 0.3;
        theExtraBModifier *= 0.3;
    }

    m_painter->save();
    m_painter->drawImage(QRect(rect.x(), rect.y(), theWidth, theHeight), theCompatibleBitmap);
    m_painter->restore();
}

//=============================================================================
/**
 * Draw a gradient over inRect.
 * This will blend horizontally across the rect from inBeginColor into inEndColor.
 * This does linear interpolation.
 * @param inRect the rect to draw on.
 * @param inBeginColor the start (left most) color.
 * @param inEndColor the final (right most) color.
 */
void CWinRenderer::DrawGradient(const QRect &inRect, const QColor &inBeginColor, const QColor &inEndColor)
{
    const QRect rect = inRect.translated(m_Translation);
    QLinearGradient gradient(rect.topLeft(), rect.topRight());
    gradient.setColorAt(0, inBeginColor);
    gradient.setColorAt(1, inEndColor);

    QBrush brush(gradient);
    m_painter->fillRect(rect, brush);
}

void CWinRenderer::FillHashed(const QRect &inRect, const QColor &inForegroundColor)
{
    m_painter->save();

    QBrush theBrush(inForegroundColor, Qt::BDiagPattern);
    QPen pen(inForegroundColor);
    m_painter->setPen(pen);
    m_painter->setBrush(theBrush);
    m_painter->drawRect(inRect.translated(m_Translation));

    m_painter->restore();
}

QPen CWinRenderer::GetPen(const QColor &inColor, int inWidth, Qt::PenStyle inStyle)
{
    QPen thePen;
    SPenInfo theInfo = { inColor, inWidth, inStyle };
    TPenMap::iterator thePos = m_Pens.find(theInfo);
    if (thePos != m_Pens.end()) {
        thePen = thePos->second;
    } else {
        thePen.setColor(inColor);
        thePen.setWidth(inWidth);
        m_Pens[theInfo] = thePen;
    }
    return thePen;
}

QPixmap CWinRenderer::pixmap() const
{
    return {};
}

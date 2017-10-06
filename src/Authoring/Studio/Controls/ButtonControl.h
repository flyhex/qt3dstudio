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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_BUTTON_CONTROL_H
#define INCLUDED_BUTTON_CONTROL_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "Multicaster.h"

#include <QPixmap>

#include <boost/signals.hpp>

//==============================================================================
//	Forwards
//==============================================================================

//=============================================================================
/**
 * Base class for creating button controls.
 */
class CButtonControl : public CControl
{
public:
    /// States for procedural buttons that might result in drawing changes
    enum EButtonState {
        EBUTTONSTATE_UP, ///< Indicates that the button is currently in the up position
        EBUTTONSTATE_DOWN, ///< Indicates that the button is currently in the up position
        EBUTTONSTATE_INDETERMINATE ///< Not used yet; provided for sub-classes to implement a
                                   ///tri-state functionality
    };

protected:
    EButtonState m_State; ///< Specifies what state the button is currently in; state changes as the
                          ///button is clicked on
    QPixmap m_ImageUp; ///< The image for the button in its normal state
    QPixmap m_ImageDown; ///< The image for the button while it is being clicked
    QPixmap m_ImageOver; ///< The image for the button while the mouse is over
    QPixmap m_ImageUpDisabled; ///< The image for the button when it is disabled and unavailable
                                  ///to the user
    QPixmap m_ImageDownDisabled; ///< The image for the button when it is disabled and unavailable
                                    ///to the user
    bool m_AutoSize; ///< true if the button will automatically resize itself
    bool m_MouseEnter; ///< Flag for determining mouse enter/exit status so that the number of
                       ///invalidations is minimized.  See OnMouseOver and OnMouseOut.
    bool m_IsMouseDown;
    bool m_VCenterImage;
    bool m_HCenterImage;
    long m_Margin;

public:
    CButtonControl();
    virtual ~CButtonControl();

    EButtonState GetButtonState() const;
    virtual void SetButtonState(EButtonState inState);

    void SetUpImage(const QPixmap &inImage);
    void SetUpImage(const QString &inImageName);
    void SetDownImage(const QPixmap &inImage);
    void SetDownImage(const QString &inImageName);
    void SetOverImage(const QPixmap &inImage);
    void SetOverImage(const QString &inImageName);
    void SetDisabledImage(const QPixmap &inImage);
    void SetDisabledImage(const QString &inImageName);
    void SetUpDisabledImage(const QString &inImageName);
    QSize GetImageSize() const;

    void SetAutoSize(bool inEnableAutoSize);
    bool GetAutoSize();

    void SetCenterImage(bool inVCenter, bool inHCenter);
    void SetMarginImage(long inMargin);

    void Draw(CRenderer *inRenderer) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void Invalidate(bool inIsInvalidated = true)  override;

    boost::signal1<void, CControl *> SigButtonDown;
    boost::signal1<void, CControl *> SigButtonUp;
    boost::signal1<void, CControl *> SigClicked;

protected:
    virtual void Render(CRenderer *inRenderer);
    virtual QPixmap GetCurrentImage() const;
    virtual void Resize();
};

#endif // INCLUDED_BUTTON_CONTROL_H

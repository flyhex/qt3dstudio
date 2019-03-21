/****************************************************************************
**
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

#ifndef Q3DSPRESENTATION_H
#define Q3DSPRESENTATION_H

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include "q3dsdatainput.h"

QT_BEGIN_NAMESPACE

class Q3DSPresentationPrivate;
class Q3DSElement;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

class Q_STUDIO3D_EXPORT Q3DSPresentation : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DSPresentation)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QStringList variantList READ variantList WRITE setVariantList NOTIFY variantListChanged)

public:
    explicit Q3DSPresentation(QObject *parent = nullptr);
    ~Q3DSPresentation();

    QUrl source() const;
    QStringList variantList() const;

    void registerElement(Q3DSElement *scene);
    void unregisterElement(Q3DSElement *scene);
    Q3DSElement *registeredElement(const QString &elementPath) const;

    void registerDataInput(Q3DSDataInput *dataInput);
    void unregisterDataInput(Q3DSDataInput *dataInput);
    Q3DSDataInput *registeredDataInput(const QString &name) const;

    Q_INVOKABLE QVariantList getDataInputs() const;
    QVector<Q3DSDataInput *> dataInputs() const;

    // Input event handlers
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

public Q_SLOTS:
    void setSource(const QUrl &source);
    void setVariantList(const QStringList &variantList);
    void goToSlide(const QString &elementPath, unsigned int index);
    void goToSlide(const QString &elementPath, const QString &name);
    void goToSlide(const QString &elementPath, bool next, bool wrap);
    void goToTime(const QString &elementPath, float time);
    void setAttribute(const QString &elementPath, const QString &attributeName,
                      const QVariant &value);
    void setPresentationActive(const QString &id, bool active);
    void fireEvent(const QString &elementPath, const QString &eventName);
    void setGlobalAnimationTime(qint64 milliseconds);
    void setDataInputValue(const QString &name, const QVariant &value,
                           Q3DSDataInput::ValueRole valueRole = Q3DSDataInput::ValueRole::Value);

Q_SIGNALS:
    void variantListChanged(const QStringList &variantList);
    void sourceChanged(const QUrl &source);
    void slideEntered(const QString &elementPath, unsigned int index, const QString &name);
    void slideExited(const QString &elementPath, unsigned int index, const QString &name);
    // Indicates that data input definitions in the Studio project have been parsed
    // and datainputs are available through dataInputs() / getDataInputs().
    void dataInputsReady();

private:
    Q_DISABLE_COPY(Q3DSPresentation)
    Q3DSPresentationPrivate *d_ptr;

    friend class Q3DSPresentationItem;
    friend class Q3DSSurfaceViewerPrivate;
    friend class Q3DSWidgetPrivate;
    friend class Q3DSRenderer;
    friend class Q3DSView;
    friend class Q3DSDataInput;
};

QT_END_NAMESPACE

#endif // Q3DSPRESENTATION_H

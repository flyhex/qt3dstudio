/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "q3dsdatainput_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q3DSDataInput::Q3DSDataInput(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataInputPrivate(this))
{
}

Q3DSDataInput::Q3DSDataInput(const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataInputPrivate(this))
{
    d_ptr->m_name = name;
}

Q3DSDataInput::Q3DSDataInput(Q3DSPresentation *presentation, const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataInputPrivate(this))
{
    d_ptr->m_name = name;
    d_ptr->m_presentation = presentation;
    if (presentation)
        presentation->registerDataInput(this);
}

Q3DSDataInput::Q3DSDataInput(Q3DSDataInputPrivate *d, Q3DSPresentation *presentation,
                             const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(d)
{
    d_ptr->m_name = name;
    d_ptr->m_presentation = presentation;
    if (presentation)
        presentation->registerDataInput(this);
}

Q3DSDataInput::~Q3DSDataInput()
{
    delete d_ptr;
}

QString Q3DSDataInput::name() const
{
    return d_ptr->m_name;
}

void Q3DSDataInput::setName(const QString &name)
{
    if (d_ptr->m_name != name) {
        d_ptr->m_name = name;
        if (d_ptr->m_presentation)
            d_ptr->m_presentation->registerDataInput(this);
        Q_EMIT nameChanged();
    }
}

QVariant Q3DSDataInput::value() const
{
    return d_ptr->m_value;
}

float Q3DSDataInput::min() const
{
    if (!d_ptr->m_presentation)
        return 0.0f;

    return d_ptr->m_presentation->d_ptr->dataInputMin(d_ptr->m_name);
}

float Q3DSDataInput::max() const
{
    if (!d_ptr->m_presentation)
        return 0.0f;

    return d_ptr->m_presentation->d_ptr->dataInputMax(d_ptr->m_name);
}

bool Q3DSDataInput::isValid() const
{
    if (d_ptr->m_presentation)
        return d_ptr->m_presentation->d_ptr->isValidDataInput(this);
    else
        return false;
}

void Q3DSDataInput::setMin(float min)
{
    if (!d_ptr->m_presentation)
        return;

    d_ptr->m_presentation->setDataInputValue(d_ptr->m_name, min, ValueRole::Min);
    d_ptr->m_min = min;
    emit minChanged();
}

void Q3DSDataInput::setMax(float max)
{
    if (!d_ptr->m_presentation)
        return;

    d_ptr->m_presentation->setDataInputValue(d_ptr->m_name, max, ValueRole::Max);
    d_ptr->m_max = max;
    emit maxChanged();
}

void Q3DSDataInput::setValue(const QVariant &value)
{
    // Since properties controlled by data inputs can change without the current value being
    // reflected on the value of the DataInput element, we allow setting the value to the
    // same one it was previously and still consider it a change.
    // For example, when controlling timeline, the value set to DataInput will only be
    // the current value for one frame if presentation has a running animation.
    // In order to track an element property, see DataOutput API.
    d_ptr->setValue(value, ValueRole::Value);
    Q_EMIT valueChanged();
}

void Q3DSDataInputPrivate::setPresentation(Q3DSPresentation *presentation)
{
    m_presentation = presentation;
}

Q3DSDataInputPrivate::Q3DSDataInputPrivate(Q3DSDataInput *parent)
    : q_ptr(parent)
{
}

Q3DSDataInputPrivate::~Q3DSDataInputPrivate()
{
    if (m_presentation)
        m_presentation->unregisterDataInput(q_ptr);
}

void Q3DSDataInputPrivate::setValue(const QVariant &value, Q3DSDataInput::ValueRole valueRole)
{
    m_value = value;
    if (m_presentation)
        m_presentation->setDataInputValue(m_name, m_value, valueRole);
}

void Q3DSDataInputPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;

    if (m_viewerApp && m_value.isValid())
        setValue(m_value);
}

void Q3DSDataInputPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;

    if (m_commandQueue && m_value.isValid())
        setValue(m_value);
}

QT_END_NAMESPACE

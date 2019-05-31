/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include "q3dsqmlstreamproducer.h"
#include "q3dsqmlstreamrenderer.h"

Q3DSQmlStreamProducer::Q3DSQmlStreamProducer(const QString &id, const QString &args,
                                               QObject *parent)
    : QObject(parent), m_id(id), m_args(args), m_renderer(new Q3DSQmlStreamRenderer)
{

}

Q3DSQmlStreamProducer::~Q3DSQmlStreamProducer()
{
    delete m_renderer;
}

IQ3DSQmlStreamRenderer* Q3DSQmlStreamProducer::getRenderer()
{
    return m_renderer;
}

void Q3DSQmlStreamProducer::setPresentationItem(QQuickItem *item)
{
    m_renderer->setItem(item);
}

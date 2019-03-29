/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "FilterVariantsDlg.h"
#include "StudioPreferences.h"
#include "FilterVariantsModel.h"

#include <QtWidgets/qaction.h>
#include <QtCore/qtimer.h>
#include <QtQml/qqmlcontext.h>

FilterVariantsDlg::FilterVariantsDlg(QWidget *parent, QAction *action, int actionSize,
                                     QWidget *actionWidget)
    : QQuickWidget(parent)
    , m_model(new FilterVariantsModel(m_variantsFilter, this))
    , m_action(action)
    , m_actionSize(actionSize)
    , m_actionWidget(actionWidget)
{
    setWindowTitle(tr("Filter variants"));
    QTimer::singleShot(0, this, &FilterVariantsDlg::initialize);
}

void FilterVariantsDlg::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty(QStringLiteral("_view"), this);
    rootContext()->setContextProperty(QStringLiteral("_model"), m_model);
    rootContext()->setContextProperty(QStringLiteral("_utils"), &m_qmlUtils);
    setSource(QUrl(QStringLiteral("qrc:/Application/FilterVariantsDlg.qml")));
}

QString FilterVariantsDlg::filterStr() const
{
    QString ret;
    if (!m_variantsFilter.isEmpty()) {
        const auto groups = m_variantsFilter.keys();
        for (auto &g : groups) {
            const auto group = m_variantsFilter[g];
            for (auto &tag : group)
                ret.append(g + QLatin1Char(':') + tag + QLatin1Char(','));
        }

        if (!m_variantsFilter.isEmpty())
            ret.chop(1);
    }

    return ret;
}

void FilterVariantsDlg::clearFilter()
{
    m_variantsFilter.clear();
}

int FilterVariantsDlg::actionSize() const
{
    return m_actionSize;
}

void FilterVariantsDlg::showEvent(QShowEvent *event)
{
    m_model->refresh();
    QQuickWidget::showEvent(event);
}

void FilterVariantsDlg::focusOutEvent(QFocusEvent *e)
{
    QQuickWidget::focusOutEvent(e);

    if (!m_actionWidget->underMouse()) {
        m_action->setChecked(false);
        QTimer::singleShot(0, this, &QQuickWidget::close);
    }
}

void FilterVariantsDlg::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        m_action->setChecked(false);
        QTimer::singleShot(0, this, &QQuickWidget::close);
    }

    QQuickWidget::keyPressEvent(e);
}

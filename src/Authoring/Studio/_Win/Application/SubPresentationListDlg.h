/****************************************************************************
**
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

#ifndef SUBPRESENTATIONLISTDLG_H
#define SUBPRESENTATIONLISTDLG_H

#include <QtWidgets/qdialog.h>

#include "Doc.h"

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class SubPresentationListDlg;
}
QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QStandardItemModel)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)

class CSubPresentationListDlg : public QDialog
{
    Q_OBJECT
public:
    CSubPresentationListDlg(const QString &directory,
                            const QVector<SubPresentationRecord> &subpresentations,
                            QWidget* parent = nullptr);
    ~CSubPresentationListDlg();
    QVector<SubPresentationRecord> subpresentations() const;

protected:
    void initDialog();
    void updateButtons();
    void updateContents();
    QString getUniqueId(const QString &id, bool editing = false);
    void keyPressEvent(QKeyEvent *event);

private Q_SLOTS:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void onAddSubPresentation();
    void onRemoveSubPresentation();
    void onEditSubPresentation();
    void onActivated(const QModelIndex &index);
    void onSelectionChanged();

private:
    Ui::SubPresentationListDlg *m_ui;
    QString m_directory;
    QVector<SubPresentationRecord> m_records;
    int m_currentIndex;
    QStandardItemModel *m_tableContents;
};

#endif

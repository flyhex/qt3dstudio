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

#include "DataInputDlg.h"
#include "ui_DataInputDlg.h"
#include "Qt3DSMessageBox.h"
#include "StudioApp.h"
#include "Dialogs.h"

#include <QtWidgets/qabstractbutton.h>
#include <QtWidgets/qpushbutton.h>
#include <QtGui/qstandarditemmodel.h>
#include <QtWidgets/qstyleditemdelegate.h>

CDataInputDlg::CDataInputDlg(CDataInputDialogItem **datainput, QStandardItemModel *data,
                             QWidget *parent, const QVector<EDataType> acceptedTypes)
    : QDialog(parent)
    , m_ui(new Ui::DataInputDlg)
    , m_data(data)
    , m_dataInput(*datainput)
    , m_name(m_dataInput->name)
    , m_type(0)
    , m_min(0.0)
    , m_max(10.0)
    , m_acceptedTypes(acceptedTypes)
{
    m_ui->setupUi(this);

    // For enabling stylesheet for drop-down items
    QStyledItemDelegate *itemDelegate = new QStyledItemDelegate();
    m_ui->comboBoxTypeList->setItemDelegate(itemDelegate);

    m_ui->comboBoxTypeList->addItem(tr("Boolean"), QVariant(DataTypeBoolean));
#ifdef DATAINPUT_EVALUATOR_ENABLED
    m_ui->comboBoxTypeList->addItem(tr("Evaluator"), QVariant(DataTypeEvaluator));
#endif
    m_ui->comboBoxTypeList->addItem(tr("Float"), QVariant(DataTypeFloat));
    m_ui->comboBoxTypeList->addItem(tr("Ranged Number"), QVariant(DataTypeRangedNumber));
    m_ui->comboBoxTypeList->addItem(tr("String"), QVariant(DataTypeString));
    m_ui->comboBoxTypeList->addItem(tr("Variant"), QVariant(DataTypeVariant));
    m_ui->comboBoxTypeList->addItem(tr("Vector2"), QVariant(DataTypeVector2));
    m_ui->comboBoxTypeList->addItem(tr("Vector3"), QVariant(DataTypeVector3));

    QStandardItemModel *model
            = qobject_cast<QStandardItemModel *>(m_ui->comboBoxTypeList->model());

    for (int i = 0; i < m_ui->comboBoxTypeList->model()->rowCount(); ++i)
    {
        QStandardItem *item = model->item(i, 0);
        if (!acceptedTypes.contains((EDataType)i))
            item->setEnabled(false);
    }

    const auto keys = m_dataInput->metadata.keys();
    for (auto &k : keys)
        m_orderedMetadata.append({k, m_dataInput->metadata.value(k)});

    initDialog();

    window()->setFixedSize(size());

    connect(m_ui->comboBoxTypeList,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CDataInputDlg::onTypeChanged);
    connect(m_ui->doubleSpinBoxMin,
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CDataInputDlg::onMinChanged);
    connect(m_ui->doubleSpinBoxMax,
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &CDataInputDlg::onMaxChanged);
    connect(m_ui->lineEditInputName, &QLineEdit::textChanged, this, &CDataInputDlg::onNameChanged);
    // Clunky way of making TAB jump over the delete button column, as setting focusPolicy
    // does not seem to work with cellWidget.
    connect(m_ui->tableEditMetadata, &QTableWidget::currentCellChanged,
            this, [this](int currentRow, int currentColumn, int previousRow, int previousColumn) {
        Q_UNUSED(previousRow);
        Q_UNUSED(previousColumn);
        if (currentColumn == 2) {
            if (currentRow < (m_ui->tableEditMetadata->rowCount() - 1))
                m_ui->tableEditMetadata->setCurrentCell(currentRow + 1, 0);
            else
                m_ui->tableEditMetadata->setCurrentCell(0, 0);
        }
    });
}

CDataInputDlg::~CDataInputDlg()
{
    delete m_ui;
}

void CDataInputDlg::initDialog()
{
    // Disallow special characters and whitespaces
    QRegExpValidator *rxp = new QRegExpValidator(QRegExp("[A-Za-z0-9_]+"), this);
    m_ui->lineEditInputName->setValidator(rxp);

    if (!m_dataInput->name.isEmpty()) {
        m_name = m_dataInput->name;
        if (m_dataInput->type == DataTypeRangedNumber) {
            m_min = m_dataInput->minValue;
            m_max = m_dataInput->maxValue;
        }
#ifdef DATAINPUT_EVALUATOR_ENABLED
        else if (m_type == DataTypeEvaluator) {
            m_ui->lineEditEvaluation->setText(m_dataInput->valueString);
        }
#endif
    } else {
        m_name = getUniqueId(tr("newDataInput"));
        if (m_dataInput->type == DataTypeRangedNumber) {
            m_dataInput->minValue = m_min;
            m_dataInput->maxValue = m_max;
        }
    }

    m_type = m_dataInput->type;
    m_ui->comboBoxTypeList->setCurrentIndex(m_type);
    m_ui->lineEditInputName->setText(m_name);
    if (m_type == DataTypeRangedNumber) {
        m_ui->doubleSpinBoxMin->setValue(m_dataInput->minValue);
        m_ui->doubleSpinBoxMax->setValue(m_dataInput->maxValue);
    }

    m_ui->tableEditMetadata->setColumnCount(3);
    m_ui->tableEditMetadata->setColumnWidth(0, 150);
    m_ui->tableEditMetadata->setColumnWidth(1, 280);
    m_ui->tableEditMetadata->setColumnWidth(2, 10);
    m_ui->tableEditMetadata->setHorizontalHeaderLabels({tr("Key"), tr("Metadata"), {}});

    populateMetadata(m_orderedMetadata.size() + 1);

    m_ui->tableEditMetadata->installEventFilter(this);
    updateVisibility(m_dataInput->type);
}

void CDataInputDlg::accept()
{
    if (checkDuplicateKeys()) {
        QString title(QObject::tr("Warning"));
        QString text(QObject::tr("Metadata keys must be unique."));
        g_StudioApp.GetDialogs()->DisplayMessageBox(title, text,
                                                    Qt3DSMessageBox::ICON_WARNING, false,
                                                    this);
        return;
    }

    for (const auto &it : qAsConst(m_orderedMetadata)) {
        if (it.first.trimmed().isEmpty()) {
            QString title(QObject::tr("Warning"));
            QString text(QObject::tr("Metadata keys cannot be empty."));
            g_StudioApp.GetDialogs()->DisplayMessageBox(title, text,
                                                        Qt3DSMessageBox::ICON_WARNING, false,
                                                        this);
            return;
        }
    }

    if (m_dataInput->name != m_name)
        m_dataInput->name = getUniqueId(m_name);

    m_dataInput->type = m_type;
    if (m_type == DataTypeRangedNumber) {
        m_dataInput->minValue = m_min;
        m_dataInput->maxValue = m_max;
    }
#ifdef DATAINPUT_EVALUATOR_ENABLED
    else if (m_type == DataTypeEvaluator) {
        m_dataInput->valueString = m_text;
    }
#endif

    m_dataInput->metadata.clear();
    for (auto const &it : qAsConst(m_orderedMetadata))
        m_dataInput->metadata.insert(it.first, it.second);

    QDialog::accept();
}

void CDataInputDlg::onTypeChanged(int type)
{
    updateVisibility(type);
    m_type = type;
}

void CDataInputDlg::onMinChanged(float min)
{
    if (m_ui->doubleSpinBoxMax->value() < min)
        m_ui->doubleSpinBoxMax->setValue(min);
    m_min = min;
}

void CDataInputDlg::onMaxChanged(float max)
{
    if (m_ui->doubleSpinBoxMin->value() > max)
        m_ui->doubleSpinBoxMin->setValue(max);
    m_max = max;
}

void CDataInputDlg::onNameChanged(const QString &name)
{
    int cursorPos = m_ui->lineEditInputName->cursorPosition();
    m_name = name;
    m_ui->lineEditInputName->setText(m_name);
    m_ui->lineEditInputName->setCursorPosition(cursorPos);
}

QString CDataInputDlg::getUniqueId(const QString &id)
{
    QString retval = QStringLiteral("%1").arg(id);
    int idx = 1;
    while (m_data->findItems(retval, Qt::MatchExactly, 0).size() && idx < 1000) {
        retval = QStringLiteral("%1_%2").arg(id).arg(idx, 3, 10, QLatin1Char('0'));
        ++idx;
    }
    return retval;
}

void CDataInputDlg::updateVisibility(int type)
{
    if (type == DataTypeRangedNumber) {
        m_ui->labelMin->setVisible(true);
        m_ui->labelMax->setVisible(true);
        m_ui->doubleSpinBoxMin->setVisible(true);
        m_ui->doubleSpinBoxMax->setVisible(true);
    } else {
        m_ui->labelMin->setVisible(false);
        m_ui->labelMax->setVisible(false);
        m_ui->doubleSpinBoxMin->setVisible(false);
        m_ui->doubleSpinBoxMax->setVisible(false);
    }
#ifdef DATAINPUT_EVALUATOR_ENABLED
    if (type == DataTypeEvaluator) {
        m_ui->lineEditEvaluation->setVisible(true);
        m_ui->labelEvaluation->setVisible(true);
    } else {
        m_ui->lineEditEvaluation->setVisible(false);
        m_ui->labelEvaluation->setVisible(false);
    }
#endif
    // Adjust text label positioning according to the
    // visibility of info text warning about allowed datatypes.
    if (m_dataInput->ctrldElems.size()) {
        m_ui->labelInfoText->setVisible(true);
        m_ui->infoTxtSpacer->changeSize(20, 18);
    } else {
        m_ui->labelInfoText->setVisible(false);
        m_ui->infoTxtSpacer->changeSize(20, 0);
    }
}

const bool CDataInputDlg::isEquivalentDataType(int dlgType,
                                               qt3dsdm::DataModelDataType::Value dmType,
                                               bool strict)
{
    if ((dlgType == EDataType::DataTypeString
         && dmType == qt3dsdm::DataModelDataType::String)
        || (dlgType == EDataType::DataTypeRangedNumber
            && dmType == qt3dsdm::DataModelDataType::RangedNumber)
        || (dlgType == EDataType::DataTypeFloat
            && (dmType == qt3dsdm::DataModelDataType::Float
                || (dmType == qt3dsdm::DataModelDataType::String && !strict)))
        || (dlgType == EDataType::DataTypeBoolean
            && dmType == qt3dsdm::DataModelDataType::Bool)
        || (dlgType == EDataType::DataTypeVector3
            && dmType == qt3dsdm::DataModelDataType::Float3)
        || (dlgType == EDataType::DataTypeVector2
            && dmType == qt3dsdm::DataModelDataType::Float2)
        // Variant can be bound to any property type except
        // as timeline controller because only datainput of type Ranged Number
        // has additional min/max information. For slide control,
        // we can allow variant type in addition to String type.
        || (dlgType == EDataType::DataTypeVariant
            && dmType != qt3dsdm::DataModelDataType::RangedNumber)
#ifdef DATAINPUT_EVALUATOR_ENABLED
        || dlgType == EDataType::DataTypeEvaluator
#endif
        ) {
        return true;
    }

    return false;
}

QVector<EDataType> CDataInputDlg::getAcceptedTypes(qt3dsdm::DataModelDataType::Value dmType,
                                                   bool strict)
{
    QVector<EDataType> acceptedTypes;
    for (auto candidate : allDataTypes) {
        if (isEquivalentDataType(candidate, dmType, strict))
            acceptedTypes.append(candidate);
    }
    return acceptedTypes;
}

bool CDataInputDlg::eventFilter(QObject *obj, QEvent *ev)
{
    // Eat Enter if the user is editing metadata, to avoid inadvertent accept of
    // the entire dialog.
    if (obj == m_ui->tableEditMetadata && ev->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(ev)->key() == Qt::Key_Return
            || static_cast<QKeyEvent *>(ev)->key() == Qt::Key_Enter) {
            QKeyEvent *tabEv = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
            QApplication::sendEvent(m_ui->tableEditMetadata, tabEv);
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void CDataInputDlg::populateMetadata(int rows)
{
    m_ui->tableEditMetadata->clearContents();
    m_ui->tableEditMetadata->setRowCount(rows);

    QList<QPair<QString, QString>>::iterator it = m_orderedMetadata.begin();
    for (int i = 0; i < rows; ++i) {
        if (it != m_orderedMetadata.end()) {
            addMetadataRow(it->first, it->second, i);
            it++;
        } else {
            addMetadataRow({}, {}, i);
        }
    }
}

void CDataInputDlg::addMetadataRow(const QString &key, const QString &metadata, int row)
{
    // Grow table if required
    if (row > m_ui->tableEditMetadata->rowCount() - 1)
        m_ui->tableEditMetadata->setRowCount(row + 1);

    static const QIcon trashIcon = QIcon(":/images/Action-Trash-Normal.png");
    QLineEdit *keyEdit = new QLineEdit();
    QLineEdit *metadataEdit = new QLineEdit();

    keyEdit->installEventFilter(this);
    metadataEdit->installEventFilter(this);

    keyEdit->setText(key);
    metadataEdit->setText(metadata);
    // '$' is used as a delimiter character in .UIP
    const QRegExpValidator *rxpNoDollar = new QRegExpValidator(QRegExp("[^\\$]+"), this);
    keyEdit->setValidator(rxpNoDollar);
    metadataEdit->setValidator(rxpNoDollar);

    QPushButton *delButton = new QPushButton(trashIcon, {});

    m_ui->tableEditMetadata->setCellWidget(row, 0, keyEdit);
    m_ui->tableEditMetadata->setCellWidget(row, 1, metadataEdit);
    m_ui->tableEditMetadata->setCellWidget(row, 2, delButton);

    // Check whether the user is editing an existing pair or inserting a new one.
    // In the latter case append a new blank row.
    connect(keyEdit, &QLineEdit::editingFinished, this, [&, keyEdit, metadataEdit] {
        auto currRow = m_ui->tableEditMetadata->currentRow();

        if (currRow < m_orderedMetadata.size())
            m_orderedMetadata.replace(currRow, {keyEdit->text(), metadataEdit->text()});
        else
            m_orderedMetadata.append({keyEdit->text(), metadataEdit->text()});

        // Require both key and metadata to be non-empty before appending a new
        // blank row.
        if (currRow == (m_ui->tableEditMetadata->rowCount() - 1)
            && !static_cast<QLineEdit *>(m_ui->tableEditMetadata->cellWidget(currRow, 1))
                ->text().isEmpty()) {
            addMetadataRow({}, {}, currRow + 1);
            m_ui->tableEditMetadata->scrollToBottom();
        }
    });

    connect(metadataEdit, &QLineEdit::editingFinished, this, [&, keyEdit, metadataEdit] {
        auto currRow = m_ui->tableEditMetadata->currentRow();
        if (currRow < m_orderedMetadata.size())
            m_orderedMetadata.replace(currRow, {keyEdit->text(), metadataEdit->text()});
        else
            m_orderedMetadata.append({keyEdit->text(), metadataEdit->text()});

        // Require both key and metadata to be non-empty before appending a new
        // blank row.
        if (currRow == (m_ui->tableEditMetadata->rowCount() - 1)
            && !static_cast<QLineEdit *>(m_ui->tableEditMetadata->cellWidget(currRow, 0))
                ->text().isEmpty()) {
            addMetadataRow({}, {}, currRow + 1);
            m_ui->tableEditMetadata->scrollToBottom();
        }
    });

    connect(delButton, &QPushButton::clicked, this, [&, delButton] {
        auto currRow = m_ui->tableEditMetadata->indexAt(delButton->pos()).row();
        // Never delete last row as it is a placeholder for new items.
        if (currRow < m_ui->tableEditMetadata->rowCount() - 1) {
            m_orderedMetadata.removeAt(currRow);
            m_ui->tableEditMetadata->removeRow(currRow);
        }
    });
}

bool CDataInputDlg::checkDuplicateKeys() const
{
    for (int i = 0; i < m_orderedMetadata.size() - 1; ++i) {
        auto key = m_orderedMetadata[i].first;
        for (int j = i + 1; j < m_orderedMetadata.size(); ++j) {
            if (key == m_orderedMetadata[j].first)
                return true;
        }
    }
    return false;
}

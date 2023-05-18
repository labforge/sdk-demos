#include "ui/parameterwidget.hpp"
#include <string>
#include <variant>
#include <PvGenParameterArray.h>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QRubberBand>
#include <QtWidgets/QDoubleSpinBox>

using namespace std;
using namespace labforge::ui;

static const string s_ignored_parameter_groups[] =
        {"Root\\Deprecated",
         "Root\\TransportLayerControl",
         "Root\\TestControl",
         "Root\\EventControl",
         "Root\\SourceControl",
         "Root\\AcquisitionControl",
         "Root\\ImageFormatControl",
         "Root\\DeviceControl"
        };

static bool s_is_visible_control(const char*name) {
  string str(name);
  for(const string & s : s_ignored_parameter_groups) {
    if(str.starts_with(s))
      return false;
  }
  return true;
}

static variant<int, double, const char*,bool> s_read_parameter(PvGenParameter*param) {
  PvGenType type;
  param->GetType(type);

  double dtmp;
  int64_t itmp;
  PvString str;

  switch(type) {
//    case PvGenTypeBoolean:
//      break;
    case PvGenTypeFloat:
      dynamic_cast<PvGenFloat*>(param)->GetValue(dtmp);
      return dtmp;
    case PvGenTypeInteger:
      dynamic_cast<PvGenInteger*>(param)->GetValue(itmp);
      return (int)itmp;
    case PvGenTypeString:
      dynamic_cast<PvGenString*>(param)->GetValue(str);
      return str.GetAscii();
    // To Satisfy GCC
    case PvGenTypeEnum:
      break;
    case PvGenTypeBoolean:
      break;
    case PvGenTypeCommand:
      break;
    case PvGenTypeRegister:
      break;
    case PvGenTypeUndefined:
      break;
  }
  return 0;
}

static QString s_format_parameter(PvGenParameter*param, variant<int, double, const char*,bool> value) {
  QString result;
  PvGenType type;
  PvResult res;
  PvString str;

  param->GetType(type);
  switch(type) {
//    case PvGenTypeBoolean:
//      break;
    case PvGenTypeFloat:
      result = QString::number(get<double>(value), 'f', 2);
      res = dynamic_cast<PvGenFloat*>(param)->GetUnit(str);
      if((std::string(str.GetAscii()).length() > 0) && res.IsOK()) {
        result = result + ' ' + str.GetAscii();
      }
      break;
    case PvGenTypeInteger:
      result = QString::number(get<int>(value));
      res = dynamic_cast<PvGenInteger*>(param)->GetUnit(str);
      if((std::string(str.GetAscii()).length() > 0) && res.IsOK()) {
        result = result + ' ' + str.GetAscii();
      }
      break;
    case PvGenTypeString:
      result = get<const char*>(value);
      break;
      // To satisfy GCC
    case PvGenTypeEnum:
      break;
    case PvGenTypeBoolean:
      break;
    case PvGenTypeCommand:
      break;
    case PvGenTypeRegister:
      break;
    case PvGenTypeUndefined:
      break;
  }
  return result;
}

QParameterDelegate::QParameterDelegate(QObject *parent, std::map<const std::string, PvGenParameter *> *params)
        : QStyledItemDelegate(parent), m_params(params) { }

QWidget * QParameterDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  (void)option;
  // Figure out what parameter we have
  if(index.column() != 1)
    return nullptr;

  QString name = index.sibling(index.row(), 0).data().toString();
  if(!m_params->contains(name.toStdString()))
    return nullptr;

  PvGenParameter*param = m_params->at(name.toStdString());

  // Are we actually writable
  if(!param->IsWritable())
    return nullptr;

  // Figure out what type we have
  PvGenType type;
  PvResult res = param->GetType(type);
  if(!res.IsOK())
    return nullptr;

  QWidget *editor = nullptr;
  switch(type) {
//    case PvGenTypeBoolean:
//      bol = dynamic_cast<PvGenBoolean *>(param);
//      bool res;
//      res = bol->GetValue(res);
//      value = to_string(res);
//      break;
    case PvGenTypeFloat:
      editor = new QDoubleSpinBox(parent);
      break;
    case PvGenTypeInteger:
      editor = new QSpinBox(parent);
      break;
    case PvGenTypeString:
      editor = new QInputDialog(parent);
      break;
      // To satisfy GCC
    case PvGenTypeEnum:
      break;
    case PvGenTypeBoolean:
      break;
    case PvGenTypeCommand:
      break;
    case PvGenTypeRegister:
      break;
    case PvGenTypeUndefined:
      break;
  }

  return editor;
}

void QParameterDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  if(index.column() != 1)
    return;
  QString name = index.sibling(index.row(), 0).data().toString();
  if(!m_params->contains(name.toStdString()))
    return;

  PvGenParameter*param = m_params->at(name.toStdString());

  // Are we actually writable
  if(!param->IsWritable())
    return;

  // Figure out what type we have
  PvGenType type;
  PvResult res = param->GetType(type);
  if(!res.IsOK())
    return;

  QDoubleSpinBox *qdss;
  PvGenFloat *pF;
  QSpinBox *qss;
  PvGenInteger *pI;
  QInputDialog *inp;
  PvGenString *pS;
  PvString str;
  double dtmp;
  int64_t itmp;

  switch(type) {
//    case PvGenTypeBoolean:
//      break;
    case PvGenTypeFloat:
      qdss = dynamic_cast<QDoubleSpinBox*>(editor);
      pF = dynamic_cast<PvGenFloat*>(param);
      res = pF->GetMax(dtmp);
      if(res.IsOK())
        qdss->setMaximum(dtmp);
      res = pF->GetMin(dtmp);
      if(res.IsOK())
        qdss->setMinimum(dtmp);
      res = pF->GetUnit(str);
      if(res.IsOK())
        qdss->setSuffix(str.GetAscii());
      pF->GetValue(dtmp);
      qdss->setValue(dtmp);
      break;
    case PvGenTypeInteger:
      qss = dynamic_cast<QSpinBox*>(editor);
      pI =  dynamic_cast<PvGenInteger*>(param);
      res = pI->GetMax(itmp);
      if(res.IsOK())
        qss->setMaximum((int)itmp);
      res = pI->GetMin(itmp);
      if(res.IsOK())
        qss->setMinimum((int)itmp);
      res = pI->GetUnit(str);
      if(res.IsOK())
        qss->setSuffix(str.GetAscii());
      pI->GetValue(itmp);
      qss->setValue((int)itmp);
      break;
    case PvGenTypeString:
      inp = dynamic_cast<QInputDialog*>(editor);
      pS = dynamic_cast<PvGenString*>(param);
      pS->GetMaxLength(itmp);
      pS->GetValue(str);
      inp->setTextValue(str.GetAscii());
      break;
      // To Satisfy GCC
    case PvGenTypeEnum:
      break;
    case PvGenTypeBoolean:
      break;
    case PvGenTypeCommand:
      break;
    case PvGenTypeRegister:
      break;
    case PvGenTypeUndefined:
      break;
  }
}

void QParameterDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  if(index.column() != 1)
    return;
  QString name = index.sibling(index.row(), 0).data().toString();
  if(!m_params->contains(name.toStdString()))
    return;

  // Signal to the parent that editing is done
  // Direct parent is the viewport parent of the treeview, it's parent is the TreeView
  ParameterWidget*parent = dynamic_cast<ParameterWidget*>(editor->parentWidget()->parentWidget());
  if(parent) {
    parent->EditingDone();
  }

  PvGenParameter*param = m_params->at(name.toStdString());

  // Are we actually writable
  if(!param->IsWritable())
    return;

  // Figure out what type we have
  PvGenType type;
  PvResult res = param->GetType(type);
  if(!res.IsOK())
    return;

  QDoubleSpinBox *qdss;
  PvGenFloat *pF;
  PvGenInteger *pI;
  QSpinBox *qss;
  double dtmp;
  int itmp;
  QString result;
  PvString str;
  QInputDialog *inp;
  PvGenString *pS;

  switch(type) {
//    case PvGenTypeBoolean:
//      break;
    case PvGenTypeFloat:
      qdss = dynamic_cast<QDoubleSpinBox*>(editor);
      pF = dynamic_cast<PvGenFloat*>(param);
      dtmp = qdss->value();
      result = s_format_parameter(param, dtmp);
      // Propagate update to sensor
      res = pF->SetValue(dtmp);
      if(!res.IsOK())
        return;
      break;
    case PvGenTypeInteger:
      qss = dynamic_cast<QSpinBox*>(editor);
      pI =  dynamic_cast<PvGenInteger*>(param);
      itmp = qss->value();
      result = s_format_parameter(param, itmp);
      // Propagate update to sensor
      res = pI->SetValue(itmp);
      if(!res.IsOK())
        return;
      break;
    case PvGenTypeString:
      inp = dynamic_cast<QInputDialog*>(editor);
      pS = dynamic_cast<PvGenString*>(param);
      result = inp->textValue();
      // Propagate update to sensor
      res = pS->SetValue(result.toStdString().c_str());
      if(!res.IsOK())
        return;
      break;
      // To Satisfy GCC
    case PvGenTypeEnum:
      break;
    case PvGenTypeBoolean:
      break;
    case PvGenTypeCommand:
      break;
    case PvGenTypeRegister:
      break;
    case PvGenTypeUndefined:
      break;
  }
  // Update model if the update worked
  model->setData(index, result);
}

void ParameterWidget::setupUi() {
 /* QStringList headerLabels;
  headerLabels.push_back(tr("Parameter"));
  headerLabels.push_back(tr("Value"));

  setColumnCount(headerLabels.count());
  setHeaderLabels(headerLabels);

  setItemDelegateForColumn(1, new QParameterDelegate(this, &m_name_to_parameter));*/

  OnDisconnect();
}

ParameterWidget::ParameterWidget(QWidget *parent) : QTreeWidget(parent) {
  connect(this, &QTreeWidget::itemDoubleClicked, this, &ParameterWidget::OnTreeWidgetItemDoubleClicked);
  installEventFilter(this);
}

void ParameterWidget::editItem(QTreeWidgetItem *item, int column) {
  m_editing = true;
  QTreeWidget::editItem(item, column);
}

bool ParameterWidget::eventFilter(QObject *object, QEvent *event) {
  if (object == this && event->type() == QEvent::KeyRelease ) {
    QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
    if (((keyEvent->key() == Qt::Key_F2))
    && !m_editing) {
      QTreeWidgetItem *item = currentItem();
      if(item != nullptr)
        editItem(item, 1);
      // Special tab handling
      return true;
    } else {
      return false;
    }
  }
  return false;
}

bool ParameterWidget::OnConnect(PvDevice *device) {
  if(!device)
    return false;

  m_device = device;
  if(pollParameters()) {
    setEnabled(true);
    return true;
  } else {
    m_device = nullptr;
    return false;
  }
}

void ParameterWidget::OnDisconnect() {
  // Remove all parameters and related mappings
  clear();
  m_parameter_to_control.clear();
  m_name_to_parameter.clear();
  setEnabled(false);
  m_device = nullptr; // Clear associated device
}

bool ParameterWidget::pollParameters() {
  if(!m_device)
    return false;

  map<string,QTreeWidgetItem *> mCategoryToWidget;
  QList<QTreeWidgetItem *> categories;
  PvGenParameterArray *lDeviceParams = m_device->GetParameters();
  for(size_t i = 0; i < lDeviceParams->GetCount(); i++) {
    if(lDeviceParams->Get(i)->IsAvailable() && lDeviceParams->Get(i)->IsImplemented()) {
      PvGenType type;
      PvString name = lDeviceParams->Get(i)->GetName();
      PvString cat;
      PvResult res = lDeviceParams->Get(i)->GetCategory(cat);
      if(!s_is_visible_control(cat.GetAscii())) // Skip ignored parameter groups
        continue;

      // Strip Root\ from category
      string category_trunc = string(cat.GetAscii()).substr(5, strlen(cat.GetAscii()));
      if(!res.IsOK()) {
        continue;
      }
      lDeviceParams->Get(i)->GetType(type);
      QString value = s_format_parameter(lDeviceParams->Get(i), s_read_parameter(lDeviceParams->Get(i)));
      int alignment = Qt::AlignRight;

      QTreeWidgetItem *root;
      // Find root note containing category label
      if(mCategoryToWidget.contains(category_trunc)) {
        root = mCategoryToWidget[category_trunc];
      } else {
        // If not found, create one and map it
        root = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(QString(category_trunc.c_str())));
        root->setFirstColumnSpanned(true);
        root->setFlags(root->flags() & ~Qt::ItemIsSelectable);

        mCategoryToWidget[category_trunc] = root;
        categories.append(root);
      }
      // Create the value tree-widget item
      QTreeWidgetItem *item = new QTreeWidgetItem(root, {name.GetAscii(), value});
      item->setFlags(item->flags() | Qt::ItemIsEditable);
      item->setTextAlignment(1, alignment);
      PvString tmp;
      res = lDeviceParams->Get(i)->GetToolTip(tmp);
      if(res.IsOK()) {
        item->setToolTip(0, tmp.GetAscii());
      }
      // Populate parameter map
      m_parameter_to_control[name.GetAscii()] = item;
      m_name_to_parameter[name.GetAscii()] = lDeviceParams->Get(i);
      // FIXME: maybe some static label field onclick ... res = lDeviceParams->Get(i)->GetDescription(tmp);
    }
  }
  // Poll out min max ranges
  insertTopLevelItems(0, categories);
  expandAll();
  resizeColumnToContents(0);
  resizeColumnToContents(1);
  return true;
}

void ParameterWidget::OnTreeWidgetItemDoubleClicked(QTreeWidgetItem *item, int column) {
  if(column == 1)
    editItem(item, column);
}
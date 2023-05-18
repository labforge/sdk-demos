#ifndef _parameterwidget_hpp_
#define _parameterwidget_hpp_

#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeView>
#include <QHeaderView>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <PvGenParameter.h>
#include <map>
#include <memory>
#include <PvDevice.h>


namespace labforge::ui {

  class ParameterWidget : public QTreeWidget {
    Q_OBJECT
  public:
    explicit ParameterWidget(QWidget *parent = nullptr);
    virtual ~ParameterWidget() = default;
    void setupUi();

    void OnDisconnect();
    bool OnConnect(PvDevice *device);
    bool eventFilter(QObject *object, QEvent *event);
    void editItem(QTreeWidgetItem *item, int column = 0);
    void EditingDone() { m_editing = false; }

  private:
    bool pollParameters();

    bool m_editing{};
    std::map<const std::string, QTreeWidgetItem*> m_parameter_to_control;
    std::map<const std::string, PvGenParameter*> m_name_to_parameter;
    PvDevice * m_device;

  };
}

#endif // _parameterwidget_hpp_
#include "mock_device.hpp"
#include "ui/parameterwidget.hpp"
#include <QtTest/QtTest>

namespace labforge::test {
  class TestParameters : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void TestConnectAndRead();
    void TestCreation();
  private:
    std::vector<std::pair<const RegisterDefinition*, std::variant<int, float, std::string>>> m_regs;
    std::unique_ptr<MockDevice> m_device;
  };
}
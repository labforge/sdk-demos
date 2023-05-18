#include <qtest_widgets.h> // Enable GUI features
#include "test_parameters.hpp"
#include "util.hpp"

using namespace std;
using namespace labforge::test;
using namespace labforge::ui;

#define INITIAL_EXPOSURE_MS 1000
#define INITIAL_GAIN 2.0f

void TestParameters::initTestCase() {
  // Find a NIC with a MAC address
  QString nic = findNIC();

  // Define regs
  RegisterDefinition *exposure = new RegisterDefinition(0x1000,
                                                        PvGenTypeInteger,
                                                        "exposure",
                                                        PvGenAccessModeReadWrite,
                                                        0,
                                                        8000,
                                                        "exp",
                                                        "ttip",
                                                        "ctrls",
                                                        "ms");
  RegisterDefinition *gain = new RegisterDefinition(0x1010,
                                                    PvGenTypeFloat,
                                                    "gain",
                                                    PvGenAccessModeReadWrite,
                                                    1.0,
                                                    8.0,
                                                    "gain value",
                                                    "ttip gain",
                                                    "ctrls gain",
                                                    nullptr);
  m_regs.emplace_back(make_pair(
          exposure,
          INITIAL_EXPOSURE_MS));
  m_regs.emplace_back(make_pair(
          gain,
          INITIAL_GAIN));
  try {
    m_device = make_unique<MockDevice>(nic.toStdString(),
                                       &m_regs,
                                       1920,
                                       1080);
  } catch(const exception & e) {
    QFAIL("Device creation failed");
  }
}

void TestParameters::cleanupTestCase() {
  for( auto i : m_regs ) {
    delete i.first;
  }
}

void TestParameters::TestCreation() {
  ParameterWidget w;
  w.setupUi();
  // Check initial state as disabled
  QVERIFY2(w.columnCount() == 2, "Header not initialised");
  QVERIFY2(!w.isEnabled(), "Invalid disconnect state");
}

void TestParameters::TestConnectAndRead() {
  ParameterWidget w;
  w.setupUi();

  // Connect to mock device
  QString nic = findNIC();
  PvResult res;
  PvDevice *lDevice = PvDevice::CreateAndConnect( nic.toStdString().c_str(), &res );
  bool b = w.OnConnect(dynamic_cast<PvDeviceGEV*>(lDevice));
  QVERIFY2(b, "Cannot connect to mock device");
  // Check state change

  QVERIFY2(w.isEnabled(), "Invalid disconnect state");
  // Check initial parameter values
  QTreeWidgetItemIterator it(&w, QTreeWidgetItemIterator::Editable);
  size_t seen_parameters = 2;
  while (*it) {
    QTreeWidgetItem *widget = *it;
    if(widget->text(0) == "exposure") {
      QVERIFY2((widget->text(1) == "1000 ms"), "Exposure with unit not properly formatted");
      seen_parameters++;
    }
    if(widget->text(0) == "gain") {
      QVERIFY2((widget->text(1) == "2.00"), "Gain unitless not properly formatted");
      seen_parameters++;
    }
    cout << "ELEM: " << widget->text(0).toStdString() << " = " << widget->text(1).toStdString() << endl;
    ++it;
  }

  // Set Exposure and GAIN
  // FIXME_sometime https://www.francescmm.com/testing-qtablewidget-with-qtest/

  // Disconnect and verify state
  w.OnDisconnect();
  QVERIFY2(!w.isEnabled(), "Invalid disconnect state");
  QTreeWidgetItemIterator it2(&w, QTreeWidgetItemIterator::Editable);
  seen_parameters = 0;
  while (*it2) {
    seen_parameters++;
    ++it2;
  }
  QVERIFY2(seen_parameters == 0, "Parameters not erased from view");

  lDevice->Disconnect();
  PvDevice::Free( lDevice );
}

QTEST_MAIN(TestParameters)



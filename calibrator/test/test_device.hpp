#ifndef __test_device_hpp__
#define __test_device_hpp__

#include <QtTest/QtTest>
#include "mock_device.hpp"
#include <PvDevice.h>
#include <PvDeviceGEV.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvBuffer.h>

namespace labforge::test {

  class TestDevice: public QObject {
    Q_OBJECT
  private slots:
    void initTestCase();
    void cleanupTestCase();

    // Pretty much PvStreamSample
    void doTestStream();
    void doTestParameters();
    void doTestChunks();
  private:

    std::unique_ptr<MockDevice> m_device;
    cv::Mat m_test_img;
    std::vector<std::pair<const RegisterDefinition*, std::variant<int, float, std::string>>> m_regs;
  };

}

#endif // __test_device_hpp__
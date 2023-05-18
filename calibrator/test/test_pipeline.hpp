#ifndef __test_pipeline_hpp__
#define __test_pipeline_hpp__

#include <memory>
#include <QtTest/QtTest>
#include <PvSoftDeviceGEVInterfaces.h>
#include <PvStreamingChannelSourceDefault.h>
#include <iostream>
#include <limits>
#include <memory>
#include <variant>
#include <vector>
#include <map>
#include <QString>
#include <opencv2/core.hpp>
#include <PvSoftDeviceGEV.h>
#include <PvFPSStabilizer.h>
#include "mock_device.hpp"

namespace labforge::test {
  class TestPipeline : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void testPipeline();

  private:
    std::unique_ptr<MockDevice> m_device;
    cv::Mat m_test_img;
    std::vector<std::pair<const RegisterDefinition*, std::variant<int, float, std::string>>> m_regs;
  };
}

#endif // __test_pipeline_hpp__

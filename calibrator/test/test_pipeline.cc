#include "test_pipeline.hpp"
#include "util.hpp"
#include "gev/pipeline.hpp"
#include "gev/util.hpp"

using namespace labforge::test;
using namespace labforge::gev;
using namespace std;
using namespace cv;

void TestPipeline::initTestCase() {
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
      1000));
  m_regs.emplace_back(make_pair(
      gain,
      2.0f));
  try {
    m_device = make_unique<MockDevice>(nic.toStdString(),
                                       &m_regs,
                                       1920,
                                       1080);
  } catch(const exception & e) {
    QFAIL("Device creation failed");
  }

  // Load a test image
  m_test_img = imread("testdata/asa_1080p.png");
  if(m_test_img.empty())
    QVERIFY2(false, "Could not load testdata");

  QVERIFY2(m_device->QueueImage(m_test_img), "Could not queue image");
}

void TestPipeline::testPipeline() {
  PvDevice *lDevice;
  PvStream *lStream;
  list<PvBuffer *> lBufferList;

  // Find first ethernet device
  QString nic = findNIC();

  PvResult res;
  lDevice = PvDevice::CreateAndConnect( nic.toStdString().c_str(), &res );
  QVERIFY2(nullptr != lDevice, (string("Could not connect cause : ")
                                +res.GetCodeString().GetAscii()).c_str());

  lStream = OpenStream( nic.toStdString().c_str(), &res );
  QVERIFY2(nullptr != lStream, (string("Could not start stream cause : ")
                                +res.GetCodeString().GetAscii()).c_str());

  ConfigureStream( lDevice, lStream );

  try {
    Pipeline p(dynamic_cast<PvStreamGEV *>(lStream), dynamic_cast<PvDeviceGEV *>(lDevice));
    QVERIFY2(p.Start(), "Started pipeline");

    list<tuple<Mat*, Mat*, vector<chunks_t>>> images;
    // Try to get at least one image pair
    bool got_image = false;
    size_t trials = 0;
    while(p.IsStarted() && !got_image && trials < 10) {
      QThread::currentThread()->usleep(100*1000);
      p.GetPairs(images);
      if(!images.empty()) {
        got_image = true;
      } else {
        trials++;
      }
    }
    QVERIFY2(got_image, "Unable to receive images");
    // Convert image
    tuple<Mat*, Mat*, vector<chunks_t>> pair = images.front();
    Mat tmp;
    cvtColor(*get<0>(pair), tmp, COLOR_YUV2BGR_YUY2);
    resize(tmp, tmp, Size(m_test_img.cols, m_test_img.rows));
    QVERIFY2(GetImageSimilarity(tmp, m_test_img) < 0.05, "Received image differs");
    cvtColor(*get<1>(pair), tmp, COLOR_YUV2BGR_YUY2);
    resize(tmp, tmp, Size(m_test_img.cols, m_test_img.rows));
    QVERIFY2(GetImageSimilarity(tmp, m_test_img) < 0.05, "Received image differs");

    p.Stop();
  } catch(...) {
    QFAIL("Unexpected exception");
  }
}

void TestPipeline::cleanupTestCase() {
  for( auto i : m_regs ) {
    delete i.first;
  }
}

QTEST_MAIN(TestPipeline)
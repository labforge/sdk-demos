#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <PvSoftDeviceGEV.h>
#include <PvSystem.h>
#include <opencv2/opencv.hpp>

#include "test_device.hpp"
#include "util.hpp"
#include "gev/util.hpp"
#include "labforge/gev/chunkdata.h"

using namespace labforge::test;
using namespace labforge::gev;
using namespace std;
using namespace cv;

#define INITIAL_EXPOSURE_MS 28
#define INITIAL_GAIN 1.1f
#define INITIAL_NEG -1

void TestDevice::initTestCase() {
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
  RegisterDefinition *neg = new RegisterDefinition(0x1014,
                                                   PvGenTypeInteger,
                                                   "negative_test",
                                                   PvGenAccessModeReadWrite,
                                                   -10.0,
                                                   8.0,
                                                   "negative_test",
                                                   "ttip negative",
                                                   "ctrls negative",
                                                   nullptr);
  m_regs.emplace_back(make_pair(
          exposure,
          INITIAL_EXPOSURE_MS));
  m_regs.emplace_back(make_pair(
          gain,
          INITIAL_GAIN));
  m_regs.emplace_back(make_pair(neg, INITIAL_NEG));
  try {
    m_device = make_unique<MockDevice>(nic.toStdString(),
                                       &m_regs,
                                       1920,
                                       1080);
  } catch(const exception & e) {
    QFAIL(e.what());
  }

  // Load a test image
  m_test_img = imread("testdata/asa_1080p.png");
  if(m_test_img.empty())
    QVERIFY2(false, "Could not load testdata");

  QVERIFY2(m_device->QueueImage(m_test_img), "Could not queue image");
}

void TestDevice::cleanupTestCase() {
  for( auto i : m_regs ) {
    delete i.first;
  }
}

void TestDevice::doTestParameters() {
  QString nic = findNIC();
  PvResult res;
  PvDevice *lDevice = PvDevice::CreateAndConnect( nic.toStdString().c_str(), &res );
  PvDeviceGEV *gDevice = dynamic_cast<PvDeviceGEV *>(lDevice);
  QVERIFY2(nullptr != gDevice, (string("Could not connect cause : ")
                                +res.GetCodeString().GetAscii()).c_str());

  PvGenParameterArray *params = gDevice->GetParameters();
  PvGenParameter *exp = params->Get("exposure");
  QVERIFY2(exp != nullptr, "Parameter missing");
  bool b = false;
  exp->IsAvailable(b);
  QVERIFY2(b, "Parameter not available");
  exp->IsReadable(b);
  QVERIFY2(b, "Parameter not readable");
  exp->IsWritable(b);
  QVERIFY2(b, "Parameter not writable");
  PvGenInteger *iexp = dynamic_cast<PvGenInteger *>(exp);
  int64_t val;
  iexp->GetValue(val);
  QVERIFY2(INITIAL_EXPOSURE_MS == val, "Parameter not initialized");

  PvGenParameter *gain = params->Get("gain");
  b = false;
  gain->IsAvailable(b);
  QVERIFY2(b, "Parameter not available");
  gain->IsReadable(b);
  QVERIFY2(b, "Parameter not readable");
  gain->IsWritable(b);
  QVERIFY2(b, "Parameter not writable");
  PvGenFloat *fgain = dynamic_cast<PvGenFloat *>(gain);
  QVERIFY2(fgain != nullptr, "Parameter missing");
  double dval;
  res = fgain->GetValue(dval);
  QVERIFY2(res.IsOK(), "Error polling gain");
  QVERIFY2(fabs(dval - INITIAL_GAIN) < 0.1, "Gain not properly set");

  // Negative values test
  PvGenParameter *neg = params->Get("negative_test");
  b = false;
  QVERIFY2(neg != nullptr, "Parameter not available");
  neg->IsReadable(b);
  QVERIFY2(b, "Parameter not readable");
  neg->IsWritable(b);
  QVERIFY2(b, "Parameter not writable");
  PvGenInteger*ineg = dynamic_cast<PvGenInteger*>(neg);
  QVERIFY2(ineg != nullptr, "Parameter missing");
  val = 0;
  res = ineg->GetValue(val);
  QVERIFY2(res.IsOK(), "Error polling gain");
  QVERIFY2(val == INITIAL_NEG, "Gain not properly set");

  // Setting parameters through GEV link
  res = fgain->SetValue(7.0);
  QVERIFY2(res.IsOK(), "Could not set gain");
  res = iexp->SetValue(1000);
  QVERIFY2(res.IsOK(), "Could not set exposure");

  res = fgain->GetValue(dval);
  QVERIFY2(fabs(dval - 7.0) < 0.1, "Gain not properly set");

  res = fgain->SetValue(0.9);
  QVERIFY2(!res.IsOK(), "Invalid gain setting permitted");

  // Setting parameters through test shim
  b = m_device->SetRegisterValue("exposure", 100);
  QVERIFY2(b, "Could not set exposure through test shim");
  iexp->GetValue(val);
  QVERIFY2(100 == val, "Test shim not updating");

  // Getting parameters through test shim
  iexp->SetValue(1001);
  std::variant<int, float, string> vval;
  m_device->GetRegisterValue("exposure", vval);
  QVERIFY2(get<int>(vval) == 1001, "Test shim not updated");

  // Close the stream
  lDevice->Disconnect();
  PvDevice::Free( lDevice );
}

void TestDevice::doTestStream() {
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
  // Create and queue buffers
  CreateStreamBuffers(lDevice, lStream, &lBufferList, 16);
  auto lIt = lBufferList.begin();
  while ( lIt != lBufferList.end() ) {
    lStream->QueueBuffer( *lIt );
    lIt++;
  }
  std::vector<std::variant<chunk_dnn_t, chunk_features_t, chunk_descriptors_t>> chunks;
  Mat * last_image = AcquireImages( lDevice, lStream, chunks );
  QVERIFY2(last_image != nullptr, "No images received");
  cv::resize(*last_image, *last_image, cv::Size(m_test_img.cols, m_test_img.rows));
  FreeStreamBuffers( &lBufferList );
  QVERIFY2(GetImageSimilarity(*last_image, m_test_img) < 0.05, "Received image differs");
  delete last_image;

  // Close the stream
  lStream->Close();
  PvStream::Free( lStream );
  PvDevice::Free( lDevice );
}

void TestDevice::doTestChunks() {
  PvDevice *lDevice = nullptr;
  PvStream *lStream;
  list<PvBuffer *> lBufferList;

  // Find first ethernet device
  QString nic = findNIC();

  PvResult res;
  lDevice = PvDevice::CreateAndConnect( nic.toStdString().c_str(), &res );
  QVERIFY2(nullptr != lDevice, (string("Could not connect cause : ")
                                +res.GetDescription().GetAscii()).c_str());

  lStream = OpenStream( nic.toStdString().c_str(), &res );
  QVERIFY2(nullptr != lStream, (string("Could not start stream cause : ")
                                +res.GetCodeString().GetAscii()).c_str());

  ConfigureStream( lDevice, lStream );

  // Configure chunk data
  PvGenParameterArray *params = lDevice->GetParameters();
  // Enable chunk mode
  PvGenBoolean*chunk_enable = dynamic_cast<PvGenBoolean*>(params->Get("ChunkModeActive"));
  chunk_enable->SetValue(true);
  // Enable all chunks
  chunk_enable = dynamic_cast<PvGenBoolean*>(params->Get("ChunkEnable"));
  PvGenEnum* chunk_selection = dynamic_cast<PvGenEnum*>(params->Get("ChunkSelector"));
  res = chunk_selection->SetValue(CHUNK_DNN_TEXT);
  QVERIFY2(res.IsOK(), (string("Could not select ")+CHUNK_DNN_TEXT+" cause: "
                                +res.GetCodeString().GetAscii()).c_str());
  chunk_enable->SetValue(true);
  res = chunk_selection->SetValue(CHUNK_FEATURES_RIGHT_TXT);
  QVERIFY2(res.IsOK(), (string("Could not select ")+CHUNK_FEATURES_RIGHT_TXT+" cause: "
                        +res.GetCodeString().GetAscii()).c_str());
  chunk_enable->SetValue(true);

  // Queue test data
  chunk_dnn_t *dnn_chunk = new chunk_dnn_t ;
  dnn_chunk->count = 1;
  dnn_chunk->detection[0].top = 1;
  dnn_chunk->detection[0].left = 2;
  dnn_chunk->detection[0].bottom = 3;
  dnn_chunk->detection[0].right = 4;
  dnn_chunk->detection[0].cid = 0xFF;
  dnn_chunk->detection[0].score = 0.99;

  chunk_features_t *ft = new chunk_features_t ;
  ft->count = 2;
  ft->features[0].x = 1;
  ft->features[0].y = 2;
  ft->features[1].x = 3;
  ft->features[1].y = 4;

  m_device->GetMockSource()->SetDetection(dnn_chunk);
  m_device->GetMockSource()->SetFeatures(1, ft);

  // Create and queue buffers
  CreateStreamBuffers(lDevice, lStream, &lBufferList, 16);
  auto lIt = lBufferList.begin();
  while ( lIt != lBufferList.end() ) {
    lStream->QueueBuffer( *lIt );
    lIt++;
  }

  std::vector<std::variant<chunk_dnn_t, chunk_features_t, chunk_descriptors_t>> chunks;
  Mat * last_image = AcquireImages( lDevice, lStream, chunks );
  QVERIFY2(chunks.size() == 2, "Chunk mismatch");
  chunk_dnn_t & received_dnn = get<chunk_dnn_t>(chunks[0]);
  chunk_features_t &received_ft = get<chunk_features_t>(chunks[1]);

  QVERIFY2(received_ft.count == ft->count, "FT count mismatch");
  QVERIFY2(ft->features[0].x == received_ft.features[0].x &&
           ft->features[0].y == received_ft.features[0].y &&
           ft->features[1].x == received_ft.features[1].x &&
           ft->features[1].y == received_ft.features[1].y, "Feature mismatch");
  QVERIFY2(received_dnn.count == dnn_chunk->count, "DNN count mismatch");
  QVERIFY2(dnn_chunk->detection[0].top == received_dnn.detection[0].top &&
  dnn_chunk->detection[0].left == received_dnn.detection[0].left &&
  dnn_chunk->detection[0].bottom == received_dnn.detection[0].bottom &&
  dnn_chunk->detection[0].right == received_dnn.detection[0].right &&
  dnn_chunk->detection[0].cid == received_dnn.detection[0].cid &&
  dnn_chunk->detection[0].score == received_dnn.detection[0].score, "DNN mismatch");


  QVERIFY2(last_image != nullptr, "No images received");
  cv::resize(*last_image, *last_image, cv::Size(m_test_img.cols, m_test_img.rows));
  FreeStreamBuffers( &lBufferList );
  QVERIFY2(GetImageSimilarity(*last_image, m_test_img) < 0.05, "Received image differs");
  delete last_image;

  // Close the stream
  lStream->Close();
  PvStream::Free( lStream );
  PvDevice::Free( lDevice );
}

QTEST_MAIN(TestDevice)
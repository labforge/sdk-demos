#include <iostream>
#include <vector>

#include "util.hpp"
#include "mock_device.hpp"

using namespace std;
using namespace cv;
using namespace labforge::test;

int main(int argc, char**argv) {
  // Find a NIC with a MAC address
  QString nic = findNIC();

  vector<pair<const RegisterDefinition*,variant<int,float,string>>> regs;

  // Define regs
  RegisterDefinition exposure(0x1000,
                                                        PvGenTypeInteger,
                                                        "exposure",
                                                        PvGenAccessModeReadWrite,
                                                        0,
                                                        8000,
                                                        "exp",
                                                        "ttip",
                                                        "ctrls",
                                                        "ms");
  RegisterDefinition gain(0x1010,
                                                    PvGenTypeFloat,
                                                    "gain",
                                                    PvGenAccessModeReadWrite,
                                                    1.0,
                                                    8.0,
                                                    "gain value",
                                                    "ttip gain",
                                                    "ctrls",
                                                    nullptr);
  RegisterDefinition neg(0x1014,
                                                   PvGenTypeInteger,
                                                   "negative",
                                                   PvGenAccessModeReadWrite,
                                                   -10.0,
                                                   8.0,
                                                   "negative_test",
                                                   "ttip negative",
                                                   "ctrls",
                                                   nullptr);

  regs.emplace_back(make_pair(
          &exposure,
          1000));
  regs.emplace_back(make_pair(
          &gain,
          1.0f));
  regs.emplace_back(make_pair(
          &neg,
          -1));

  MockDevice device(nic.toStdString(),
                    &regs,
                    1920,
                    1080);

  // Load a test image
  Mat test_img = imread("testdata/asa_1080p.png");
  device.QueueImage(test_img);
  // Fake some NN detections
  chunk_dnn_t *detections = new chunk_dnn_t;
  detections->count = 2;
  detections->detection[0].cid = 8;
  detections->detection[0].score = 0.991f;
  detections->detection[0].left = 379*2;
  detections->detection[0].top = 72*2;
  detections->detection[0].right = 470*2;
  detections->detection[0].bottom = 120*2;

  detections->detection[1].cid = 7;
  detections->detection[1].score = 0.991f;
  detections->detection[1].left = 413*2;
  detections->detection[1].top = 262*2;
  detections->detection[1].right = 452*2;
  detections->detection[1].bottom = 310*2;

  chunk_features_t *right_ft = new chunk_features_t ;
  right_ft->count = 1;
  right_ft->features[0].x = 283*2;
  right_ft->features[0].y = 455*2;

  chunk_features_t *left_ft = new chunk_features_t ;
  left_ft->count = 1;
  left_ft->features[0].x = 283*2;
  left_ft->features[0].y = 492*2;

  device.GetMockSource()->SetDetection(detections);
  device.GetMockSource()->SetFeatures(0, left_ft);
  device.GetMockSource()->SetFeatures(1, right_ft);
  // Keep Device up and running for testing
  for(;;) { }

  delete detections;

  return 0;
}
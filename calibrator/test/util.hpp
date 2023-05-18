#ifndef _util_hpp_
#define _util_hpp_


#include <PvStream.h>
#include <PvDeviceGEV.h>
#include <opencv2/opencv.hpp>
#include <QString>
#include <variant>
//#include "labforge/gev/chunkdata.h"

namespace labforge::test {



// Utility functions
cv::Mat* AcquireImages(PvDevice *aDevice, PvStream *aStream, std::vector<std::variant<chunk_dnn_t, chunk_features_t, chunk_descriptors_t>> &chunks);
double GetImageSimilarity(cv::Mat &l, cv::Mat &r);
QString findNIC();

}

#endif // _util_hpp_
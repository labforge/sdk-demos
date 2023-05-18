#include "util.hpp"
#include "gev/util.hpp"
#include <QNetworkInterface>
#include <PvSystem.h>
#include <PvStreamGEV.h>
#include <opencv2/opencv.hpp>
#include <QThread>

using namespace std;
using namespace cv;
using namespace labforge::gev;

Mat* labforge::test::AcquireImages(PvDevice *aDevice, PvStream *aStream, std::vector<std::variant<chunk_dnn_t, chunk_features_t, chunk_descriptors_t>> &chunks ) {

  // Get device parameters need to control streaming
  PvGenParameterArray *lDeviceParams = aDevice->GetParameters();

  // Map the GenICam AcquisitionStart and AcquisitionStop commands
  PvGenCommand *lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
  PvGenCommand *lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );

  // Get stream parameters
  PvGenParameterArray *lStreamParams = aStream->GetParameters();

  // Map a few GenICam stream stats counters
  PvGenFloat *lFrameRate = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "AcquisitionRate" ) );
  PvGenFloat *lBandwidth = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "Bandwidth" ) );

  // Enable streaming and send the AcquisitionStart command
  aDevice->StreamEnable();
  lStart->Execute();

  double lFrameRateVal = 0.0;
  double lBandwidthVal = 0.0;

  // Acquire images until the user instructs us to stop.
  cv::Mat *last_image = nullptr;
  size_t errors = 0;
  while(last_image == nullptr && errors < 10) {
    PvBuffer *lBuffer = nullptr;
    PvResult lOperationResult;

    // Retrieve next buffer
    PvResult lResult = aStream->RetrieveBuffer( &lBuffer, &lOperationResult, 1000 );
    if ( lResult.IsOK() )
    {
      if ( lOperationResult.IsOK() )
      {
        //
        // We now have a valid buffer. This is where you would typically process the buffer.
        // -----------------------------------------------------------------------------------------
        // ...

        lFrameRate->GetValue( lFrameRateVal );
        lBandwidth->GetValue( lBandwidthVal );

        switch ( lBuffer->GetPayloadType() ) {
          case PvPayloadTypeImage:
            if (last_image != nullptr) {
              delete last_image;
            }
            last_image = new cv::Mat(lBuffer->GetImage()->GetHeight(),
                                     lBuffer->GetImage()->GetWidth(),
                                     CV_8UC2,
                                     lBuffer->GetImage()->GetDataPointer());
            if(lBuffer->GetChunkCount() > 1) {
              // Assemble the chunks
              if(lBuffer->GetChunkRawDataByID(CHUNK_DNN_ID) != nullptr) {
                chunk_dnn_t *data = (chunk_dnn_t *)lBuffer->GetChunkRawDataByID(CHUNK_DNN_ID);
                chunks.push_back(*data);
              }
              if(lBuffer->GetChunkRawDataByID(CHUNK_FEATURES_RIGHT) != nullptr) {
                chunk_features_t *data = (chunk_features_t *)lBuffer->GetChunkRawDataByID(CHUNK_FEATURES_RIGHT);
                chunks.push_back(*data);
              }
              if(lBuffer->GetChunkRawDataByID(CHUNK_FEATURES_LEFT) != nullptr) {
                chunk_features_t *data = (chunk_features_t *)lBuffer->GetChunkRawDataByID(CHUNK_FEATURES_LEFT);
                chunks.push_back(*data);
              }
              if(lBuffer->GetChunkRawDataByID(CHUNK_DESCRIPTORS_LEFT) != nullptr) {
                chunk_descriptors_t *data = (chunk_descriptors_t *)lBuffer->GetChunkRawDataByID(CHUNK_DESCRIPTORS_LEFT);
                chunks.push_back(*data);
              }
              if(lBuffer->GetChunkRawDataByID(CHUNK_DESCRIPTORS_RIGHT) != nullptr) {
                chunk_descriptors_t *data = (chunk_descriptors_t *)lBuffer->GetChunkRawDataByID(CHUNK_DESCRIPTORS_RIGHT);
                chunks.push_back(*data);
              }
            }

            break;
          default:
            cout << "INVALID TYPE RECEIVED" << endl;
            break;
        }
      } else {
        // Non OK operational result
        cout << "OPFAIL: " << lOperationResult.GetCodeString().GetAscii() << "\n";
        QThread::currentThread()->usleep(100*1000);
        errors++;
      }

      // Re-queue the buffer in the stream object
      aStream->QueueBuffer( lBuffer );
    }
    else
    {
      // Retrieve buffer failure
      cout << "BUFFAIL: " << lResult.GetCodeString().GetAscii() << "\n";
      QThread::currentThread()->usleep(100*1000);
      errors++;
    }
  }

  // Tell the device to stop sending images.
  lStop->Execute();

  // Disable streaming on the device
  aDevice->StreamDisable();

  // Abort all buffers from the stream and dequeue
  aStream->AbortQueuedBuffers();
  while ( aStream->GetQueuedBufferCount() > 0 )
  {
    PvBuffer *lBuffer = nullptr;
    PvResult lOperationResult;

    aStream->RetrieveBuffer( &lBuffer, &lOperationResult );
  }
  // Convert image
  if(last_image) {
    cvtColor(*last_image, *last_image, COLOR_YUV2BGR_YUY2);
  }

  return last_image;
}

double labforge::test::GetImageSimilarity(cv::Mat &A, cv::Mat &B) {
  if ( A.rows > 0 && A.rows == B.rows && A.cols > 0 && A.cols == B.cols ) {
    // Calculate the L2 relative error between images.
    double errorL2 = norm( A, B, cv::NORM_L2 );
    // Convert to a reasonable scale, since L2 error is summed across all pixels of the image.
    double similarity = errorL2 / (double)( A.rows * A.cols );
    return similarity;
  }
  else {
    return 100000000.0;  // Return a bad value
  }
}

QString labforge::test::findNIC() {
  foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces()) {
    // Return only the first non-loopback MAC Address
    if (!(netInterface.flags() & QNetworkInterface::IsLoopBack))
      return netInterface.hardwareAddress();
  }
  throw runtime_error("Host does not have a loopback interface");
}
/******************************************************************************
 *  Copyright 2023 Labforge Inc.                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this project except in compliance with the License.        *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************

@file pipeline.hpp Pipeline header file
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#ifndef __PIPELINE_HPP__
#define __PIPELINE_HPP__

#include <opencv2/core.hpp>
#include <PvPipeline.h>
#include <PvDeviceGEV.h>
#include <PvStreamGEV.h>
#include <list>
#include <QThread>
#include <QMutex>
#include <variant>
#include <QQueue>
#include <QString>
#include <vector>
#include "inc/bottlenose_chunk_parser.hpp"
namespace labforge::gev {

  struct BNImageData{
    cv::Mat* left;
    cv::Mat* right;
    uint64_t timestamp;
    int32_t min_disparity;
    pointcloud_t pc;
  };

  class Pipeline : public QThread {
    Q_OBJECT

  public:
    Pipeline(PvStreamGEV *stream_gev, PvDeviceGEV*device_gev, QObject*parent = nullptr);

    virtual ~Pipeline();
    bool Start(bool calibrate, bool stereo);
    void Stop();
    bool IsStarted() { return m_start_flag; }
    size_t GetPairs(std::list<BNImageData> &out);
    void run() override;

  Q_SIGNALS:
    void pairReceived();
    void monoReceived();
    void terminated(bool fatal = false);
    void onError(QString msg);
    void timeout();

  private:
    PvStreamGEV * m_stream;
    PvDeviceGEV * m_device;

    PvGenCommand *m_start;
    PvGenCommand *m_stop;
    PvGenFloat *m_fps;
    PvGenFloat *m_bandwidth;
    PvGenInteger *m_mindisparity;

    std::list<PvBuffer*> m_buffers;
    QQueue<BNImageData> m_images;
    volatile bool m_start_flag;
    QMutex m_image_lock;

    void enterCalibrationMode(bool enable, bool stereo);

  };
}

#endif // __PIPELINE_HPP__
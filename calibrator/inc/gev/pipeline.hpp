/**
 * Helper functions for managing image streams.
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

namespace labforge::gev {

  class Pipeline : public QThread {
    Q_OBJECT

  public:
    Pipeline(PvStreamGEV *stream_gev, PvDeviceGEV*device_gev, QObject*parent = nullptr);

    virtual ~Pipeline();
    bool Start(bool calibrate);
    void Stop();
    bool IsStarted() { return m_start_flag; }
    size_t GetPairs(std::list<std::tuple<cv::Mat*, cv::Mat*>> &out);
    void run() override;

  Q_SIGNALS:
    void pairReceived();
    void monoReceived(bool is_disparity);
    void terminated(bool fatal = false);

  private:
    PvStreamGEV * m_stream;
    PvDeviceGEV * m_device;

    PvGenCommand *m_start;
    PvGenCommand *m_stop;
    PvGenFloat *m_fps;
    PvGenFloat *m_bandwidth;
    PvGenEnum* m_pixformat;
    PvGenBoolean *m_rectify;
    PvGenBoolean *m_undistort;

    bool m_rectify_init;
    bool m_undistort_init;
    PvString m_pixfmt_init;

    std::list<PvBuffer*> m_buffers;
    std::list<std::tuple<cv::Mat*, cv::Mat* >> m_images;    
    volatile bool m_start_flag;
    QMutex m_image_lock;
  };
}

#endif // __PIPELINE_HPP__
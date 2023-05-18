#include <QThread>
#include <QImage>
#include <QString>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#ifndef __IO_DATA_THREAD_HPP__
#define __IO_DATA_THREAD_HPP__

namespace labforge::io {
struct ImageData
{
    QImage left;
    QImage right;
};


class DataThread : public QThread
{
    Q_OBJECT

public:
    DataThread(QObject *parent = nullptr);
    ~DataThread();

    void process(const QImage &left, const QImage &right);
    bool setFolder(QString new_folder);
    void setStereo(bool is_stereo){m_stereo = is_stereo;}

signals:
    void dataReceived();
    void dataProcessed();

protected:
    void run() override;

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    
    QString m_folder;
    QString m_left_subfolder;
    QString m_right_subfolder;
    QString m_disparity_subfolder;
    QQueue<ImageData> m_queue;
    uint64_t m_frame_counter;
    QString m_left_fname;
    QString m_right_fname;
    QString m_disparity_fname;
    bool m_stereo;
    bool m_abort;
};
}

#endif // __IO_DATA_THREAD_HPP__
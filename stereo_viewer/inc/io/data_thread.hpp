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

@file data_thread.hpp Data thread header file
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#include <QThread>
#include <QImage>
#include <QString>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <cstdint>

#ifndef __IO_DATA_THREAD_HPP__
#define __IO_DATA_THREAD_HPP__

namespace labforge::io {
struct ImageData
{
    uint64_t timestamp;
    QImage left;
    QImage right;
    QString format;
};

class DataThread : public QThread
{
    Q_OBJECT

public:
    DataThread(QObject *parent = nullptr);
    ~DataThread();

    void process(uint64_t timestamp, const QImage &left, const QImage &right, QString format);
    bool setFolder(QString new_folder);
    void setStereoDisparity(bool is_stereo, bool is_disparity);   

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
    bool m_disparity;

};
}

#endif // __IO_DATA_THREAD_HPP__
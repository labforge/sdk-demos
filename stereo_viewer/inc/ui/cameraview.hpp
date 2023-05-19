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

@file camera_view.hpp CameraView class definition
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#ifndef __CAMERAVIEW_HPP__
#define __CAMERAVIEW_HPP__

#include <memory>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRubberBand>
#include <QMouseEvent>

namespace labforge::ui {

class CameraView : public QLabel {
  Q_OBJECT
public:
  explicit CameraView(QWidget *parent = nullptr);
  virtual ~CameraView() {};

  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void setImage(QImage &img, bool redraw);
  void addTarget(QRect &pos, QString &label, QColor &color, int width);
  void addFeature(QPoint &pos, QColor &color, int width);
  void reset();
  void redrawPixmap();

private:
  QPoint m_origin;
  QRect m_crop;
  std::unique_ptr<QRubberBand> m_rubberband;
  std::unique_ptr<QPixmap> m_last_frame;
  bool m_scaled;

};

} // namespace labforge::ui

#endif // #define __CAMERAVIEW_HPP__
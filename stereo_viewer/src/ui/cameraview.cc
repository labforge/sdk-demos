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

@file cameraview.cc Camera view widget implementation
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#include <QPainter>
#include <QPen>

#include "ui/cameraview.hpp"

using namespace std;
using namespace labforge::ui;

CameraView::CameraView(QWidget *parent) : QLabel(parent), m_scaled(false) {

}

void CameraView::resizeEvent(QResizeEvent *event) {
  (void)event;
  if(m_last_frame) {
    redrawPixmap();
  } else {
    QLabel::resizeEvent(event);
  }
}

void CameraView::mousePressEvent(QMouseEvent *event) {
  (void)event;

  if(event->button() == Qt::RightButton) {
    // reset scale
    m_scaled = false;
    if(m_rubberband)
      m_rubberband->hide();
    redrawPixmap();
  } else if(!m_scaled && event->button() == Qt::LeftButton) {
    m_origin = event->pos();
    // Create selector
    if(!m_rubberband) {
      m_rubberband = make_unique<QRubberBand>(QRubberBand::Rectangle, this);
      QPalette pal;
      pal.setBrush(QPalette::Highlight, QBrush(Qt::green));
      m_rubberband->setPalette(pal);
    }
    m_rubberband->setGeometry(QRect(m_origin, QSize()));

    m_rubberband->show();
  } else {
    QLabel::mousePressEvent(event);
  }
}

void CameraView::mouseMoveEvent(QMouseEvent *event) {
  if(!m_scaled && m_rubberband) {
    m_rubberband->setGeometry(QRect(m_origin, event->pos()).normalized());
  } else {
    QLabel::mouseMoveEvent(event);
  }
}

void CameraView::mouseReleaseEvent(QMouseEvent *event) {
  if(!m_scaled && event->button() == Qt::LeftButton) {
    if(m_last_frame && m_rubberband) {
      QRectF scaled_pixmap_rect = pixmap()->rect();
      QSize window_size = size();

      // Find start position of pixmap, assuming everything is centered
      scaled_pixmap_rect = QRectF((window_size.width() - scaled_pixmap_rect.width()) / 2.0,
                                  (window_size.height() - scaled_pixmap_rect.height()) / 2.0,
                                  scaled_pixmap_rect.width(),
                                  scaled_pixmap_rect.height());
      QRectF selection = m_rubberband->geometry();
//      cout << "Input: " << selection.x() << ", " << selection.y() << ", " << selection.width() << ", " << selection.height() << endl;

      // Make sure we do not cross boundaries
      selection = scaled_pixmap_rect.intersected(selection);
//      cout << "InputS: " << selection.x() << ", " << selection.y() << ", " << selection.width() << ", " << selection.height() << endl;

      // Find origin coordinate in scaled pixmap
      selection = QRectF(selection.x() - scaled_pixmap_rect.x(),
                         selection.y() - scaled_pixmap_rect.y(),
                         selection.width(),
                         selection.height());

      double scale = m_last_frame->width() / scaled_pixmap_rect.width();
//      cout << "Scale: " << scale << endl;
      m_crop = QRect(selection.x()*scale,
                     selection.y()*scale,
                     selection.width()*scale,
                     selection.height()*scale);
//      if(m_crop.width()*m_crop.height() <= 0) {
//        QLabel::mouseReleaseEvent(event);
//        return;
//      }
//
//      cout << "Result" << m_crop.x() << ", " << m_crop.y()
//      << ", " << m_crop.width()
//      << ", " << m_crop.height() << endl;
      m_scaled = true;
      redrawPixmap();
    }
    if(m_rubberband)
      m_rubberband->hide();
  } else {
    QLabel::mouseReleaseEvent(event);
  }
}

void CameraView::setImage(QImage &img, bool redraw) {
  m_last_frame = std::make_unique<QPixmap>(QPixmap::fromImage(img));
  if(redraw)
    redrawPixmap();
}

void CameraView::addTarget(QRect &pos, QString &label, QColor &color, int width) {
  if(m_last_frame) {
    // Directly paint bounding box on image
    QPainter paint(m_last_frame.get());
    paint.setPen(QPen(color,width));
    paint.drawRect(pos);
    if(label.length() > 0)
      paint.drawText(pos, Qt::AlignCenter, label);
  }
}

void CameraView::addFeature(QPoint &pos, QColor &color, int width) {
  if(m_last_frame) {
    // Directly paint bounding box on image
    QPainter paint(m_last_frame.get());
    paint.setPen(QPen(color,width));
    paint.drawPoint(pos);
  }
}

void CameraView::reset() {
  clear();
  m_scaled = false;
  m_last_frame.reset();
}

void CameraView::redrawPixmap() {
  if(m_scaled && m_last_frame) {
    setPixmap(m_last_frame->copy(m_crop).scaled(size(),
                                                Qt::KeepAspectRatio,
                                                Qt::FastTransformation));
  } else if(m_last_frame) {
    setPixmap(m_last_frame->scaled(size(),
                                   Qt::KeepAspectRatio,
                                   Qt::FastTransformation));
  }
}
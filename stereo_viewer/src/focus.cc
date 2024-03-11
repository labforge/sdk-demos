/******************************************************************************
 *  Copyright 2024 Labforge Inc.                                              *
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

@file focus.cpp Utility functions for computing focus target value.
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#include "focus.hpp"

#include <iostream>
#include <utility>
#include <QPainter>

using namespace cv;
using namespace std;

Focus::Focus(size_t maxValues, QColor lineColor, size_t lineWidth)
: m_maxValues(maxValues), m_lineColor(std::move(lineColor)), m_lineWidth(lineWidth), m_enabled(false) {
  m_last_values.reserve(maxValues);
}

void Focus::enable(bool enable) {
  m_enabled = enable;
  // Reset values every time this gets touched
  m_last_values.clear();
}

void Focus::process(QPixmap &pixmap) {
  if(m_enabled) {
    QImage image = pixmap.toImage();
    Mat mat = to_mat(image);
    if(!mat.empty()) {
      Mat gray;
      cvtColor(mat, gray, COLOR_BGR2GRAY);
      double value = focusValue(gray);
      double brightness = averageBrightness(gray);
      m_last_values.push_back(value);
      // Overflow
      if(m_last_values.size() > m_maxValues) {
        m_last_values.erase(m_last_values.begin());
      }
      // Front fill
      if(m_last_values.size() < m_maxValues) {
        m_last_values.insert(m_last_values.begin(), m_maxValues - m_last_values.size(), value);
      }
      // Draw focus helper
      paint(pixmap, brightness);
    }
  }
}

cv::Mat Focus::to_mat(const QImage &image) {
  Mat mat;
  switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
      mat = Mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
      break;
    default:
      return mat;
  }
  return mat;
}

double Focus::focusValue(const cv::Mat &gray) {
  Mat lap;
  Laplacian(gray, lap, CV_64F);
  Scalar mu, sigma;
  meanStdDev(lap, mu, sigma);
  return sigma.val[0] * sigma.val[0];
}

double Focus::averageBrightness(const cv::Mat& gray) {
    cv::Scalar meanBrightness = cv::mean(gray);
    return meanBrightness[0];
}

void Focus::paint(QPixmap &img, double brightness) {
  QPainter paint(&img);
  paint.setPen(QPen(m_lineColor, m_lineWidth));

  // Determine the range of the data
  double minValue = *std::min_element(m_last_values.begin(), m_last_values.end());
  double maxValue = *std::max_element(m_last_values.begin(), m_last_values.end());
  double range = maxValue - minValue;
  int width = img.width();
  int height = img.height();

  // Plot each point
  for (std::size_t i = 0; i < m_last_values.size() - 1; ++i) {
    double normalizedStart = (m_last_values[i] - minValue) / range;
    double normalizedEnd = (m_last_values[i + 1] - minValue) / range;
    int x1 = (i * width) / m_last_values.size();
    int y1 = height - (normalizedStart * height);
    int x2 = ((i + 1) * width) / m_last_values.size();
    int y2 = height - (normalizedEnd * height);

    paint.drawLine(x1, y1, x2, y2);
  }

  // Set up the painter for drawing text
  QFont font = paint.font();
  font.setPixelSize(12); // Adjust the size as needed
  paint.setFont(font);
  paint.setPen(Qt::red); // Choose a color that contrasts well with your image background

  // Create the text to display
  QString brightnessText = QString("Brightness: %1").arg(brightness, 3, 'f', 0, ' ');

  // Calculate the position to draw the text
  int textPadding = 5; // Padding from the top-right corner
  QFontMetrics fontMetrics(font);
  int textWidth = fontMetrics.width(brightnessText);
  int textX = width - textWidth - textPadding; // Adjust X position based on text width
  int textY = textPadding + fontMetrics.ascent(); // Adjust Y position to align text properly

  // Draw the text on the image
  paint.drawText(textX, textY, brightnessText);
  if (!m_last_values.empty()) {
    double lastValue = m_last_values.back();
    QString lastValueText = QString("Sharpness: %1").arg(lastValue, 6, 'f', 0, ' ');
    textX = textPadding; // X position is just the padding amount
    textY = textPadding + fontMetrics.ascent(); // Adjust Y position to align text properly

    // Draw the text on the image
    paint.drawText(textX, textY, lastValueText);
  }
}
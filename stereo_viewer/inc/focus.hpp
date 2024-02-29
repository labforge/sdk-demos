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

@file focus.hpp Utility functions for computing focus target value.
@author Thomas Reidemeister <thomas@labforge.ca>
*/
#ifndef __FOCUS_HPP__
#define __FOCUS_HPP__

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <QImage>
#include <QPixmap>

/**
 * Focus class. This class is used to compute the focus value of an image.
 */
class Focus {
public:
  /**
   * Constructor
   * @param maxValues Maximum number of focus values to display in image stream.
   * @param lineColor Line color of focus annotation to use.
   * @param lineWidth Line with of focus annotation to use.
   */
  explicit Focus(size_t maxValues = 100, QColor lineColor = Qt::green, size_t lineWidth = 3);
  ~Focus() = default;
  void enable(bool enable);
  void process(QPixmap &pixmap);
private:
  static cv::Mat to_mat(const QImage &image);
  static double focusValue(const cv::Mat &gray);
  static double averageBrightness(const cv::Mat& gray);
  void paint(QPixmap &img, double brightness);
  size_t m_maxValues;
  QColor m_lineColor;
  size_t m_lineWidth;
  bool m_enabled;
  std::vector<double> m_last_values;
};

#endif // __FOCUS_HPP__
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

using namespace cv;
using namespace std;

double focusValue(const QPixmap &pixmap) {
  // Convert QPixmap to QImage
  QImage qimage = pixmap.toImage();

  // Convert QImage to Mat
  Mat mat;
  switch (qimage.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
      mat = Mat(qimage.height(), qimage.width(), CV_8UC4, const_cast<uchar*>(qimage.bits()), qimage.bytesPerLine());
      break;
    default:
      return -1.0;
  }

  return 0.0;
}
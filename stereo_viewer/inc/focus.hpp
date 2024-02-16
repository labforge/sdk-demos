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

double focusValue(const QPixmap &pixmap);

#endif // __FOCUS_HPP__
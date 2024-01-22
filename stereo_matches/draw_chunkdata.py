# coding: utf-8
"""
******************************************************************************
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
"""
__author__ = "G. M. Tchamgoue <martin@labforge.ca>"
__copyright__ = "Copyright 2023, Labforge Inc."

import cv2


def draw_keypoints(image, keypoints: dict = None):
    if keypoints is None:
        return

    # convert to opencv KeyPoint datatype
    kp = [cv2.KeyPoint(x=point.x, y=point.y, size=15) for point in keypoints['data']]
    image = cv2.drawKeypoints(image, kp, 0, flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
    return image

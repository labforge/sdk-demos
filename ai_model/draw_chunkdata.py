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
__author__ = "Thomas Reidemeister <thomas@labforge.ca>"
__copyright__ = "Copyright 2024, Labforge Inc."

import cv2


def draw_bounding_boxes(image, detections, color=(0, 255, 0), thickness=2):
    """
    Draw bounding boxes on an image
    :param image: opencv2 image
    :param detections: List of detections received from Bottlenose
    :param color: Annotation color
    :param thickness: Border thickness
    :return:
    """
    image_height, image_width = image.shape[:2]
    for detection in detections:
        # Sanitize box coordinates
        left = max(0, min(int(detection.left), image_width - 1))
        top = max(0, min(int(detection.top), image_height - 1))
        right = max(0, min(int(detection.right), image_width - 1))
        bottom = max(0, min(int(detection.bottom), image_height - 1))

        # Draw the bounding box
        cv2.rectangle(image, (left, top), (right, bottom), color, thickness)

        # Prepare the text for the label and score
        text = f"{detection.label}: {detection.score:.2f}"

        # Get the width and height of the text box
        (text_width, text_height), baseline = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)

        # Draw a filled rectangle to put the text in
        cv2.rectangle(image, (left, top - text_height - baseline),
                      (left + text_width, top), (0, 255, 0), -1)

        # Put the text in the rectangle
        cv2.putText(image, text, (left, top - baseline), cv2.FONT_HERSHEY_SIMPLEX,
                    0.5, (0, 0, 0), 1)

    return image

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

def draw_matches(image1: np.ndarray, image2: np.ndarray, keypoints1: dict, keypoints2: dict, matches):
    """
    decode the input buffer as matched keypoints.
    the layout tells the structure of the data
    [0,1] = point16, [2,3] point16x8
    0: COORDINATE_ONLY, 1: INDEX_ONLY
    2: COORDINATE_DETAILED, 3: INDEX_DETAILED
    """

    def find_index(point, keypoints: dict, mtype: int, bad_value=65535):    
        if((mtype == 0) or (mtype == 2)):
            for idx, kp in enumerate(keypoints['data']):
                if point.x == kp.x and point.y == kp.y:
                    return idx, True
        elif((mtype == 1) or (mtype == 3)):
            return point.x, point.x != bad_value

        return 0, False


    if keypoints1 is None or keypoints2 is None or matches is None:
        return
    
    matching_indexes = []
    for idx in range(len(keypoints1['data'])):
        matched_point = matches.points[idx]

        mindex, found = find_index(point=matched_point, keypoints=keypoints2, mtype=matches.layout, bad_value=matches.unmatched)
        if found:
            matching_indexes.append((idx, mindex))
            #print('Index found {0} at {1}'.format(match_point,mindex))
    # print(matching_indexes)

    kp1 = [cv2.KeyPoint(x=pt.x, y=pt.y, size=1) for pt in keypoints1['data']]
    kp2 = [cv2.KeyPoint(x=pt[0], y=pt[1], size=1) for pt in keypoints2['data']]

    good = [cv2.DMatch(_imgIdx=0, _queryIdx=sidx, _trainIdx=midx, _distance=0)
            for (sidx, midx) in matching_indexes]
    results = np.array([])
    results = cv2.drawMatches(image1, kp1, image2, kp2, matches1to2=good,
                              outImg=results, flags=cv2.DRAW_MATCHES_FLAGS_NOT_DRAW_SINGLE_POINTS)

    #save_file = save_file.replace(
    #    '_matches', '_matches_' + left_corners['type'].lower())
    #cv2.imwrite(save_file, results)
    return results

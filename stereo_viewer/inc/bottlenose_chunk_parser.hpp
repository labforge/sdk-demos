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

@file bottlenose_chunk_parser.hpp Parse chunk data.
@author G. M. Tchamgoue <martin@labforge.ca>, Thomas Reidemeister <thomas@labforge.ca>
*/
#ifndef __BOTTLENOSE_CHUNK_PARSER_HPP__
#define __BOTTLENOSE_CHUNK_PARSER_HPP__

#include <PvBuffer.h>
#include <vector>

#if defined(__GNUC__) // GCC and Clang
#define PACKED_STRUCT_BEGIN() struct __attribute__((packed, aligned(4)))
#define PACKED_STRUCT_END()
#elif defined(_MSC_VER) // MSVC
#define PACKED_STRUCT_BEGIN() __pragma(pack(push, 1)) struct
#define PACKED_STRUCT_END() __pragma(pack(pop))
#else
#error Unsupported compiler
#endif

#define MAX_KEYPOINTS 0xFFFF

/**
 * @brief ChunkIDs for possible buffers appended to the GEV buffer.
 */
typedef enum {
  CHUNK_ID_FEATURES = 0x4001,    ///< Keypoints
  CHUNK_ID_DESCRIPTORS = 0x4002, ///< Descriptors
  CHUNK_ID_DNNBBOXES = 0x4003,   ///< Bounding boxes for detected targets
  CHUNK_ID_EMBEDDINGS = 0x4004,  ///< Embeddings
  CHUNK_ID_INFO = 0x4005,        ///< Meta information
  CHUNK_ID_MATCHES = 0x4006,     ///< Matching
  CHUNK_ID_POINTCLOUD = 0x4007   ///< Sparse Point Cloud
} chunk_type_t;

/**
 * @brief Meta information chunk data to decode timestamps.
 */
typedef PACKED_STRUCT_BEGIN() {
  uint64_t real_time; ///< Real time in milliseconds since epoch
  uint32_t count;     ///< Frame counter
  float gain;         ///< Gain value
  float exposure;     ///< Exposure value
} PACKED_STRUCT_END() info_t;


typedef struct vector3f{
  float x;
  float y;
  float z;
} vector3f_t;

typedef std::vector<vector3f_t> pointcloud_t;

/**
 * Decode meta information from buffer, if present.
 * @param buffer Buffer received on GEV interface
 * @param info Meta information
 * @return
 */
bool chunkDecodeMetaInformation(PvBuffer *buffer, info_t *info);

std::string ms_to_date_string(uint64_t ms);

bool chunkDecodePointCloud(PvBuffer *buffer, std::vector<vector3f_t>&pointcloud);

#endif // __BOTTLENOSE_CHUNK_PARSER_HPP__
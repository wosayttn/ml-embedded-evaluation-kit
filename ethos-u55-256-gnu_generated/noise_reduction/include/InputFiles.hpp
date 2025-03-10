/*
 * SPDX-FileCopyrightText: Copyright 2024, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*********************    Autogenerated file. DO NOT EDIT *******************
 * Generated from gen_audio_cpp.py tool file.
 * Date: 2024-09-26 10:06:58.271415
 ***************************************************************************/

#ifndef GENERATED_AUDIOCLIPS_H
#define GENERATED_AUDIOCLIPS_H

#include <cstdint>
#include <stddef.h>

#define NUMBER_OF_FILES (3U)

extern const int16_t audio0[65646];
extern const int16_t audio1[64757];
extern const int16_t audio2[63058];

/**
 * @brief       Gets the filename for the baked-in input array
 * @param[in]   idx     Index of the input.
 * @return      const C string pointer to the name.
 **/
const char* GetFilename(const uint32_t idx);

/**
 * @brief       Gets the pointer to audio data.
 * @param[in]   idx     Index of the input.
 * @return      Pointer to 16-bit signed integer data.
 **/
const int16_t* GetAudioArray(const uint32_t idx);

/**
 * @brief       Gets the size of the input array.
 * @param[in]   idx     Index of the input.
 * @return      Size of the input array in bytes.
 **/
uint32_t GetAudioArraySize(const uint32_t idx);

#endif /* GENERATED_AUDIOCLIPS_H */
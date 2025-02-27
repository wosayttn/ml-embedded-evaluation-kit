/*
 * SPDX-FileCopyrightText: Copyright 2021-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
#ifndef POSTER_HELPER_H
#define POSTER_HELPER_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint8_t *pu8SrcImgBuf;
    uint32_t u32ImgWidth;
    uint32_t u32ImgHeight;
    uint32_t u32NumComponents;
    uint32_t u32Quality;
} S_JPEG_CONTEXT;

int poster_mqtt(S_JPEG_CONTEXT *psJpegECtx);

#endif /* POSTER_HELPER_H */

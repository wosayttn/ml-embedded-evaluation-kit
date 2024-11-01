/*
 * SPDX-FileCopyrightText: Copyright 2022-2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
#include "audio_helper.h"

#include "log_macros.h"
#include "rtthread.h"
#include "rtdevice.h"

//#undef DBG_ENABLE
#define DBG_LEVEL LOG_LVL_INFO
#define DBG_SECTION_NAME  "ml.audio"
#define DBG_COLOR
#include <rtdbg.h>

#if !defined(MLEVK_UC_AUDIO_CAPTURE_DEVICE)
    #define MLEVK_UC_AUDIO_CAPTURE_DEVICE  "dmic0"
#endif

static rt_device_t s_psAudioDev = RT_NULL;

void audio_capture_fini(void)
{
    if (s_psAudioDev)
    {
        rt_device_close(s_psAudioDev);
        s_psAudioDev = RT_NULL;
    }
}

int audio_capture_init(uint32_t u32SmplRate, uint32_t u32SmplBit, uint32_t u32ChnNum)
{
    rt_err_t result;
    struct rt_audio_caps caps;

    LOG_I("Information:");
    LOG_I("Sample rate %d", u32SmplRate);
    LOG_I("Channels %d", u32ChnNum);
    LOG_I("Sample bits %d", u32SmplBit);

    s_psAudioDev = rt_device_find(MLEVK_UC_AUDIO_CAPTURE_DEVICE);
    if (s_psAudioDev == RT_NULL)
    {
        LOG_E("device %s not found", MLEVK_UC_AUDIO_CAPTURE_DEVICE);
        goto exit_audio_capture_init;
    }

    /* open micphone device */
    result = rt_device_open(s_psAudioDev, RT_DEVICE_OFLAG_RDONLY);
    if (result != RT_EOK)
    {
        LOG_E("open %s device failed", MLEVK_UC_AUDIO_CAPTURE_DEVICE);
        goto exit_audio_capture_init;
    }

    caps.main_type = AUDIO_TYPE_INPUT;
    caps.sub_type  = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = u32SmplRate;
    caps.udata.config.channels = u32ChnNum;
    caps.udata.config.samplebits = u32SmplBit;
    result = rt_device_control(s_psAudioDev, AUDIO_CTL_CONFIGURE, &caps);
    if (result != RT_EOK)
    {
        rt_device_close(s_psAudioDev);
        LOG_E("set %s device failed", MLEVK_UC_AUDIO_CAPTURE_DEVICE);
        goto exit_audio_capture_init;
    }

    return 0;

exit_audio_capture_init:

    s_psAudioDev = RT_NULL;

    return -1;
}

uint32_t audio_capture_get_frame(uint8_t *pu8BufAddr, uint32_t u32BufLen)
{
    /* Read raw data from capture audio device. */
    if (s_psAudioDev && pu8BufAddr && (u32BufLen > 0))
    {
        uint32_t size = 0;
        uint32_t u32Off = 0;
        uint32_t u32Remain = u32BufLen;

        while (u32Remain > 0)
        {
            size = rt_device_read(s_psAudioDev, 0, pu8BufAddr + u32Off, (u32Remain > 4096) ? 4096 : u32Remain);
            u32Remain -= size;
            u32Off += size;

            //LOG_I("(%d/%d)", u32Remain, u32BufLen);
            if (u32Remain)
                rt_thread_mdelay(100);
        }

        return u32BufLen;
    }

    return 0;
}

uint32_t audio_transcode_pcm16to8(uint8_t *pu8BufAddr, uint32_t u32BufLen)
{
    int len = u32BufLen / sizeof(uint16_t);
    uint16_t *pu16 = (uint16_t *)pu8BufAddr;
    uint8_t *pu8 = pu8BufAddr;

    while (len > 0)
    {
        *pu8 = (uint8_t)(*pu16 >> 8);

        len--;
        pu8++;
        pu16++;
    }

    return u32BufLen / sizeof(uint16_t);
}

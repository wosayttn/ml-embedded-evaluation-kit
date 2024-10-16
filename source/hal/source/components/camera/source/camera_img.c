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
#include "camera_img.h"

#include "log_macros.h"
#include "rtthread.h"
#include "drv_ccap.h"

//#undef DBG_ENABLE
#define DBG_LEVEL LOG_LVL_INFO
#define DBG_SECTION_NAME  "ml.cam"
#define DBG_COLOR
#include <rtdbg.h>

#define THREAD_PRIORITY   5
#define THREAD_STACK_SIZE 4096
#define THREAD_TIMESLICE  5

//#define DEF_CROP_PACKET_RECT
//#define DEF_SWITCH_BASE_ADDR_ISR
//#define DEF_FRAMERATE_DIV2

#define DEF_DURATION            10
#define DEF_ENABLE_PLANAR_PIPE  0
#define DEF_ONE_SHOT            1

#define DEF_PACKET_WIDTH        192
#define DEF_PACKET_HEIGHT       192
#define DEF_PLANAR_WIDTH        192
#define DEF_PLANAR_HEIGHT       192

typedef struct
{
    char *thread_name;
    char *devname_ccap;
    char *devname_sensor;
    uint32_t thread_running;

    ccap_view_info_t ccap_viewinfo_planar;
    ccap_view_info_t ccap_viewinfo_packet;
} ccap_grabber_param;
typedef ccap_grabber_param *ccap_grabber_param_t;

typedef struct
{
    ccap_config    sCcapConfig;
    uint32_t       u32FrameEnd;
    uint32_t       u32ISRFrameEnd;
    rt_sem_t       semFrameEnd;
    rt_device_t    psDevCcap;
} ccap_grabber_context;
typedef ccap_grabber_context *ccap_grabber_context_t;

static ccap_grabber_param s_GrabberParam =
{
    .thread_name = "grab0",
    .devname_ccap = "ccap0",
    .devname_sensor = "sensor0",
    .thread_running = 1
};

static void nu_ccap_event_hook(void *pvData, uint32_t u32EvtMask)
{
    ccap_grabber_context_t psGrabberContext = (ccap_grabber_context_t)pvData;

    if (u32EvtMask & NU_CCAP_FRAME_END)
    {
#if defined(DEF_SWITCH_BASE_ADDR_ISR)
        switch_framebuffers(psGrabberContext);
#endif
        psGrabberContext->u32ISRFrameEnd++;
        rt_sem_release(psGrabberContext->semFrameEnd);
    }

    if (u32EvtMask & NU_CCAP_ADDRESS_MATCH)
    {
        LOG_I("Address matched");
    }

    if (u32EvtMask & NU_CCAP_MEMORY_ERROR)
    {
        LOG_E("Access memory error");
    }
}

static rt_device_t ccap_sensor_init(ccap_grabber_context_t psGrabberContext, ccap_grabber_param_t psGrabberParam)
{
    rt_err_t ret;
    ccap_view_info_t psViewInfo;
    sensor_mode_info *psSensorModeInfo;
    rt_device_t psDevSensor = RT_NULL;
    rt_device_t psDevCcap = RT_NULL;
    ccap_config_t    psCcapConfig = &psGrabberContext->sCcapConfig;

    psDevCcap = rt_device_find(psGrabberParam->devname_ccap);
    if (psDevCcap == RT_NULL)
    {
        LOG_E("Can't find %s", psGrabberParam->devname_ccap);
        goto exit_ccap_sensor_init;
    }

    psDevSensor = rt_device_find(psGrabberParam->devname_sensor);
    if (psDevSensor == RT_NULL)
    {
        LOG_E("Can't find %s", psGrabberParam->devname_sensor);
        goto exit_ccap_sensor_init;
    }

//    psCcapConfig->sPipeInfo_Packet.pu8FarmAddr = rt_malloc_align(RT_ALIGN(DEF_PACKET_WIDTH * DEF_PACKET_HEIGHT * 2, 32), 32);
//    psCcapConfig->sPipeInfo_Packet.u32PixFmt   = CCAP_PAR_OUTFMT_RGB565;

    psCcapConfig->sPipeInfo_Packet.pu8FarmAddr = rt_malloc_align(RT_ALIGN(DEF_PACKET_WIDTH * DEF_PACKET_HEIGHT, 32), 32);
    psCcapConfig->sPipeInfo_Packet.u32PixFmt   = CCAP_PAR_OUTFMT_ONLY_Y;

    psCcapConfig->sPipeInfo_Packet.u32Width    = DEF_PACKET_WIDTH;
    psCcapConfig->sPipeInfo_Packet.u32Height   = DEF_PACKET_HEIGHT;
    psCcapConfig->u32Stride_Packet             = DEF_PACKET_WIDTH;

    if (psCcapConfig->sPipeInfo_Packet.pu8FarmAddr == RT_NULL)
    {
        psCcapConfig->sPipeInfo_Packet.u32Height = 0;
        psCcapConfig->sPipeInfo_Packet.u32Width  = 0;
        psCcapConfig->sPipeInfo_Packet.u32PixFmt = 0;
        psCcapConfig->u32Stride_Packet           = 0;
    }
    LOG_I("Packet.FarmAddr@0x%08X", psCcapConfig->sPipeInfo_Packet.pu8FarmAddr);
    LOG_I("Packet.FarmWidth: %d", psCcapConfig->sPipeInfo_Packet.u32Width);
    LOG_I("Packet.FarmHeight: %d", psCcapConfig->sPipeInfo_Packet.u32Height);

    /* Planar pipe for encoding */
#if DEF_ENABLE_PLANAR_PIPE
    psCcapConfig->sPipeInfo_Planar.u32Width    = DEF_PLANAR_WIDTH;
    psCcapConfig->sPipeInfo_Planar.u32Height   = DEF_PLANAR_HEIGHT;
    psCcapConfig->sPipeInfo_Planar.pu8FarmAddr = rt_malloc_align(RT_ALIGN(DEF_PLANAR_WIDTH * DEF_PLANAR_HEIGHT * 2, 32), 32);
    psCcapConfig->sPipeInfo_Planar.u32PixFmt   = CCAP_PAR_PLNFMT_YUV420;
    psCcapConfig->u32Stride_Planar             = DEF_PLANAR_WIDTH;

    if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr == RT_NULL)
    {
        psCcapConfig->sPipeInfo_Planar.u32Height = 0;
        psCcapConfig->sPipeInfo_Planar.u32Width  = 0;
        psCcapConfig->sPipeInfo_Planar.u32PixFmt = 0;
        psCcapConfig->u32Stride_Planar           = 0;
    }

    LOG_I("Planar.FarmAddr@0x%08X", psCcapConfig->sPipeInfo_Planar.pu8FarmAddr);
    LOG_I("Planar.FarmWidth: %d", psCcapConfig->sPipeInfo_Planar.u32Width);
    LOG_I("Planar.FarmHeight: %d", psCcapConfig->sPipeInfo_Planar.u32Height);
#endif

    /* open CCAP */
    ret = rt_device_open(psDevCcap, 0);
    if (ret != RT_EOK)
    {
        LOG_E("Can't open %s", psGrabberParam->devname_ccap);
        goto exit_ccap_sensor_init;
    }

    /* Find suit mode for packet pipe */
    if (psCcapConfig->sPipeInfo_Packet.pu8FarmAddr != RT_NULL)
    {
        /* Check view window of packet pipe */
        psViewInfo = &psCcapConfig->sPipeInfo_Packet;

        if ((rt_device_control(psDevSensor, CCAP_SENSOR_CMD_GET_SUIT_MODE, (void *)&psViewInfo) != RT_EOK)
                || (psViewInfo == RT_NULL))
        {
            LOG_E("Can't get suit mode for packet.");
            goto fail_ccap_init;
        }
    }

    /* Find suit mode for planner pipe */
    if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr != RT_NULL)
    {
        int recheck = 1;

        if (psViewInfo != RT_NULL)
        {
            if ((psCcapConfig->sPipeInfo_Planar.u32Width <= psViewInfo->u32Width) ||
                    (psCcapConfig->sPipeInfo_Planar.u32Height <= psViewInfo->u32Height))
                recheck = 0;
        }

        if (recheck)
        {
            /* Check view window of planner pipe */
            psViewInfo = &psCcapConfig->sPipeInfo_Planar;

            /* Find suit mode */
            if ((rt_device_control(psDevSensor, CCAP_SENSOR_CMD_GET_SUIT_MODE, (void *)&psViewInfo) != RT_EOK)
                    || (psViewInfo == RT_NULL))
            {
                LOG_E("Can't get suit mode for planner.");
                goto exit_ccap_sensor_init;
            }
        }
    }

#if defined(DEF_CROP_PACKET_RECT)
    /* Set cropping rectangle */
    if (psViewInfo->u32Width >= psCcapConfig->sPipeInfo_Packet.u32Width)
    {
        /* sensor.width >= preview.width */
        psCcapConfig->sRectCropping.x = (psViewInfo->u32Width - psCcapConfig->sPipeInfo_Packet.u32Width) / 2;
        psCcapConfig->sRectCropping.width  = psCcapConfig->sPipeInfo_Packet.u32Width;
    }
    else
    {
        /* sensor.width < preview.width */
        psCcapConfig->sRectCropping.x = 0;
        psCcapConfig->sRectCropping.width  = psViewInfo->u32Width;
    }

    if (psViewInfo->u32Height >= psCcapConfig->sPipeInfo_Packet.u32Height)
    {
        /* sensor.height >= preview.height */
        psCcapConfig->sRectCropping.y = (psViewInfo->u32Height - psCcapConfig->sPipeInfo_Packet.u32Height) / 2;
        psCcapConfig->sRectCropping.height = psCcapConfig->sPipeInfo_Packet.u32Height;
    }
    else
    {
        /* sensor.height < preview.height */
        psCcapConfig->sRectCropping.y = 0;
        psCcapConfig->sRectCropping.height = psViewInfo->u32Height;
    }
#else
    /* Set cropping rectangle */
    psCcapConfig->sRectCropping.x      = 0;
    psCcapConfig->sRectCropping.y      = 0;
    psCcapConfig->sRectCropping.width  = psViewInfo->u32Width;
    psCcapConfig->sRectCropping.height = psViewInfo->u32Height;
#endif

    /* ISR Hook */
    psCcapConfig->pfnEvHndler = nu_ccap_event_hook;
    psCcapConfig->pvData      = (void *)psGrabberContext;

    /* Get Suitable mode. */
    psSensorModeInfo = (sensor_mode_info *)psViewInfo;

    /* Feed CCAP configuration */
    ret = rt_device_control(psDevCcap, CCAP_CMD_CONFIG, (void *)psCcapConfig);
    if (ret != RT_EOK)
    {
        LOG_E("Can't feed configuration %s", psGrabberParam->devname_ccap);
        goto fail_ccap_init;
    }

    {
        int i32SenClk = psSensorModeInfo->u32SenClk;
        if ((i32SenClk > 45000000) && DEF_ENABLE_PLANAR_PIPE)
            i32SenClk = 45000000; /* Bandwidth limitation: Slow down sensor clock */

        /* speed up pixel clock */
        if (rt_device_control(psDevCcap, CCAP_CMD_SET_SENCLK, (void *)&i32SenClk) != RT_EOK)
        {
            LOG_E("Can't feed setting.");
            goto fail_ccap_init;
        }
    }

    /* Initial CCAP sensor */
    if (rt_device_open(psDevSensor, 0) != RT_EOK)
    {
        LOG_E("Can't open sensor.");
        goto fail_sensor_init;
    }

    /* Feed settings to sensor */
    if (rt_device_control(psDevSensor, CCAP_SENSOR_CMD_SET_MODE, (void *)psSensorModeInfo) != RT_EOK)
    {
        LOG_E("Can't feed setting.");
        goto fail_sensor_init;
    }

    ret = rt_device_control(psDevCcap, CCAP_CMD_SET_PIPES, (void *)psViewInfo);
    if (ret != RT_EOK)
    {
        LOG_E("Can't set pipes %s", psGrabberParam->devname_ccap);
        goto fail_ccap_init;
    }

    return psDevCcap;

fail_sensor_init:

    if (psDevSensor)
        rt_device_close(psDevSensor);

fail_ccap_init:

    if (psDevCcap)
        rt_device_close(psDevCcap);

exit_ccap_sensor_init:

    psDevCcap = psDevSensor = RT_NULL;

    return psDevCcap;
}

static void ccap_sensor_fini(rt_device_t psDevCcap, rt_device_t psDevSensor)
{
    if (psDevSensor)
        rt_device_close(psDevSensor);

    if (psDevCcap)
        rt_device_close(psDevCcap);
}

static ccap_grabber_context sGrabberContext;
static void ccap_grabber(void *parameter)
{
    rt_err_t ret;
    ccap_grabber_param_t psGrabberParam = (ccap_grabber_param_t)parameter;

    rt_device_t psDevCcap = RT_NULL;
    rt_device_t psDevLcd = RT_NULL;

    rt_tick_t last, now;
    rt_bool_t bDrawDirect;

    rt_memset((void *)&sGrabberContext, 0, sizeof(ccap_grabber_context));

    sGrabberContext.semFrameEnd = rt_sem_create(psGrabberParam->devname_ccap, 0, RT_IPC_FLAG_FIFO);
    if (sGrabberContext.semFrameEnd == RT_NULL)
    {
        LOG_E("Can't allocate sem resource %s", psGrabberParam->devname_ccap);
        goto exit_ccap_grabber;
    }

    /* initial ccap & sensor*/
    psDevCcap = ccap_sensor_init(&sGrabberContext, psGrabberParam);
    if (psDevCcap == RT_NULL)
    {
        LOG_E("Can't init %s and %s", psGrabberParam->devname_ccap, psGrabberParam->devname_sensor);
        goto exit_ccap_grabber;
    }

#if defined(DEF_FRAMERATE_DIV2)
    /* Set frame rate / 2 */
    if (rt_device_control(psDevCcap, CCAP_CMD_SET_FRAMERATE_NM, (void *)(1 << 16 | 2)) != RT_EOK)
    {
        LOG_W("Can't set frame rate ", psGrabberParam->devname_ccap);
    }
#endif

    sGrabberContext.psDevCcap = psDevCcap;

    /* Start to capture */
    if (rt_device_control(psDevCcap, CCAP_CMD_START_CAPTURE, RT_NULL) != RT_EOK)
    {
        LOG_E("Can't start %s", psGrabberParam->devname_ccap);
        goto exit_ccap_grabber;
    }

    psGrabberParam->ccap_viewinfo_packet = &sGrabberContext.sCcapConfig.sPipeInfo_Packet;

#if DEF_ONE_SHOT
    /* Set one-shot mode */
    uint32_t u32OneShot = 1;
    if (rt_device_control(psDevCcap, CCAP_CMD_SET_OPMODE, &u32OneShot) != RT_EOK)
    {
        LOG_E("Can't set one-shot mode %s", psGrabberParam->devname_ccap);
        goto exit_ccap_grabber;
    }
#endif

    psGrabberParam->thread_running = 1;
    last = now = rt_tick_get();
    while (psGrabberParam->thread_running)
    {
#if DEF_ONE_SHOT
        rt_thread_mdelay(DEF_DURATION * 1000);
#else
        if (sGrabberContext.semFrameEnd)
        {
            rt_sem_take(sGrabberContext.semFrameEnd, RT_WAITING_FOREVER);
        }
        sGrabberContext.u32FrameEnd++;
#endif

        /* FPS */
        now = rt_tick_get();
        if ((now - last) >= (DEF_DURATION * 1000))
        {
            uint32_t u32CurrFPS = sGrabberContext.u32FrameEnd / DEF_DURATION;

            LOG_I("%s: %d FPS", psGrabberParam->devname_ccap, u32CurrFPS);
            sGrabberContext.u32FrameEnd = 0;
            last = now;
        }
    }

exit_ccap_grabber:

    ccap_sensor_fini(rt_device_find(psGrabberParam->devname_ccap), rt_device_find(psGrabberParam->devname_sensor));

    ccap_config_t    psCcapConfig = &sGrabberContext.sCcapConfig;

    if (psCcapConfig->sPipeInfo_Packet.pu8FarmAddr)
    {
        rt_free_align(psCcapConfig->sPipeInfo_Packet.pu8FarmAddr);
        psCcapConfig->sPipeInfo_Packet.pu8FarmAddr = RT_NULL;
    }

#if DEF_ENABLE_PLANAR_PIPE
    if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr)
    {
        rt_free_align(psCcapConfig->sPipeInfo_Planar.pu8FarmAddr);
        psCcapConfig->sPipeInfo_Planar.pu8FarmAddr = RT_NULL;
    }
#endif

    return;
}

int camera_fini(void)
{
    rt_thread_t ccap_thread = rt_thread_find(s_GrabberParam.thread_name);
    if (ccap_thread != RT_NULL)
    {
        s_GrabberParam.thread_running = 0;
    }
}

int camera_init(void)
{
    rt_thread_t ccap_thread = rt_thread_find(s_GrabberParam.thread_name);
    if (ccap_thread == RT_NULL)
    {
        ccap_thread = rt_thread_create(s_GrabberParam.thread_name,
                                       ccap_grabber,
                                       &s_GrabberParam,
                                       THREAD_STACK_SIZE,
                                       THREAD_PRIORITY,
                                       THREAD_TIMESLICE);

        if (ccap_thread != RT_NULL)
            rt_thread_startup(ccap_thread);
    }
}

const uint8_t *camera_get_frame(int pipe)
{
    ccap_config_t psCcapConfig = &sGrabberContext.sCcapConfig;
    if (pipe == 0)
    {
        if (psCcapConfig->sPipeInfo_Packet.pu8FarmAddr)
            return (const uint8_t *)psCcapConfig->sPipeInfo_Packet.pu8FarmAddr;
    }
    else if (pipe == 1)
    {
#if DEF_ENABLE_PLANAR_PIPE
        if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr)
            return (const uint8_t *)psCcapConfig->sPipeInfo_Planar.pu8FarmAddr;
#endif
    }

    return NULL;
}

int camera_sync(void)
{
#if DEF_ONE_SHOT
    if (sGrabberContext.semFrameEnd)
    {
        rt_sem_take(sGrabberContext.semFrameEnd, RT_WAITING_FOREVER);
        sGrabberContext.u32FrameEnd++;
    }
#endif

    return 0;
}

int camera_oneshot(void)
{
#if DEF_ONE_SHOT
    int OpModeShutter = 1;
    /* One-shot mode, trigger next frame */
    rt_device_control(sGrabberContext.psDevCcap, CCAP_CMD_SET_OPMODE, &OpModeShutter);
#endif

    return 0;
}

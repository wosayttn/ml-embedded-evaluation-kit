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
#define DEF_ONE_SHOT            1

typedef struct
{
    char *thread_name;
    char *devname_ccap;
    char *devname_sensor;
    uint32_t thread_running;

    ccap_view_info_t psViewInfo_Packet;
    ccap_view_info_t psViewInfo_Planar;
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
    .thread_running = 1,
    .psViewInfo_Packet = RT_NULL,
    .psViewInfo_Planar = RT_NULL,
};

static ccap_grabber_context sGrabberContext;

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

static void ccap_sensor_fini(rt_device_t psDevCcap, rt_device_t psDevSensor)
{
    ccap_config_t psCcapConfig = &sGrabberContext.sCcapConfig;

    if (psDevSensor)
        rt_device_close(psDevSensor);

    if (psDevCcap)
        rt_device_close(psDevCcap);

    if (psCcapConfig->sPipeInfo_Packet.pu8FarmAddr)
    {
        rt_free_align(psCcapConfig->sPipeInfo_Packet.pu8FarmAddr);
        psCcapConfig->sPipeInfo_Packet.pu8FarmAddr = RT_NULL;
    }

    if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr)
    {
        rt_free_align(psCcapConfig->sPipeInfo_Planar.pu8FarmAddr);
        psCcapConfig->sPipeInfo_Planar.pu8FarmAddr = RT_NULL;
    }
}

#define CCAP_PAR_OUTFMT_RGB888_U8       ((1 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x0ul << CCAP_PAR_OUTFMT_Pos))
#define CCAP_PAR_OUTFMT_BGR888_U8       ((0 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x0ul << CCAP_PAR_OUTFMT_Pos))
#define CCAP_PAR_OUTFMT_ARGB888_U8      ((3 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x0ul << CCAP_PAR_OUTFMT_Pos))
#define CCAP_PAR_OUTFMT_BGRA888_U8      ((2 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x0ul << CCAP_PAR_OUTFMT_Pos))
/*!< CCAP PAR/PARM setting for Image Data RGB888 INT8  Format Output to System Memory                \hideinitializer */
#define CCAP_PAR_OUTFMT_RGB888_I8       ((1 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x1ul << CCAP_PAR_OUTFMT_Pos))
#define CCAP_PAR_OUTFMT_BGR888_I8       ((0 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x1ul << CCAP_PAR_OUTFMT_Pos))
#define CCAP_PAR_OUTFMT_ARGB888_I8      ((3 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x1ul << CCAP_PAR_OUTFMT_Pos))
#define CCAP_PAR_OUTFMT_BGRA888_I8      ((2 << CCAP_PARM_RGB888OUTORD_Pos) | CCAP_PARM_OUTFMTH_Msk | (0x1ul << CCAP_PAR_OUTFMT_Pos))

static uint32_t ccap_get_fmt_bpp(uint32_t u32PixFmt)
{
    switch (u32PixFmt)
    {
    case CCAP_PAR_OUTFMT_ONLY_Y:
        return 1;

    case CCAP_PAR_PLNFMT_YUV422:
    //case CCAP_PAR_OUTFMT_YUV422:
    case CCAP_PAR_PLNFMT_YUV420:
    case CCAP_PAR_OUTFMT_RGB555:
    case CCAP_PAR_OUTFMT_RGB565:
        return 2;

    case CCAP_PAR_OUTFMT_BGR888_I8:
    case CCAP_PAR_OUTFMT_RGB888_I8:
    case CCAP_PAR_OUTFMT_BGR888_U8:
    case CCAP_PAR_OUTFMT_RGB888_U8:
        return 3;

    case CCAP_PAR_OUTFMT_ARGB888_I8:
    case CCAP_PAR_OUTFMT_BGRA888_I8:
    case CCAP_PAR_OUTFMT_ARGB888_U8:
    case CCAP_PAR_OUTFMT_BGRA888_U8:
        return 4;

    default:
        return 0;
    }

    return 0;
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

    if (psGrabberParam->psViewInfo_Packet)
    {
        psCcapConfig->sPipeInfo_Packet.u32PixFmt   = psGrabberParam->psViewInfo_Packet->u32PixFmt;
        psCcapConfig->sPipeInfo_Packet.u32Width    = psGrabberParam->psViewInfo_Packet->u32Width;
        psCcapConfig->sPipeInfo_Packet.u32Height   = psGrabberParam->psViewInfo_Packet->u32Height;
        psCcapConfig->u32Stride_Packet             = psGrabberParam->psViewInfo_Packet->u32Width;
        if (psGrabberParam->psViewInfo_Packet->pu8FarmAddr == RT_NULL)
        {
            uint32_t u32Sz = psCcapConfig->sPipeInfo_Packet.u32Width *
                             psCcapConfig->sPipeInfo_Packet.u32Height *
                             ccap_get_fmt_bpp(psCcapConfig->sPipeInfo_Packet.u32PixFmt);

            // Allocate memory.
            psCcapConfig->sPipeInfo_Packet.pu8FarmAddr = rt_malloc_align(RT_ALIGN(u32Sz, 32), 32);
            if (psCcapConfig->sPipeInfo_Packet.pu8FarmAddr == RT_NULL)
            {
                psCcapConfig->sPipeInfo_Packet.u32Height = 0;
                psCcapConfig->sPipeInfo_Packet.u32Width  = 0;
                psCcapConfig->sPipeInfo_Packet.u32PixFmt = 0;
                psCcapConfig->u32Stride_Packet           = 0;
            }
        }

        LOG_I("Packet.FarmAddr@0x%08X", psCcapConfig->sPipeInfo_Packet.pu8FarmAddr);
        LOG_I("Packet.FarmWidth: %d", psCcapConfig->sPipeInfo_Packet.u32Width);
        LOG_I("Packet.FarmHeight: %d", psCcapConfig->sPipeInfo_Packet.u32Height);
    }

    if (psGrabberParam->psViewInfo_Planar)
    {
        /* Planar pipe for encoding */
        psCcapConfig->sPipeInfo_Planar.u32PixFmt   = psGrabberParam->psViewInfo_Planar->u32PixFmt;
        psCcapConfig->sPipeInfo_Planar.u32Width    = psGrabberParam->psViewInfo_Planar->u32Width;
        psCcapConfig->sPipeInfo_Planar.u32Height   = psGrabberParam->psViewInfo_Planar->u32Height;
        psCcapConfig->u32Stride_Planar             = psGrabberParam->psViewInfo_Planar->u32Width;
        if (psGrabberParam->psViewInfo_Planar->pu8FarmAddr == RT_NULL)
        {
            uint32_t u32Sz = psCcapConfig->sPipeInfo_Planar.u32Width *
                             psCcapConfig->sPipeInfo_Planar.u32Height *
                             ccap_get_fmt_bpp(psCcapConfig->sPipeInfo_Planar.u32PixFmt);

            // Allocate memory.
            psCcapConfig->sPipeInfo_Planar.pu8FarmAddr = rt_malloc_align(RT_ALIGN(u32Sz, 32), 32);
            if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr == RT_NULL)
            {
                psCcapConfig->sPipeInfo_Planar.u32Height = 0;
                psCcapConfig->sPipeInfo_Planar.u32Width  = 0;
                psCcapConfig->sPipeInfo_Planar.u32PixFmt = 0;
                psCcapConfig->u32Stride_Planar           = 0;
            }

        }

        LOG_I("Planar.FarmAddr@0x%08X", psCcapConfig->sPipeInfo_Planar.pu8FarmAddr);
        LOG_I("Planar.FarmWidth: %d", psCcapConfig->sPipeInfo_Planar.u32Width);
        LOG_I("Planar.FarmHeight: %d", psCcapConfig->sPipeInfo_Planar.u32Height);
    }

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
            goto exit_ccap_sensor_init;
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
        goto exit_ccap_sensor_init;
    }

    {
        int i32SenClk = psSensorModeInfo->u32SenClk;

        /* speed up pixel clock */
        if (rt_device_control(psDevCcap, CCAP_CMD_SET_SENCLK, (void *)&i32SenClk) != RT_EOK)
        {
            LOG_E("Can't feed setting.");
            goto exit_ccap_sensor_init;
        }
    }

    /* Initial CCAP sensor */
    if (rt_device_open(psDevSensor, 0) != RT_EOK)
    {
        LOG_E("Can't open sensor.");
        goto exit_ccap_sensor_init;
    }

    /* Feed settings to sensor */
    if (rt_device_control(psDevSensor, CCAP_SENSOR_CMD_SET_MODE, (void *)psSensorModeInfo) != RT_EOK)
    {
        LOG_E("Can't feed setting.");
        goto exit_ccap_sensor_init;
    }

    ret = rt_device_control(psDevCcap, CCAP_CMD_SET_PIPES, (void *)psViewInfo);
    if (ret != RT_EOK)
    {
        LOG_E("Can't set pipes %s", psGrabberParam->devname_ccap);
        goto exit_ccap_sensor_init;
    }

    return psDevCcap;

exit_ccap_sensor_init:

    ccap_sensor_fini(rt_device_find(psGrabberParam->devname_ccap), rt_device_find(psGrabberParam->devname_sensor));

    psDevCcap = psDevSensor = RT_NULL;

    return psDevCcap;
}

static void ccap_grabber(void *parameter)
{
    rt_err_t ret;
    rt_device_t psDevCcap = RT_NULL;
    rt_tick_t last, now;
    rt_bool_t bDrawDirect;
    ccap_grabber_param_t psGrabberParam = (ccap_grabber_param_t)parameter;

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

int camera_init(ccap_view_info_t psViewInfo_Packet, ccap_view_info_t psViewInfo_Planar)
{
    rt_thread_t ccap_thread = rt_thread_find(s_GrabberParam.thread_name);
    if (ccap_thread == RT_NULL)
    {
        if (psViewInfo_Packet)
        {
            s_GrabberParam.psViewInfo_Packet = psViewInfo_Packet;
        }

        if (psViewInfo_Planar)
        {
            s_GrabberParam.psViewInfo_Planar = psViewInfo_Planar;
        }

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
        if (psCcapConfig->sPipeInfo_Planar.pu8FarmAddr)
            return (const uint8_t *)psCcapConfig->sPipeInfo_Planar.pu8FarmAddr;
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

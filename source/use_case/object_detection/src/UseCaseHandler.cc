/*
 * SPDX-FileCopyrightText: Copyright 2022 Arm Limited and/or its affiliates
 * <open-source-office@arm.com> SPDX-License-Identifier: Apache-2.0
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
#include "UseCaseHandler.hpp"
#include "DetectorPostProcessing.hpp"
#include "DetectorPreProcessing.hpp"
#include "InputFiles.hpp"
#include "UseCaseCommonUtils.hpp"
#include "YoloFastestModel.hpp"
#include "hal.h"
#include "log_macros.h"

#include <cinttypes>

namespace arm
{
namespace app
{

/**
 * @brief           Presents inference results along using the data presentation
 *                  object.
 * @param[in]       results            Vector of detection results to be displayed.
 * @return          true if successful, false otherwise.
 **/
static bool
PresentInferenceResult(const std::vector<object_detection::DetectionResult> &results);

/**
 * @brief           Draw boxes directly on the LCD for all detected objects.
 * @param[in]       results            Vector of detection results to be displayed.
 * @param[in]       imageStartX        X coordinate where the image starts on the LCD.
 * @param[in]       imageStartY        Y coordinate where the image starts on the LCD.
 * @param[in]       imgDownscaleFactor How much image has been downscaled on LCD.
 **/
static void DrawDetectionBoxes(const std::vector<object_detection::DetectionResult> &results,
                               uint32_t imgStartX,
                               uint32_t imgStartY,
                               uint32_t imgDownscaleFactor);

/* Object detection inference handler. */
bool ObjectDetectionHandler(ApplicationContext &ctx, uint32_t imgIndex, bool runAll)
{
    auto &profiler = ctx.Get<Profiler &>("profiler");


    constexpr uint32_t dataPsnImgDownscaleFactor = 1;
    constexpr uint32_t dataPsnImgStartX          = 10;
    constexpr uint32_t dataPsnImgStartY          = 35;

    constexpr uint32_t dataPsnTxtInfStartX = 20;
    constexpr uint32_t dataPsnTxtInfStartY = 28;

    hal_lcd_clear(COLOR_BLACK);

    auto &model = ctx.Get<Model &>("model");

    /* If the request has a valid size, set the image index. */
    if (imgIndex < NUMBER_OF_FILES)
    {
        if (!SetAppCtxIfmIdx(ctx, imgIndex, "imgIndex"))
        {
            return false;
        }
    }
    if (!model.IsInited())
    {
        printf_err("Model is not initialised! Terminating processing.\n");
        return false;
    }

    auto initialImgIdx = ctx.Get<uint32_t>("imgIndex");

    TfLiteTensor *inputTensor   = model.GetInputTensor(0);
    TfLiteTensor *outputTensor0 = model.GetOutputTensor(0);
    TfLiteTensor *outputTensor1 = model.GetOutputTensor(1);

    if (!inputTensor->dims)
    {
        printf_err("Invalid input tensor dims\n");
        return false;
    }
    else if (inputTensor->dims->size < 3)
    {
        printf_err("Input tensor dimension should be >= 3\n");
        return false;
    }

    TfLiteIntArray *inputShape = model.GetInputShape(0);

    const int inputImgCols = inputShape->data[YoloFastestModel::ms_inputColsIdx];
    const int inputImgRows = inputShape->data[YoloFastestModel::ms_inputRowsIdx];

    /* Set up pre and post-processing. */
    DetectorPreProcess preProcess = DetectorPreProcess(inputTensor, true, model.IsDataSigned());

    std::vector<object_detection::DetectionResult> results;
    const object_detection::PostProcessParams postProcessParams
    {
        inputImgRows,
        inputImgCols,
        object_detection::originalImageSize,
        object_detection::anchor1,
        object_detection::anchor2};
    DetectorPostProcess postProcess =
        DetectorPostProcess(outputTensor0, outputTensor1, results, postProcessParams);
    do
    {
        /* Ensure there are no results leftover from previous inference when running all. */
        results.clear();

        /* Strings for presentation/logging. */
        std::string str_inf{"Running inference... "};

        const uint8_t *currImage = GetImgArray(ctx.Get<uint32_t>("imgIndex"));

        auto dstPtr = static_cast<uint8_t *>(inputTensor->data.uint8);
        const size_t copySz =
            inputTensor->bytes < IMAGE_DATA_SIZE ? inputTensor->bytes : IMAGE_DATA_SIZE;

        /* Run the pre-processing, inference and post-processing. */
        if (!preProcess.DoPreProcess(currImage, copySz))
        {
            printf_err("Pre-processing failed.");
            return false;
        }

        /* Display image on the LCD. */
        hal_lcd_display_image(
            (arm::app::object_detection::channelsImageDisplayed == 3) ? currImage : dstPtr,
            inputImgCols,
            inputImgRows,
            arm::app::object_detection::channelsImageDisplayed,
            dataPsnImgStartX,
            dataPsnImgStartY,
            dataPsnImgDownscaleFactor);

        /* Display message on the LCD - inference running. */
        hal_lcd_display_text(
            str_inf.c_str(), str_inf.size(), dataPsnTxtInfStartX, dataPsnTxtInfStartY, false);

        /* Run inference over this image. */
        info("Running inference on image %" PRIu32 " => %s\n",
             ctx.Get<uint32_t>("imgIndex"),
             GetFilename(ctx.Get<uint32_t>("imgIndex")));

        if (!RunInference(model, profiler))
        {
            printf_err("Inference failed.");
            return false;
        }

        if (!postProcess.DoPostProcess())
        {
            printf_err("Post-processing failed.");
            return false;
        }

        /* Erase. */
        str_inf = std::string(str_inf.size(), ' ');
        hal_lcd_display_text(
            str_inf.c_str(), str_inf.size(), dataPsnTxtInfStartX, dataPsnTxtInfStartY, false);

        /* Draw boxes. */
        DrawDetectionBoxes(
            results, dataPsnImgStartX, dataPsnImgStartY, dataPsnImgDownscaleFactor);

#if VERIFY_TEST_OUTPUT
        DumpTensor(outputTensor0);
        DumpTensor(outputTensor1);
#endif /* VERIFY_TEST_OUTPUT */

        if (!PresentInferenceResult(results))
        {
            return false;
        }

        profiler.PrintProfilingResult();

        IncrementAppCtxIfmIdx(ctx, "imgIndex");

    }
    while (runAll && ctx.Get<uint32_t>("imgIndex") != initialImgIdx);

    return true;
}

/* Object detection live inference handler. */
bool ObjectDetectionHandlerLive(ApplicationContext &ctx)
{
    auto &profiler = ctx.Get<Profiler &>("profiler");

    constexpr uint32_t dataPsnImgDownscaleFactor = 1;
    constexpr uint32_t dataPsnImgStartX          = 0;
    constexpr uint32_t dataPsnImgStartY          = 0;

    auto &model = ctx.Get<Model &>("model");

    if (!model.IsInited())
    {
        printf_err("Model is not initialised! Terminating processing.\n");
        return false;
    }

    TfLiteTensor *inputTensor   = model.GetInputTensor(0);
    TfLiteTensor *outputTensor0 = model.GetOutputTensor(0);
    TfLiteTensor *outputTensor1 = model.GetOutputTensor(1);

    if (!inputTensor->dims)
    {
        printf_err("Invalid input tensor dims\n");
        return false;
    }
    else if (inputTensor->dims->size < 3)
    {
        printf_err("Input tensor dimension should be >= 3\n");
        return false;
    }

    TfLiteIntArray *inputShape = model.GetInputShape(0);

    const int inputImgCols = inputShape->data[YoloFastestModel::ms_inputColsIdx];
    const int inputImgRows = inputShape->data[YoloFastestModel::ms_inputRowsIdx];

    /* Set up pre and post-processing. */
    DetectorPreProcess preProcess = DetectorPreProcess(inputTensor, false, model.IsDataSigned());

    std::vector<object_detection::DetectionResult> results;
    const object_detection::PostProcessParams postProcessParams
    {
        inputImgRows,
        inputImgCols,
        object_detection::originalImageSize,
        object_detection::anchor1,
        object_detection::anchor2
    };

    DetectorPostProcess postProcess =
        DetectorPostProcess(outputTensor0, outputTensor1, results, postProcessParams);

#define TC8263                  0
#define M55M1_PLANAR_ONLY       0
#if (TC8263==1)
    ccap_view_info sViewInfo_Packet;

    /* TEST CHIP use packet-y only */
    sViewInfo_Packet.u32Width    = inputImgCols;
    sViewInfo_Packet.u32Height   = inputImgRows;
    sViewInfo_Packet.pu8FarmAddr = NULL;  /* Allocated in camera driver. */
    sViewInfo_Packet.u32PixFmt   = CCAP_PAR_OUTFMT_ONLY_Y;

    /* Initialise CAMERA - use packet pipe only */
    if (0 != hal_camera_init(&sViewInfo_Packet, NULL))
    {
        printf_err("hal_camera_init failed\n");
        return false;
    }
#else
    ccap_view_info sViewInfo_Planar;
#if (M55M1_PLANAR_ONLY==0)
    ccap_view_info sViewInfo_Packet;
    sViewInfo_Packet.u32Width    = inputImgCols;
    sViewInfo_Packet.u32Height   = inputImgRows;
    sViewInfo_Packet.pu8FarmAddr = NULL;  /* Allocated in camera driver. */
    sViewInfo_Packet.u32PixFmt   = CCAP_PAR_OUTFMT_RGB565;
#endif

    sViewInfo_Planar.u32Width    = inputImgCols;
    sViewInfo_Planar.u32Height   = inputImgRows;
    sViewInfo_Planar.pu8FarmAddr = NULL;  /* Allocated in camera driver. */
    sViewInfo_Planar.u32PixFmt   = CCAP_PAR_PLNFMT_YUV422;

    /* Initialise CAMERA - use packet/planar pipes */
#if (M55M1_PLANAR_ONLY==1)
    if (0 != hal_camera_init(NULL, &sViewInfo_Planar))
#else
    if (0 != hal_camera_init(&sViewInfo_Packet, &sViewInfo_Planar))
#endif
    {
        printf_err("hal_camera_init failed\n");
        return false;
    }
#endif

    const size_t copySz = inputImgRows * inputImgCols;
    while (1)
    {
        /* Sync a frame. */
        hal_camera_sync();

#if (M55M1_PLANAR_ONLY==0)
        /* Get packet buffer. */
        const uint8_t *PktImage = hal_camera_get_frame(0);
        if (PktImage == NULL)
        {
            printf_err("Sync pkt frame failed.");
            break;
        }
#endif

#if (TC8263!=1)
        /* Get planar buffer. */
        const uint8_t *PlaImage = hal_camera_get_frame(1);
        if (PlaImage == NULL)
        {
            printf_err("Sync pla frame failed.");
            break;
        }
#endif

        /* One-shutter new frame. */
        hal_camera_oneshot();

        /* Display image on the LCD. */
        hal_lcd_display_image(
#if (M55M1_PLANAR_ONLY==1)
            PlaImage,
            sViewInfo_Planar.u32Width,
            sViewInfo_Planar.u32Height,
#else
            PktImage,
            sViewInfo_Packet.u32Width,
            sViewInfo_Packet.u32Height,
#endif
#if (TC8263==1) || (M55M1_PLANAR_ONLY==1)
            1,
#else
            2,
#endif
            dataPsnImgStartX,
            dataPsnImgStartY,
            dataPsnImgDownscaleFactor);

        /* Run the pre-processing, inference and post-processing. */
        if (!preProcess.DoPreProcess(
#if (TC8263==1)
                    PktImage,
#else
                    PlaImage,
#endif
                    copySz))
        {
            printf_err("Pre-processing failed.");
            break;
        }

        /* Ensure there are no results leftover from previous inference when running all. */
        results.clear();

        if (!RunInference(model, profiler))
        {
            printf_err("Inference failed.");
            break;
        }

        if (!postProcess.DoPostProcess())
        {
            printf_err("Post-processing failed.");
            break;
        }

        /* Execute poster mission. JPEG encoding -> Base64 transcoding -> MQTT publish image. */
        if (results.size() > 0)
        {
#if (TC8263==1)
            // Only Y
            PosterNotify(ctx, (const uint8_t *)PktImage, inputImgRows, inputImgCols, 1);
#else
            // Only Y
            //PosterNotify(ctx, (const uint8_t *)PlaImage, inputImgRows, inputImgCols, 1);
            // YUV422P
            PosterNotify(ctx, (const uint8_t *)PlaImage, inputImgRows, inputImgCols, 2);
#endif
            /* Draw boxes. */
            DrawDetectionBoxes(
                results, dataPsnImgStartX, dataPsnImgStartY, dataPsnImgDownscaleFactor);
        }

    } // while(1)

    return false;
}

static bool
PresentInferenceResult(const std::vector<object_detection::DetectionResult> &results)
{
    hal_lcd_set_text_color(COLOR_GREEN);

    /* If profiling is enabled, and the time is valid. */
    info("Final results:\n");
    info("Total number of inferences: 1\n");

    for (uint32_t i = 0; i < results.size(); ++i)
    {
        info("%" PRIu32 ") (%f) -> %s {x=%d,y=%d,w=%d,h=%d}\n",
             i,
             results[i].m_normalisedVal,
             "Detection box:",
             results[i].m_x0,
             results[i].m_y0,
             results[i].m_w,
             results[i].m_h);
    }

    return true;
}

static bool
PresentInferenceResultMqtt(const std::vector<object_detection::DetectionResult> &results)
{
    for (uint32_t i = 0; i < results.size(); ++i)
    {
        info("%" PRIu32 ") (%f) -> %s {x=%d,y=%d,w=%d,h=%d}\n",
             i,
             results[i].m_normalisedVal,
             "Detection box:",
             results[i].m_x0,
             results[i].m_y0,
             results[i].m_w,
             results[i].m_h);
    }

    return true;
}

static void DrawDetectionBoxes(const std::vector<object_detection::DetectionResult> &results,
                               uint32_t imgStartX,
                               uint32_t imgStartY,
                               uint32_t imgDownscaleFactor)
{
    uint32_t lineThickness = 1;

    for (const auto &result : results)
    {
        /* Top line. */
        hal_lcd_display_box(imgStartX + result.m_x0 / imgDownscaleFactor,
                            imgStartY + result.m_y0 / imgDownscaleFactor,
                            result.m_w / imgDownscaleFactor,
                            lineThickness,
                            COLOR_GREEN);
        /* Bot line. */
        hal_lcd_display_box(imgStartX + result.m_x0 / imgDownscaleFactor,
                            imgStartY + (result.m_y0 + result.m_h) / imgDownscaleFactor -
                            lineThickness,
                            result.m_w / imgDownscaleFactor,
                            lineThickness,
                            COLOR_GREEN);

        /* Left line. */
        hal_lcd_display_box(imgStartX + result.m_x0 / imgDownscaleFactor,
                            imgStartY + result.m_y0 / imgDownscaleFactor,
                            lineThickness,
                            result.m_h / imgDownscaleFactor,
                            COLOR_GREEN);
        /* Right line. */
        hal_lcd_display_box(imgStartX + (result.m_x0 + result.m_w) / imgDownscaleFactor -
                            lineThickness,
                            imgStartY + result.m_y0 / imgDownscaleFactor,
                            lineThickness,
                            result.m_h / imgDownscaleFactor,
                            COLOR_GREEN);
    }
}

} /* namespace app */
} /* namespace arm */

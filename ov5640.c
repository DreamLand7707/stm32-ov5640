/**
 ******************************************************************************
 * @file    ov5640.c
 * @author  MCD Application Team
 * @brief   This file provides the OV5640 camera driver
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2019-2020 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "ov5640.h"
#include "FreeRTOS.h"
#include "task.h"

/** @addtogroup BSP
 * @{
 */

/** @addtogroup Components
 * @{
 */

/** @addtogroup OV5640
 * @brief     This file provides a set of functions needed to drive the
 *            OV5640 Camera module.
 * @{
 */

/** @defgroup OV5640_Private_TypesDefinitions
 * @{
 */

/**
 * @}
 */
/** @defgroup OV5640_Private_Variables
 * @{
 */

OV5640_CAMERA_Drv_t OV5640_CAMERA_Driver =
    {
        OV5640_Init,
        OV5640_DeInit,
        OV5640_ReadID,
        OV5640_GetCapabilities,
        OV5640_SetLightMode,
        OV5640_SetColorEffect,
        OV5640_SetBrightness,
        OV5640_SetSaturation,
        OV5640_SetContrast,
        OV5640_SetHueDegree,
        OV5640_MirrorFlipConfig,
        OV5640_ZoomConfig,
        OV5640_SetResolution,
        OV5640_GetResolution,
        OV5640_SetPixelFormat,
        OV5640_GetPixelFormat,
        OV5640_NightModeConfig};

/**
 * @}
 */

/** @defgroup OV5640_Private_Functions_Prototypes Private Functions Prototypes
 * @{
 */
static int32_t OV5640_ReadRegWrap(void *handle, uint16_t Reg, uint8_t *Data, uint16_t Length);
static int32_t OV5640_WriteRegWrap(void *handle, uint16_t Reg, uint8_t *Data, uint16_t Length);
static int32_t OV5640_Delay(OV5640_Object_t *pObj, uint32_t Delay);

/**
 * @}
 */

/** @defgroup OV5640_Exported_Functions OV5640 Exported Functions
 * @{
 */
/**
 * @brief  Register component IO bus
 * @param  Component object pointer
 * @retval Component status
 */
int32_t OV5640_RegisterBusIO(OV5640_Object_t *pObj, OV5640_IO_t *pIO) {
    int32_t ret;

    if (pObj == NULL) {
        ret = OV5640_ERROR;
    }
    else {
        pObj->IO.Init      = pIO->Init;
        pObj->IO.DeInit    = pIO->DeInit;
        pObj->IO.Address   = pIO->Address;
        pObj->IO.WriteReg  = pIO->WriteReg;
        pObj->IO.ReadReg   = pIO->ReadReg;
        pObj->IO.GetTick   = pIO->GetTick;

        pObj->Ctx.ReadReg  = OV5640_ReadRegWrap;
        pObj->Ctx.WriteReg = OV5640_WriteRegWrap;
        pObj->Ctx.handle   = pObj;

        pObj->bright       = 0x01;
        pObj->saturation   = 0x41;
        pObj->contrast     = 0x41;
        pObj->huedegree    = 0x32;


        if (pObj->IO.Init != NULL) {
            ret = pObj->IO.Init();
        }
        else {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

/**
 * @brief  Initializes the OV5640 CAMERA component.
 * @param  pObj  pointer to component object
 * @param  Resolution  Camera resolution
 * @param  PixelFormat pixel format to be configured
 * @retval Component status
 */
int32_t OV5640_Init(OV5640_Object_t *pObj, uint32_t Resolution, uint32_t PixelFormat) {
    uint32_t index;
    int32_t  ret = OV5640_OK;

    /* Initialization sequence for OV5640 */
    static const uint16_t OV5640_Common[][2] = {
        {    OV5640_SCCB_SYSTEM_CTRL1, 0x11},
        {        OV5640_SYSTEM_CTROL0, 0x82},
        {    OV5640_SCCB_SYSTEM_CTRL1, 0x03},
        {                      0x3630, 0x36},
        {                      0x3631, 0x0e},
        {                      0x3632, 0xe2},
        {                      0x3633, 0x12},
        {                      0x3621, 0xe0},
        {                      0x3704, 0xa0},
        {                      0x3703, 0x5a},
        {                      0x3715, 0x78},
        {                      0x3717, 0x01},
        {                      0x370b, 0x60},
        {                      0x3705, 0x1a},
        {                      0x3905, 0x02},
        {                      0x3906, 0x10},
        {                      0x3901, 0x0a},
        {                      0x3731, 0x12},
        {                      0x3600, 0x08},
        {                      0x3601, 0x33},
        {                      0x302d, 0x60},
        {                      0x3620, 0x52},
        {                      0x371b, 0x20},
        {                      0x471c, 0x50},
        {           OV5640_AEC_CTRL13, 0x43},
        {OV5640_AEC_GAIN_CEILING_HIGH, 0x00},
        { OV5640_AEC_GAIN_CEILING_LOW, 0xf8},
        {                      0x3635, 0x13},
        {                      0x3636, 0x03},
        {                      0x3634, 0x40},
        {                      0x3622, 0x01},
        {        OV5640_5060HZ_CTRL01, 0x34},
        {        OV5640_5060HZ_CTRL04, 0x28},
        {        OV5640_5060HZ_CTRL05, 0x98},
        {  OV5640_LIGHTMETER1_TH_HIGH, 0x00},
        {   OV5640_LIGHTMETER1_TH_LOW, 0x00},
        {  OV5640_LIGHTMETER2_TH_HIGH, 0x01},
        {   OV5640_LIGHTMETER2_TH_LOW, 0x2c},
        {   OV5640_SAMPLE_NUMBER_HIGH, 0x9c},
        {    OV5640_SAMPLE_NUMBER_LOW, 0x40},
        {      OV5640_TIMING_TC_REG20, 0x06},
        {      OV5640_TIMING_TC_REG21, 0x00},
        {         OV5640_TIMING_X_INC, 0x31},
        {         OV5640_TIMING_Y_INC, 0x31},
        {       OV5640_TIMING_HS_HIGH, 0x00},
        {        OV5640_TIMING_HS_LOW, 0x00},
        {       OV5640_TIMING_VS_HIGH, 0x00},
        {        OV5640_TIMING_VS_LOW, 0x04},
        {       OV5640_TIMING_HW_HIGH, 0x0a},
        {        OV5640_TIMING_HW_LOW, 0x3f},
        {       OV5640_TIMING_VH_HIGH, 0x07},
        {        OV5640_TIMING_VH_LOW, 0x9b},
        {    OV5640_TIMING_DVPHO_HIGH, 0x03},
        {     OV5640_TIMING_DVPHO_LOW, 0x20},
        {    OV5640_TIMING_DVPVO_HIGH, 0x02},
        {     OV5640_TIMING_DVPVO_LOW, 0x58},
        /* For 800x480 resolution: OV5640_TIMING_HTS=0x790, OV5640_TIMING_VTS=0x440 */
        {      OV5640_TIMING_HTS_HIGH, 0x07},
        {       OV5640_TIMING_HTS_LOW, 0x90},
        {      OV5640_TIMING_VTS_HIGH, 0x04},
        {       OV5640_TIMING_VTS_LOW, 0x40},
        {  OV5640_TIMING_HOFFSET_HIGH, 0x00},
        {   OV5640_TIMING_HOFFSET_LOW, 0x10},
        {  OV5640_TIMING_VOFFSET_HIGH, 0x00},
        {   OV5640_TIMING_VOFFSET_LOW, 0x06},
        {                      0x3618, 0x00},
        {                      0x3612, 0x29},
        {                      0x3708, 0x64},
        {                      0x3709, 0x52},
        {                      0x370c, 0x03},
        {           OV5640_AEC_CTRL02, 0x03},
        {           OV5640_AEC_CTRL03, 0xd8},
        {    OV5640_AEC_B50_STEP_HIGH, 0x01},
        {     OV5640_AEC_B50_STEP_LOW, 0x27},
        {    OV5640_AEC_B60_STEP_HIGH, 0x00},
        {     OV5640_AEC_B60_STEP_LOW, 0xf6},
        {           OV5640_AEC_CTRL0E, 0x03},
        {           OV5640_AEC_CTRL0D, 0x04},
        {    OV5640_AEC_MAX_EXPO_HIGH, 0x03},
        {     OV5640_AEC_MAX_EXPO_LOW, 0xd8},
        {           OV5640_BLC_CTRL01, 0x02},
        {           OV5640_BLC_CTRL04, 0x02},
        {       OV5640_SYSREM_RESET00, 0x00},
        {       OV5640_SYSREM_RESET02, 0x1c},
        {       OV5640_CLOCK_ENABLE00, 0xff},
        {       OV5640_CLOCK_ENABLE02, 0xc3},
        {       OV5640_MIPI_CONTROL00, 0x58},
        {                      0x302e, 0x00},
        {        OV5640_POLARITY_CTRL, 0x22},
        {        OV5640_FORMAT_CTRL00, 0x6F},
        {      OV5640_FORMAT_MUX_CTRL, 0x01},
        {      OV5640_JPG_MODE_SELECT, 0x03},
        {          OV5640_JPEG_CTRL07, 0x04},
        {                      0x440e, 0x00},
        {                      0x460b, 0x35},
        {                      0x460c, 0x23},
        {          OV5640_PCLK_PERIOD, 0x22},
        {                      0x3824, 0x02},
        {        OV5640_ISP_CONTROL00, 0xa7},
        {        OV5640_ISP_CONTROL01, 0xa3},
        {           OV5640_AWB_CTRL00, 0xff},
        {           OV5640_AWB_CTRL01, 0xf2},
        {           OV5640_AWB_CTRL02, 0x00},
        {           OV5640_AWB_CTRL03, 0x14},
        {           OV5640_AWB_CTRL04, 0x25},
        {           OV5640_AWB_CTRL05, 0x24},
        {           OV5640_AWB_CTRL06, 0x09},
        {           OV5640_AWB_CTRL07, 0x09},
        {           OV5640_AWB_CTRL08, 0x09},
        {           OV5640_AWB_CTRL09, 0x75},
        {           OV5640_AWB_CTRL10, 0x54},
        {           OV5640_AWB_CTRL11, 0xe0},
        {           OV5640_AWB_CTRL12, 0xb2},
        {           OV5640_AWB_CTRL13, 0x42},
        {           OV5640_AWB_CTRL14, 0x3d},
        {           OV5640_AWB_CTRL15, 0x56},
        {           OV5640_AWB_CTRL16, 0x46},
        {           OV5640_AWB_CTRL17, 0xf8},
        {           OV5640_AWB_CTRL18, 0x04},
        {           OV5640_AWB_CTRL19, 0x70},
        {           OV5640_AWB_CTRL20, 0xf0},
        {           OV5640_AWB_CTRL21, 0xf0},
        {           OV5640_AWB_CTRL22, 0x03},
        {           OV5640_AWB_CTRL23, 0x01},
        {           OV5640_AWB_CTRL24, 0x04},
        {           OV5640_AWB_CTRL25, 0x12},
        {           OV5640_AWB_CTRL26, 0x04},
        {           OV5640_AWB_CTRL27, 0x00},
        {           OV5640_AWB_CTRL28, 0x06},
        {           OV5640_AWB_CTRL29, 0x82},
        {           OV5640_AWB_CTRL30, 0x38},
        {                 OV5640_CMX1, 0x1e},
        {                 OV5640_CMX2, 0x5b},
        {                 OV5640_CMX3, 0x08},
        {                 OV5640_CMX4, 0x0a},
        {                 OV5640_CMX5, 0x7e},
        {                 OV5640_CMX6, 0x88},
        {                 OV5640_CMX7, 0x7c},
        {                 OV5640_CMX8, 0x6c},
        {                 OV5640_CMX9, 0x10},
        {         OV5640_CMXSIGN_HIGH, 0x01},
        {          OV5640_CMXSIGN_LOW, 0x98},
        {    OV5640_CIP_SHARPENMT_TH1, 0x08},
        {    OV5640_CIP_SHARPENMT_TH2, 0x30},
        {OV5640_CIP_SHARPENMT_OFFSET1, 0x10},
        {OV5640_CIP_SHARPENMT_OFFSET2, 0x00},
        {          OV5640_CIP_DNS_TH1, 0x08},
        {          OV5640_CIP_DNS_TH2, 0x30},
        {      OV5640_CIP_DNS_OFFSET1, 0x08},
        {      OV5640_CIP_DNS_OFFSET2, 0x16},
        {             OV5640_CIP_CTRL, 0x08},
        {    OV5640_CIP_SHARPENTH_TH1, 0x30},
        {    OV5640_CIP_SHARPENTH_TH2, 0x04},
        {OV5640_CIP_SHARPENTH_OFFSET1, 0x06},
        {         OV5640_GAMMA_CTRL00, 0x01},
        {          OV5640_GAMMA_YST00, 0x08},
        {          OV5640_GAMMA_YST01, 0x14},
        {          OV5640_GAMMA_YST02, 0x28},
        {          OV5640_GAMMA_YST03, 0x51},
        {          OV5640_GAMMA_YST04, 0x65},
        {          OV5640_GAMMA_YST05, 0x71},
        {          OV5640_GAMMA_YST06, 0x7d},
        {          OV5640_GAMMA_YST07, 0x87},
        {          OV5640_GAMMA_YST08, 0x91},
        {          OV5640_GAMMA_YST09, 0x9a},
        {          OV5640_GAMMA_YST0A, 0xaa},
        {          OV5640_GAMMA_YST0B, 0xb8},
        {          OV5640_GAMMA_YST0C, 0xcd},
        {          OV5640_GAMMA_YST0D, 0xdd},
        {          OV5640_GAMMA_YST0E, 0xea},
        {          OV5640_GAMMA_YST0F, 0x1d},
        {            OV5640_SDE_CTRL0, 0x02},
        {            OV5640_SDE_CTRL3, 0x40},
        {            OV5640_SDE_CTRL4, 0x10},
        {            OV5640_SDE_CTRL9, 0x10},
        {           OV5640_SDE_CTRL10, 0x00},
        {           OV5640_SDE_CTRL11, 0xf8},
        {              OV5640_GMTRX00, 0x23},
        {              OV5640_GMTRX01, 0x14},
        {              OV5640_GMTRX02, 0x0f},
        {              OV5640_GMTRX03, 0x0f},
        {              OV5640_GMTRX04, 0x12},
        {              OV5640_GMTRX05, 0x26},
        {              OV5640_GMTRX10, 0x0c},
        {              OV5640_GMTRX11, 0x08},
        {              OV5640_GMTRX12, 0x05},
        {              OV5640_GMTRX13, 0x05},
        {              OV5640_GMTRX14, 0x08},
        {              OV5640_GMTRX15, 0x0d},
        {              OV5640_GMTRX20, 0x08},
        {              OV5640_GMTRX21, 0x03},
        {              OV5640_GMTRX22, 0x00},
        {              OV5640_GMTRX23, 0x00},
        {              OV5640_GMTRX24, 0x03},
        {              OV5640_GMTRX25, 0x09},
        {              OV5640_GMTRX30, 0x07},
        {              OV5640_GMTRX31, 0x03},
        {              OV5640_GMTRX32, 0x00},
        {              OV5640_GMTRX33, 0x01},
        {              OV5640_GMTRX34, 0x03},
        {              OV5640_GMTRX35, 0x08},
        {              OV5640_GMTRX40, 0x0d},
        {              OV5640_GMTRX41, 0x08},
        {              OV5640_GMTRX42, 0x05},
        {              OV5640_GMTRX43, 0x06},
        {              OV5640_GMTRX44, 0x08},
        {              OV5640_GMTRX45, 0x0e},
        {              OV5640_GMTRX50, 0x29},
        {              OV5640_GMTRX51, 0x17},
        {              OV5640_GMTRX52, 0x11},
        {              OV5640_GMTRX53, 0x11},
        {              OV5640_GMTRX54, 0x15},
        {              OV5640_GMTRX55, 0x28},
        {            OV5640_BRMATRX00, 0x46},
        {            OV5640_BRMATRX01, 0x26},
        {            OV5640_BRMATRX02, 0x08},
        {            OV5640_BRMATRX03, 0x26},
        {            OV5640_BRMATRX04, 0x64},
        {            OV5640_BRMATRX05, 0x26},
        {            OV5640_BRMATRX06, 0x24},
        {            OV5640_BRMATRX07, 0x22},
        {            OV5640_BRMATRX08, 0x24},
        {            OV5640_BRMATRX09, 0x24},
        {            OV5640_BRMATRX20, 0x06},
        {            OV5640_BRMATRX21, 0x22},
        {            OV5640_BRMATRX22, 0x40},
        {            OV5640_BRMATRX23, 0x42},
        {            OV5640_BRMATRX24, 0x24},
        {            OV5640_BRMATRX30, 0x26},
        {            OV5640_BRMATRX31, 0x24},
        {            OV5640_BRMATRX32, 0x22},
        {            OV5640_BRMATRX33, 0x22},
        {            OV5640_BRMATRX34, 0x26},
        {            OV5640_BRMATRX40, 0x44},
        {            OV5640_BRMATRX41, 0x24},
        {            OV5640_BRMATRX42, 0x26},
        {            OV5640_BRMATRX43, 0x28},
        {            OV5640_BRMATRX44, 0x42},
        {       OV5640_LENC_BR_OFFSET, 0xce},
        {                      0x5025, 0x00},
        {           OV5640_AEC_CTRL0F, 0x30},
        {           OV5640_AEC_CTRL10, 0x28},
        {           OV5640_AEC_CTRL1B, 0x30},
        {           OV5640_AEC_CTRL1E, 0x26},
        {           OV5640_AEC_CTRL11, 0x60},
        {           OV5640_AEC_CTRL1F, 0x14},
        {        OV5640_SYSTEM_CTROL0, 0x02},
    };
    uint8_t tmp;

    if (pObj->IsInitialized == 0U) {
        /* Check if resolution is supported */
        if ((Resolution > OV5640_R800x480) ||
            ((PixelFormat != OV5640_RGB565) && (PixelFormat != OV5640_YUV422) &&
             (PixelFormat != OV5640_RGB888) && (PixelFormat != OV5640_Y8) &&
             (PixelFormat != OV5640_JPEG))) {
            ret = OV5640_ERROR;
        }
        else {
            /* Set common parameters for all resolutions */
            for (index = 0; index < (sizeof(OV5640_Common) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_Common[index][1];

                    if (ov5640_write_reg(&pObj->Ctx, OV5640_Common[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }

            if (ret == OV5640_OK) {
                /* Set configuration for Serial Interface */
                if (pObj->Mode == SERIAL_MODE) {
                    if (OV5640_EnableMIPIMode(pObj) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else if (OV5640_SetMIPIVirtualChannel(pObj, pObj->VirtualChannelID) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
                else {
                    /* Set configuration for parallel Interface */
                    if (OV5640_EnableDVPMode(pObj) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        ret = OV5640_OK;
                    }
                }
            }


            if (ret == OV5640_OK) {
                /* Set specific parameters for each resolution */
                if (OV5640_SetResolution(pObj, Resolution) != OV5640_OK) {
                    ret = OV5640_ERROR;
                } /* Set specific parameters for each pixel format */
                else if (OV5640_SetPixelFormat(pObj, PixelFormat) != OV5640_OK) {
                    ret = OV5640_ERROR;
                } /* Set PixelClock, Href and VSync Polarity */
                else if (OV5640_SetPolarities(pObj, OV5640_POLARITY_PCLK_HIGH, OV5640_POLARITY_HREF_HIGH,
                                              OV5640_POLARITY_VSYNC_HIGH) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    pObj->IsInitialized = 1U;
                }
            }
        }
    }

    return ret;
}

/**
 * @brief  De-initializes the camera sensor.
 * @param  pObj  pointer to component object
 * @retval Component status
 */
int32_t OV5640_DeInit(OV5640_Object_t *pObj) {
    if (pObj->IsInitialized == 1U) {
        /* De-initialize camera sensor interface */
        pObj->IsInitialized = 0U;
    }

    return OV5640_OK;
}

/**
 * @brief  Set OV5640 camera Pixel Format.
 * @param  pObj  pointer to component object
 * @param  PixelFormat pixel format to be configured
 * @retval Component status
 */
int32_t OV5640_SetPixelFormat(OV5640_Object_t *pObj, uint32_t PixelFormat) {
    int32_t  ret = OV5640_OK;
    uint32_t index;
    uint8_t  tmp;

    /* Initialization sequence for RGB565 pixel format */
    static const uint16_t OV5640_PF_RGB565[][2] =
        {
            /*  SET PIXEL FORMAT: RGB565 */
            {  OV5640_FORMAT_CTRL00, 0x6F},
            {OV5640_FORMAT_MUX_CTRL, 0x01},
    };

    /* Initialization sequence for YUV422 pixel format */
    static const uint16_t OV5640_PF_YUV422[][2] =
        {
            /*  SET PIXEL FORMAT: YUV422 */
            {  OV5640_FORMAT_CTRL00, 0x30},
            {OV5640_FORMAT_MUX_CTRL, 0x00},
    };

    /* Initialization sequence for RGB888 pixel format */
    static const uint16_t OV5640_PF_RGB888[][2] =
        {
            /*  SET PIXEL FORMAT: RGB888 (RGBRGB)*/
            {  OV5640_FORMAT_CTRL00, 0x23},
            {OV5640_FORMAT_MUX_CTRL, 0x01},
    };

    /* Initialization sequence for Monochrome 8bits pixel format */
    static const uint16_t OV5640_PF_Y8[][2] =
        {
            /*  SET PIXEL FORMAT: Y 8bits */
            {  OV5640_FORMAT_CTRL00, 0x10},
            {OV5640_FORMAT_MUX_CTRL, 0x00},
    };

    /* Initialization sequence for JPEG format */
    static const uint16_t OV5640_PF_JPEG[][2] =
        {
            /*  SET PIXEL FORMAT: JPEG */
            {  OV5640_FORMAT_CTRL00, 0x30},
            {OV5640_FORMAT_MUX_CTRL, 0x00},
    };

    /* Check if PixelFormat is supported */
    if ((PixelFormat != OV5640_RGB565) && (PixelFormat != OV5640_YUV422) &&
        (PixelFormat != OV5640_RGB888) && (PixelFormat != OV5640_Y8) &&
        (PixelFormat != OV5640_JPEG)) {
        /* Pixel format not supported */
        ret = OV5640_ERROR;
    }
    else {
        /* Set specific parameters for each PixelFormat */
        switch (PixelFormat) {
        case OV5640_YUV422:
            for (index = 0; index < (sizeof(OV5640_PF_YUV422) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_PF_YUV422[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_YUV422[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        (void)OV5640_Delay(pObj, 1);
                    }
                }
            }
            break;

        case OV5640_RGB888:
            for (index = 0; index < (sizeof(OV5640_PF_RGB888) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_PF_RGB888[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_RGB888[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        (void)OV5640_Delay(pObj, 1);
                    }
                }
            }
            break;

        case OV5640_Y8:
            for (index = 0; index < (sizeof(OV5640_PF_Y8) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_PF_Y8[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_Y8[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        (void)OV5640_Delay(pObj, 1);
                    }
                }
            }
            break;

        case OV5640_JPEG:
            for (index = 0; index < (sizeof(OV5640_PF_JPEG) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_PF_JPEG[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_JPEG[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        (void)OV5640_Delay(pObj, 1);
                    }
                }
            }
            break;

        case OV5640_RGB565:
        default:
            for (index = 0; index < (sizeof(OV5640_PF_RGB565) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_PF_RGB565[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_PF_RGB565[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        (void)OV5640_Delay(pObj, 1);
                    }
                }
            }
            break;
        }

        if (PixelFormat == OV5640_JPEG) {
            if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
            else {
                tmp |= (1 << 5);
                if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    if (ov5640_read_reg(&pObj->Ctx, OV5640_SYSREM_RESET02, &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else {
                        tmp &= ~((1 << 4) | (1 << 3) | (1 << 2));
                        if (ov5640_write_reg(&pObj->Ctx, OV5640_SYSREM_RESET02, &tmp, 1) != OV5640_OK) {
                            ret = OV5640_ERROR;
                        }
                        else {
                            if (ov5640_read_reg(&pObj->Ctx, OV5640_CLOCK_ENABLE02, &tmp, 1) != OV5640_OK) {
                                ret = OV5640_ERROR;
                            }
                            else {
                                tmp |= ((1 << 5) | (1 << 3));
                                if (ov5640_write_reg(&pObj->Ctx, OV5640_CLOCK_ENABLE02, &tmp, 1) != OV5640_OK) {
                                    ret = OV5640_ERROR;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return ret;
}

/**
 * @brief  Set OV5640 camera Pixel Format.
 * @param  pObj  pointer to component object
 * @param  PixelFormat pixel format to be configured
 * @retval Component status
 */
int32_t OV5640_GetPixelFormat(OV5640_Object_t *pObj, uint32_t *PixelFormat) {
    (void)(pObj);
    (void)(PixelFormat);

    return OV5640_ERROR;
}

/**
 * @brief  Get OV5640 camera resolution.
 * @param  pObj  pointer to component object
 * @param  Resolution  Camera resolution
 * @retval Component status
 */
int32_t OV5640_SetResolution(OV5640_Object_t *pObj, uint32_t Resolution) {
    int32_t  ret = OV5640_OK;
    uint32_t index;
    uint8_t  tmp;

    /* Initialization sequence for WVGA resolution (800x480)*/
    static const uint16_t OV5640_WVGA[][2] =
        {
            {OV5640_TIMING_DVPHO_HIGH, 0x03},
            { OV5640_TIMING_DVPHO_LOW, 0x20},
            {OV5640_TIMING_DVPVO_HIGH, 0x01},
            { OV5640_TIMING_DVPVO_LOW, 0xE0},
    };

    /* Initialization sequence for VGA resolution (640x480)*/
    static const uint16_t OV5640_VGA[][2] =
        {
            {OV5640_TIMING_DVPHO_HIGH, 0x02},
            { OV5640_TIMING_DVPHO_LOW, 0x80},
            {OV5640_TIMING_DVPVO_HIGH, 0x01},
            { OV5640_TIMING_DVPVO_LOW, 0xE0},
    };

    /* Initialization sequence for 480x272 resolution */
    static const uint16_t OV5640_480x272[][2] =
        {
            {OV5640_TIMING_DVPHO_HIGH, 0x01},
            { OV5640_TIMING_DVPHO_LOW, 0xE0},
            {OV5640_TIMING_DVPVO_HIGH, 0x01},
            { OV5640_TIMING_DVPVO_LOW, 0x10},
    };

    /* Initialization sequence for QVGA resolution (320x240) */
    static const uint16_t OV5640_QVGA[][2] =
        {
            {OV5640_TIMING_DVPHO_HIGH, 0x01},
            { OV5640_TIMING_DVPHO_LOW, 0x40},
            {OV5640_TIMING_DVPVO_HIGH, 0x00},
            { OV5640_TIMING_DVPVO_LOW, 0xF0},
    };

    /* Initialization sequence for QQVGA resolution (160x120) */
    static const uint16_t OV5640_QQVGA[][2] =
        {
            {OV5640_TIMING_DVPHO_HIGH, 0x00},
            { OV5640_TIMING_DVPHO_LOW, 0xA0},
            {OV5640_TIMING_DVPVO_HIGH, 0x00},
            { OV5640_TIMING_DVPVO_LOW, 0x78},
    };

    /* Check if resolution is supported */
    if (Resolution > OV5640_R800x480) {
        ret = OV5640_ERROR;
    }
    else {
        /* Initialize OV5640 */
        switch (Resolution) {
        case OV5640_R160x120:
            for (index = 0; index < (sizeof(OV5640_QQVGA) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_QQVGA[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_QQVGA[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_R320x240:
            for (index = 0; index < (sizeof(OV5640_QVGA) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_QVGA[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_QVGA[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_R480x272:
            for (index = 0; index < (sizeof(OV5640_480x272) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_480x272[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_480x272[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_R640x480:
            for (index = 0; index < (sizeof(OV5640_VGA) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_VGA[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_VGA[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_R800x480:
            for (index = 0; index < (sizeof(OV5640_WVGA) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_WVGA[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_WVGA[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        default:
            ret = OV5640_ERROR;
            break;
        }
    }

    return ret;
}

/**
 * @brief  Get OV5640 camera resolution.
 * @param  pObj  pointer to component object
 * @param  Resolution  Camera resolution
 * @retval Component status
 */
int32_t OV5640_GetResolution(OV5640_Object_t *pObj, uint32_t *Resolution) {
    int32_t  ret;
    uint16_t x_size;
    uint16_t y_size;
    uint8_t  tmp;

    if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPHO_HIGH, &tmp, 1) != OV5640_OK) {
        ret = OV5640_ERROR;
    }
    else {
        x_size = (uint16_t)tmp << 8U;

        if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPHO_LOW, &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        else {
            x_size |= tmp;

            if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPVO_HIGH, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
            else {
                y_size = (uint16_t)tmp << 8U;
                if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_DVPVO_LOW, &tmp, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    y_size |= tmp;

                    if ((x_size == 800U) && (y_size == 480U)) {
                        *Resolution = OV5640_R800x480;
                        ret         = OV5640_OK;
                    }
                    else if ((x_size == 640U) && (y_size == 480U)) {
                        *Resolution = OV5640_R640x480;
                        ret         = OV5640_OK;
                    }
                    else if ((x_size == 480U) && (y_size == 272U)) {
                        *Resolution = OV5640_R480x272;
                        ret         = OV5640_OK;
                    }
                    else if ((x_size == 320U) && (y_size == 240U)) {
                        *Resolution = OV5640_R320x240;
                        ret         = OV5640_OK;
                    }
                    else if ((x_size == 160U) && (y_size == 120U)) {
                        *Resolution = OV5640_R160x120;
                        ret         = OV5640_OK;
                    }
                    else {
                        ret = OV5640_ERROR;
                    }
                }
            }
        }
    }

    return ret;
}

/**
 * @brief  Set OV5640 camera PCLK, HREF and VSYNC Polarities
 * @param  pObj  pointer to component object
 * @param  PclkPolarity Polarity of the PixelClock
 * @param  HrefPolarity Polarity of the Href
 * @param  VsyncPolarity Polarity of the Vsync
 * @retval Component status
 */
int32_t OV5640_SetPolarities(OV5640_Object_t *pObj, uint32_t PclkPolarity, uint32_t HrefPolarity,
                             uint32_t VsyncPolarity) {
    uint8_t tmp;
    int32_t ret = OV5640_OK;

    if ((pObj == NULL) || ((PclkPolarity != OV5640_POLARITY_PCLK_LOW) && (PclkPolarity != OV5640_POLARITY_PCLK_HIGH)) ||
        ((HrefPolarity != OV5640_POLARITY_HREF_LOW) && (HrefPolarity != OV5640_POLARITY_HREF_HIGH)) ||
        ((VsyncPolarity != OV5640_POLARITY_VSYNC_LOW) && (VsyncPolarity != OV5640_POLARITY_VSYNC_HIGH))) {
        ret = OV5640_ERROR;
    }
    else {
        tmp = (uint8_t)(PclkPolarity << 5U) | (HrefPolarity << 1U) | VsyncPolarity;

        if (ov5640_write_reg(&pObj->Ctx, OV5640_POLARITY_CTRL, &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

/**
 * @brief  get OV5640 camera PCLK, HREF and VSYNC Polarities
 * @param  pObj  pointer to component object
 * @param  PclkPolarity Polarity of the PixelClock
 * @param  HrefPolarity Polarity of the Href
 * @param  VsyncPolarity Polarity of the Vsync
 * @retval Component status
 */
int32_t OV5640_GetPolarities(OV5640_Object_t *pObj, uint32_t *PclkPolarity, uint32_t *HrefPolarity,
                             uint32_t *VsyncPolarity) {
    uint8_t tmp;
    int32_t ret = OV5640_OK;

    if ((pObj == NULL) || (PclkPolarity == NULL) || (HrefPolarity == NULL) || (VsyncPolarity == NULL)) {
        ret = OV5640_ERROR;
    }
    else if (ov5640_read_reg(&pObj->Ctx, OV5640_POLARITY_CTRL, &tmp, 1) != OV5640_OK) {
        ret = OV5640_ERROR;
    }
    else {
        *PclkPolarity  = (tmp >> 5U) & 0x01U;
        *HrefPolarity  = (tmp >> 1U) & 0x01U;
        *VsyncPolarity = tmp & 0x01;
    }

    return ret;
}

/**
 * @brief  Read the OV5640 Camera identity.
 * @param  pObj  pointer to component object
 * @param  Id    pointer to component ID
 * @retval Component status
 */
int32_t OV5640_ReadID(OV5640_Object_t *pObj, uint32_t *Id) {
    int32_t ret;
    uint8_t tmp;

    /* Initialize I2C */
    pObj->IO.Init();

    /* Prepare the camera to be configured */
    tmp = 0x80;
    if (ov5640_write_reg(&pObj->Ctx, OV5640_SYSTEM_CTROL0, &tmp, 1) != OV5640_OK) {
        ret = OV5640_ERROR;
    }
    else {
        (void)OV5640_Delay(pObj, 500);

        if (ov5640_read_reg(&pObj->Ctx, OV5640_CHIP_ID_HIGH_BYTE, &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        else {
            *Id = (uint32_t)tmp << 8U;
            if (ov5640_read_reg(&pObj->Ctx, OV5640_CHIP_ID_LOW_BYTE, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
            else {
                *Id |= tmp;
                ret = OV5640_OK;
            }
        }
    }

    /* Component status */
    return ret;
}

/**
 * @brief  Read the OV5640 Camera Capabilities.
 * @param  pObj          pointer to component object
 * @param  Capabilities  pointer to component Capabilities
 * @retval Component status
 */
int32_t OV5640_GetCapabilities(OV5640_Object_t *pObj, OV5640_Capabilities_t *Capabilities) {
    int32_t ret;

    if (pObj == NULL) {
        ret = OV5640_ERROR;
    }
    else {
        Capabilities->Config_Brightness    = 1;
        Capabilities->Config_Contrast      = 1;
        Capabilities->Config_HueDegree     = 1;
        Capabilities->Config_LightMode     = 1;
        Capabilities->Config_MirrorFlip    = 1;
        Capabilities->Config_NightMode     = 1;
        Capabilities->Config_Resolution    = 1;
        Capabilities->Config_Saturation    = 1;
        Capabilities->Config_SpecialEffect = 1;
        Capabilities->Config_Zoom          = 1;

        ret                                = OV5640_OK;
    }

    return ret;
}

/**
 * @brief  Set the OV5640 camera Light Mode.
 * @param  pObj  pointer to component object
 * @param  Effect  Effect to be configured
 * @retval Component status
 */
int32_t OV5640_SetLightMode(OV5640_Object_t *pObj, uint32_t LightMode) {
    int32_t  ret;
    uint32_t index;
    uint8_t  tmp;

    /* OV5640 Light Mode setting */
    static const uint16_t OV5640_LightModeAuto[][2] =
        {
            {OV5640_AWB_MANUAL_CONTROL, 0x00},
            {    OV5640_AWB_R_GAIN_MSB, 0x04},
            {    OV5640_AWB_R_GAIN_LSB, 0x00},
            {    OV5640_AWB_G_GAIN_MSB, 0x04},
            {    OV5640_AWB_G_GAIN_LSB, 0x00},
            {    OV5640_AWB_B_GAIN_MSB, 0x04},
            {    OV5640_AWB_B_GAIN_LSB, 0x00},
    };

    static const uint16_t OV5640_LightModeCloudy[][2] =
        {
            {OV5640_AWB_MANUAL_CONTROL, 0x01},
            {    OV5640_AWB_R_GAIN_MSB, 0x06},
            {    OV5640_AWB_R_GAIN_LSB, 0x48},
            {    OV5640_AWB_G_GAIN_MSB, 0x04},
            {    OV5640_AWB_G_GAIN_LSB, 0x00},
            {    OV5640_AWB_B_GAIN_MSB, 0x04},
            {    OV5640_AWB_B_GAIN_LSB, 0xD3},
    };

    static const uint16_t OV5640_LightModeOffice[][2] =
        {
            {OV5640_AWB_MANUAL_CONTROL, 0x01},
            {    OV5640_AWB_R_GAIN_MSB, 0x05},
            {    OV5640_AWB_R_GAIN_LSB, 0x48},
            {    OV5640_AWB_G_GAIN_MSB, 0x04},
            {    OV5640_AWB_G_GAIN_LSB, 0x00},
            {    OV5640_AWB_B_GAIN_MSB, 0x07},
            {    OV5640_AWB_B_GAIN_LSB, 0xCF},
    };

    static const uint16_t OV5640_LightModeHome[][2] =
        {
            {OV5640_AWB_MANUAL_CONTROL, 0x01},
            {    OV5640_AWB_R_GAIN_MSB, 0x04},
            {    OV5640_AWB_R_GAIN_LSB, 0x10},
            {    OV5640_AWB_G_GAIN_MSB, 0x04},
            {    OV5640_AWB_G_GAIN_LSB, 0x00},
            {    OV5640_AWB_B_GAIN_MSB, 0x08},
            {    OV5640_AWB_B_GAIN_LSB, 0xB6},
    };

    static const uint16_t OV5640_LightModeSunny[][2] =
        {
            {OV5640_AWB_MANUAL_CONTROL, 0x01},
            {    OV5640_AWB_R_GAIN_MSB, 0x06},
            {    OV5640_AWB_R_GAIN_LSB, 0x1C},
            {    OV5640_AWB_G_GAIN_MSB, 0x04},
            {    OV5640_AWB_G_GAIN_LSB, 0x00},
            {    OV5640_AWB_B_GAIN_MSB, 0x04},
            {    OV5640_AWB_B_GAIN_LSB, 0xF3},
    };

    tmp = 0x00;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_MANUAL_CONTROL, &tmp, 1);
    if (ret == OV5640_OK) {
        tmp = 0x46;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_CTRL16, &tmp, 1);
    }

    if (ret == OV5640_OK) {
        tmp = 0xF8;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_CTRL17, &tmp, 1);
    }

    if (ret == OV5640_OK) {
        tmp = 0x04;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_AWB_CTRL18, &tmp, 1);
    }

    if (ret == OV5640_OK) {
        switch (LightMode) {
        case OV5640_LIGHT_SUNNY:
            for (index = 0; index < (sizeof(OV5640_LightModeSunny) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_LightModeSunny[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeSunny[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_LIGHT_OFFICE:
            for (index = 0; index < (sizeof(OV5640_LightModeOffice) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_LightModeOffice[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeOffice[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_LIGHT_CLOUDY:
            for (index = 0; index < (sizeof(OV5640_LightModeCloudy) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_LightModeCloudy[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeCloudy[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_LIGHT_HOME:
            for (index = 0; index < (sizeof(OV5640_LightModeHome) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_LightModeHome[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeHome[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        case OV5640_LIGHT_AUTO:
        default:
            for (index = 0; index < (sizeof(OV5640_LightModeAuto) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)OV5640_LightModeAuto[index][1];
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_LightModeAuto[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }
            break;
        }
    }
    return ret;
}

/**
 * @brief  Set the OV5640 camera Special Effect.
 * @param  pObj  pointer to component object
 * @param  Effect  Effect to be configured
 * @retval Component status
 */
int32_t OV5640_SetColorEffect(OV5640_Object_t *pObj, uint32_t Effect) {
    int32_t ret;
    uint8_t tmp;

    switch (Effect) {
    case OV5640_COLOR_EFFECT_BLUE:
        tmp = 0xFF;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x18 | 0x07;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0xA0;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x40;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        }

        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        break;

    case OV5640_COLOR_EFFECT_RED:
        tmp = 0xFF;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x18 | 0x07;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x80;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0xC0;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        }

        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        break;

    case OV5640_COLOR_EFFECT_GREEN:
        tmp = 0xFF;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x18;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x60;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x60;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        }

        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        break;

    case OV5640_COLOR_EFFECT_BW:
        tmp = 0xFF;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x18 | 0x07;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x80;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x80;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        }

        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        break;

    case OV5640_COLOR_EFFECT_SEPIA:
        tmp = 0xFF;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x18 | 0x07;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x40;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0xA0;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        }

        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        break;

    case OV5640_COLOR_EFFECT_NEGATIVE:
        tmp = 0xFF;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x47;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }
        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        break;

    case OV5640_COLOR_EFFECT_NONE:
    default:
        tmp = 0x7F;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

        if (ret == OV5640_OK) {
            tmp = 0x07;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
        }

        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }

        break;
    }

    return ret;
}

/**
 * @brief  Set the OV5640 camera Brightness Level.
 * @note   The brightness of OV5640 could be adjusted. Higher brightness will
 *         make the picture more bright. The side effect of higher brightness
 *         is the picture looks foggy.
 * @param  pObj  pointer to component object
 * @param  Level Value to be configured
 * @retval Component status
 */
int32_t OV5640_SetBrightness(OV5640_Object_t *pObj, int32_t Level) {
    int32_t       ret;
    const uint8_t brightness_level[] = {0x40U, 0x30U, 0x20U, 0x10U, 0x00U, 0x10U, 0x20U, 0x30U, 0x40U};
    uint8_t       tmp;

    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
        tmp = brightness_level[Level + 4];
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL7, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        tmp = 0x07;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }

    if (ret == OV5640_OK) {
        if (Level < 0) {
            pObj->bright = 0x01;
            tmp          = pObj->contrast | pObj->bright | pObj->huedegree | pObj->saturation;
            if (ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
        }
        else {
            pObj->bright = 0x09;
            tmp          = pObj->contrast | pObj->bright | pObj->huedegree | pObj->saturation;
            if (ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
        }
    }

    return ret;
}

/**
 * @brief  Set the OV5640 camera Saturation Level.
 * @note   The color saturation of OV5640 could be adjusted. High color saturation
 *         would make the picture looks more vivid, but the side effect is the
 *         bigger noise and not accurate skin color.
 * @param  pObj  pointer to component object
 * @param  Level Value to be configured
 * @retval Component status
 */
int32_t OV5640_SetSaturation(OV5640_Object_t *pObj, int32_t Level) {
    int32_t       ret;
    const uint8_t saturation_level[] = {0x00U, 0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U, 0x70U, 0x80U};
    uint8_t       tmp;

    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
        tmp = saturation_level[Level + 4];
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL3, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        tmp = 0x07;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }

    if (ret == OV5640_OK) {
        pObj->saturation = 0x41;
        tmp              = pObj->contrast | pObj->bright | pObj->huedegree | pObj->saturation;
        ret              = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1);
    }

    if (ret != OV5640_OK) {
        ret = OV5640_ERROR;
    }

    return ret;
}

/**
 * @brief  Set the OV5640 camera Contrast Level.
 * @note   The contrast of OV5640 could be adjusted. Higher contrast will make
 *         the picture sharp. But the side effect is losing dynamic range.
 * @param  pObj  pointer to component object
 * @param  Level Value to be configured
 * @retval Component status
 */
int32_t OV5640_SetContrast(OV5640_Object_t *pObj, int32_t Level) {
    int32_t       ret;
    const uint8_t contrast_level[] = {0x10U, 0x14U, 0x18U, 0x1CU, 0x20U, 0x24U, 0x28U, 0x2CU, 0x30U};
    uint8_t       tmp;

    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
        tmp = 0x07;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        tmp = contrast_level[Level + 4];
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL6, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL5, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        pObj->contrast = 0x41;
        tmp            = pObj->contrast | pObj->bright | pObj->huedegree | pObj->saturation;
        ret            = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1);
    }

    if (ret != OV5640_OK) {
        ret = OV5640_ERROR;
    }

    return ret;
}

/**
 * @brief  Set the OV5640 camera Hue degree.
 * @param  pObj  pointer to component object
 * @param  Level Value to be configured
 * @retval Component status
 */
int32_t OV5640_SetHueDegree(OV5640_Object_t *pObj, int32_t Degree) {
    int32_t       ret;
    const uint8_t hue_degree_ctrl1[] = {0x80U, 0x6FU, 0x40U, 0x00U, 0x40U, 0x6FU, 0x80U, 0x6FU, 0x40U, 0x00U, 0x40U,
                                        0x6FU};
    const uint8_t hue_degree_ctrl2[] = {0x00U, 0x40U, 0x6FU, 0x80U, 0x6FU, 0x40U, 0x00U, 0x40U, 0x6FU, 0x80U, 0x6FU,
                                        0x40U};
    const uint8_t hue_degree_ctrl8[] = {0x32U, 0x32U, 0x32U, 0x02U, 0x02U, 0x02U, 0x01U, 0x01U, 0x01U, 0x31U, 0x31U,
                                        0x31U};
    uint8_t       tmp;

    tmp = 0xFF;
    ret = ov5640_write_reg(&pObj->Ctx, OV5640_ISP_CONTROL01, &tmp, 1);

    if (ret == OV5640_OK) {
        tmp = 0x07;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL0, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        tmp = hue_degree_ctrl1[Degree + 6];
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL1, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        tmp = hue_degree_ctrl2[Degree + 6];
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL2, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        pObj->huedegree = hue_degree_ctrl8[Degree + 6];
        tmp             = pObj->contrast | pObj->bright | pObj->huedegree | pObj->saturation;
        ret             = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL8, &tmp, 1);
    }

    if (ret != OV5640_OK) {
        ret = OV5640_ERROR;
    }

    return ret;
}

/**
 * @brief  Control OV5640 camera mirror/vflip.
 * @param  pObj  pointer to component object
 * @param  Config To configure mirror, flip, both or none
 * @retval Component status
 */
int32_t OV5640_MirrorFlipConfig(OV5640_Object_t *pObj, uint32_t Config) {
    int32_t ret;
    uint8_t tmp3820 = 0;
    uint8_t tmp3821;

    if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) != OV5640_OK) {
        ret = OV5640_ERROR;
    }
    else {
        tmp3820 &= 0xF9U;

        if (ov5640_read_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        else {
            ret = OV5640_OK;
            tmp3821 &= 0xF9U;

            switch (Config) {
            case OV5640_MIRROR:
                if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    tmp3821 |= 0x06U;
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
                break;
            case OV5640_FLIP:
                tmp3820 |= 0x06U;
                if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
                break;
            case OV5640_MIRROR_FLIP:
                tmp3820 |= 0x06U;
                if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    tmp3821 |= 0x06U;
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
                break;

            case OV5640_MIRROR_FLIP_NONE:
            default:
                if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG20, &tmp3820, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
                else {
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_TIMING_TC_REG21, &tmp3821, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
                break;
            }
        }
    }

    return ret;
}

/**
 * @brief  Control OV5640 camera zooming.
 * @param  pObj  pointer to component object
 * @param  Zoom  Zoom to be configured
 * @retval Component status
 */
int32_t OV5640_ZoomConfig(OV5640_Object_t *pObj, uint32_t Zoom) {
    int32_t  ret = OV5640_OK;
    uint32_t res;
    uint32_t zoom;
    uint8_t  tmp;

    /* Get camera resolution */
    if (OV5640_GetResolution(pObj, &res) != OV5640_OK) {
        ret = OV5640_ERROR;
    }
    else {
        zoom = Zoom;

        if (zoom == OV5640_ZOOM_x1) {
            tmp = 0x10;
            if (ov5640_write_reg(&pObj->Ctx, OV5640_SCALE_CTRL0, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
        }
        else {
            switch (res) {
            case OV5640_R320x240:
            case OV5640_R480x272:
                zoom = zoom >> 1U;
                break;
            case OV5640_R640x480:
                zoom = zoom >> 2U;
                break;
            default:
                break;
            }

            tmp = 0x00;
            if (ov5640_write_reg(&pObj->Ctx, OV5640_SCALE_CTRL0, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
            else {
                tmp = (uint8_t)zoom;
                if (ov5640_write_reg(&pObj->Ctx, OV5640_SCALE_CTRL1, &tmp, 1) != OV5640_OK) {
                    ret = OV5640_ERROR;
                }
            }
        }
    }

    return ret;
}

/**
 * @brief  Enable/disable the OV5640 camera night mode.
 * @param  pObj  pointer to component object
 * @param  Cmd   Enable disable night mode
 * @retval Component status
 */
int32_t OV5640_NightModeConfig(OV5640_Object_t *pObj, uint32_t Cmd) {
    int32_t ret;
    uint8_t tmp = 0;

    if (Cmd == NIGHT_MODE_ENABLE) {
        /* Auto Frame Rate: 15fps ~ 3.75fps night mode for 60/50Hz light environment,
        24Mhz clock input,24Mhz PCLK*/
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL4, &tmp, 1);
        if (ret == OV5640_OK) {
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL5, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x7C;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL00, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x01;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B50_STEP_HIGH, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x27;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B50_STEP_LOW, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x00;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B60_STEP_HIGH, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0xF6;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_B60_STEP_LOW, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x04;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL0D, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL0E, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x0B;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL02, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x88;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL03, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x0B;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_MAX_EXPO_HIGH, &tmp, 1);
        }
        if (ret == OV5640_OK) {
            tmp = 0x88;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_AEC_MAX_EXPO_LOW, &tmp, 1);
        }
        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }
    else {
        if (ov5640_read_reg(&pObj->Ctx, OV5640_AEC_CTRL00, &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
        else {
            ret = OV5640_OK;
            tmp &= 0xFBU;
            /* Set Bit 2 to 0 */
            if (ov5640_write_reg(&pObj->Ctx, OV5640_AEC_CTRL00, &tmp, 1) != OV5640_OK) {
                ret = OV5640_ERROR;
            }
        }
    }

    return ret;
}
/**
 * @brief  Configure Embedded Synchronization mode.
 * @param  pObj  pointer to component object
 * @param  pSyncCodes  pointer to Embedded Codes
 * @retval Component status
 */

int32_t OV5640_EmbeddedSynchroConfig(OV5640_Object_t *pObj, OV5640_SyncCodes_t *pSyncCodes) {
    uint8_t tmp;
    int32_t ret = OV5640_ERROR;

    /*[7] : SYNC code from reg 0x4732-0x4732, [1]: Enable Clip ,[0]: Enable CCIR656 */
    tmp = 0x83;
    if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_CTRL00, &tmp, 1) == OV5640_OK) {
        tmp = pSyncCodes->FrameStartCode;
        if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_FS, &tmp, 1) == OV5640_OK) {
            tmp = pSyncCodes->FrameEndCode;
            if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_FE, &tmp, 1) != OV5640_OK) {
                return OV5640_ERROR;
            }
            tmp = pSyncCodes->LineStartCode;
            if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_LS, &tmp, 1) == OV5640_OK) {
                tmp = pSyncCodes->LineEndCode;
                if (ov5640_write_reg(&pObj->Ctx, OV5640_CCIR656_LE, &tmp, 1) == OV5640_OK) {
                    /*Adding 1 dummy line */
                    tmp = 0x01;
                    if (ov5640_write_reg(&pObj->Ctx, OV5640_656_DUMMY_LINE, &tmp, 1) == OV5640_OK) {
                        ret = OV5640_OK;
                    }
                }
            }
        }
    }

    /* max clip value[9:8], to avoid SYNC code clipping */
    tmp = 0x2;
    if (ret == OV5640_OK) {
        ret = ov5640_write_reg(&pObj->Ctx, 0x4302, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        ret = ov5640_write_reg(&pObj->Ctx, 0x4306, &tmp, 1);
    }
    if (ret == OV5640_OK) {
        ret = ov5640_write_reg(&pObj->Ctx, 0x430A, &tmp, 1);
    }

    return ret;
}
/**
 * @brief  Enable/disable the OV5640 color bar mode.
 * @param  pObj  pointer to component object
 * @param  Cmd   Enable disable colorbar
 * @retval Component status
 */
int32_t OV5640_ColorbarModeConfig(OV5640_Object_t *pObj, uint32_t Cmd) {
    int32_t ret;
    uint8_t tmp = 0x40;

    if ((Cmd == COLORBAR_MODE_ENABLE) || (Cmd == COLORBAR_MODE_GRADUALV)) {
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        if (ret == OV5640_OK) {
            tmp = (Cmd == COLORBAR_MODE_GRADUALV ? 0x8c : 0x80);
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_PRE_ISP_TEST_SETTING1, &tmp, 1);
        }
        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }
    else {
        tmp = 0x10;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SDE_CTRL4, &tmp, 1);
        if (ret == OV5640_OK) {
            tmp = 0x00;
            ret = ov5640_write_reg(&pObj->Ctx, OV5640_PRE_ISP_TEST_SETTING1, &tmp, 1);
        }
        if (ret != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

/**
 * @brief  Set the camera pixel clock
 * @param  pObj  pointer to component object
 * @param  ClockValue Can be OV5640_PCLK_48M, OV5640_PCLK_24M, OV5640_PCLK_12M, OV5640_PCLK_9M
 *                    OV5640_PCLK_8M, OV5640_PCLK_7M
 * @retval Component status
 */
int32_t OV5640_SetPCLK(OV5640_Object_t *pObj, uint32_t ClockValue) {
    int32_t ret;
    uint8_t tmp;

    switch (ClockValue) {
    case OV5640_PCLK_7M:
        tmp = 0x38;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
        tmp = 0x16;
        ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
        break;
    case OV5640_PCLK_8M:
        tmp = 0x40;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
        tmp = 0x16;
        ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
        break;
    case OV5640_PCLK_9M:
        tmp = 0x60;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
        tmp = 0x18;
        ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
        break;
    case OV5640_PCLK_12M:
        tmp = 0x60;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
        tmp = 0x16;
        ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
        break;
    case OV5640_PCLK_48M:
        tmp = 0x60;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
        tmp = 0x03;
        ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
        break;
    case OV5640_PCLK_24M:
    default:
        tmp = 0x60;
        ret = ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL2, &tmp, 1);
        tmp = 0x13;
        ret += ov5640_write_reg(&pObj->Ctx, OV5640_SC_PLL_CONTRL3, &tmp, 1);
        break;
    }

    if (ret != OV5640_OK) {
        ret = OV5640_ERROR;
    }

    return ret;
}

/**
 * @brief  Enable DVP(Digital Video Port) Mode: Parallel Data Output
 * @param  pObj  pointer to component object
 * @retval Component status
 */
int OV5640_EnableDVPMode(OV5640_Object_t *pObj) {
    uint32_t              index;
    int32_t               ret = OV5640_OK;
    uint8_t               tmp;


    static const uint16_t regs[10][2] = {
        /* Configure the IO Pad, output FREX/VSYNC/HREF/PCLK/D[9:2]/GPIO0/GPIO1 */
        {OV5640_PAD_OUTPUT_ENABLE01, 0xFF},
        {OV5640_PAD_OUTPUT_ENABLE02, 0xF3},
        {                    0x302e, 0x00},
        /* Unknown DVP control configuration */
        {                    0x471c, 0x50},
        {     OV5640_MIPI_CONTROL00, 0x58},
        /* Timing configuration */
        {     OV5640_SC_PLL_CONTRL0, 0x18},
        {     OV5640_SC_PLL_CONTRL1, 0x41},
        {     OV5640_SC_PLL_CONTRL2, 0x60},
        {     OV5640_SC_PLL_CONTRL3, 0x13},
        {OV5640_SYSTEM_ROOT_DIVIDER, 0x01},
    };

    for (index = 0; index < sizeof(regs) / 4U; index++) {
        tmp = (uint8_t)regs[index][1];
        if (ov5640_write_reg(&pObj->Ctx, regs[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
            break;
        }
    }

    return ret;
}

int OV5640_DisablePADOutput(OV5640_Object_t *pObj) {
    uint32_t              index;
    int32_t               ret = OV5640_OK;
    uint8_t               tmp;


    static const uint16_t regs[4][2] =
        {
            /* Configure the IO Pad, output FREX/VSYNC/HREF/PCLK/D[9:2]/GPIO0/GPIO1 */
            {OV5640_PAD_OUTPUT_ENABLE01, 0x00},
            {OV5640_PAD_OUTPUT_ENABLE02, 0x00},
            {       OV5640_PAD_SELECT01,    0},
            {       OV5640_PAD_SELECT02,    0},
    };

    for (index = 0; index < sizeof(regs) / 4U; index++) {
        tmp = (uint8_t)regs[index][1];
        if (ov5640_write_reg(&pObj->Ctx, regs[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
            break;
        }
    }

    return ret;
}

/**
 * @brief  Enable MIPI (Mobile Industry Processor Interface) Mode: Serial port
 * @param  pObj  pointer to component object
 * @retval Component status
 */
int32_t OV5640_EnableMIPIMode(OV5640_Object_t *pObj) {
    int32_t               ret = OV5640_OK;
    uint8_t               tmp;
    uint32_t              index;

    static const uint16_t regs[14][2] =
        {
            /* PAD settings */
            {OV5640_PAD_OUTPUT_ENABLE01,    0},
            {OV5640_PAD_OUTPUT_ENABLE02,    0},
            {                    0x302e, 0x08},
            /* Pixel clock period */
            {        OV5640_PCLK_PERIOD, 0x23},
            /* Timing configuration */
            {     OV5640_SC_PLL_CONTRL0, 0x18},
            {     OV5640_SC_PLL_CONTRL1, 0x12},
            {     OV5640_SC_PLL_CONTRL2, 0x1C},
            {     OV5640_SC_PLL_CONTRL3, 0x13},
            {OV5640_SYSTEM_ROOT_DIVIDER, 0x01},
            {                    0x4814, 0x2a},
            {        OV5640_MIPI_CTRL00, 0x24},
            { OV5640_PAD_OUTPUT_VALUE00, 0x70},
            {     OV5640_MIPI_CONTROL00, 0x45},
            {       OV5640_FRAME_CTRL02, 0x00},
    };

    for (index = 0; index < sizeof(regs) / 4U; index++) {
        tmp = (uint8_t)regs[index][1];
        if (ov5640_write_reg(&pObj->Ctx, regs[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
            break;
        }
    }

    return ret;
}

/**
 * @brief  Set MIPI VirtualChannel
 * @param  pObj  pointer to component object
 * @param  vchannel virtual channel for Mipi Mode
 * @retval Component status
 */
int32_t OV5640_SetMIPIVirtualChannel(OV5640_Object_t *pObj, uint32_t vchannel) {
    int32_t ret = OV5640_OK;
    uint8_t tmp;

    if (ov5640_read_reg(&pObj->Ctx, 0x4814, &tmp, 1) != OV5640_OK) {
        ret = OV5640_ERROR;
    }
    else {
        tmp &= ~(3 << 6);
        tmp |= (vchannel << 6);
        if (ov5640_write_reg(&pObj->Ctx, 0x4814, &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

/**
 * @brief  Start camera
 * @param  pObj  pointer to component object
 * @retval Component status
 */
int32_t OV5640_Start(OV5640_Object_t *pObj) {
    uint8_t tmp;

    tmp = 0x2;
    return ov5640_write_reg(&pObj->Ctx, OV5640_SYSTEM_CTROL0, &tmp, 1);
}

/**
 * @brief  Stop camera
 * @param  pObj  pointer to component object
 * @retval Component status
 */
int32_t OV5640_Stop(OV5640_Object_t *pObj) {
    uint8_t tmp;

    tmp = 0x42;
    return ov5640_write_reg(&pObj->Ctx, OV5640_SYSTEM_CTROL0, &tmp, 1);
}


/**
 * @}
 */

/** @defgroup OV5640_Private_Functions Private Functions
 * @{
 */
/**
 * @brief This function provides accurate delay (in milliseconds)
 * @param pObj   pointer to component object
 * @param Delay  specifies the delay time length, in milliseconds
 * @retval OV5640_OK
 */
static int32_t OV5640_Delay(OV5640_Object_t *pObj, uint32_t Delay) {
    uint32_t tickstart;
    tickstart = pObj->IO.GetTick();
    while ((pObj->IO.GetTick() - tickstart) < Delay) {
    }
    return OV5640_OK;
}

/**
 * @brief  Wrap component ReadReg to Bus Read function
 * @param  handle  Component object handle
 * @param  Reg  The target register address to write
 * @param  pData  The target register value to be written
 * @param  Length  buffer size to be written
 * @retval error status
 */
static int32_t OV5640_ReadRegWrap(void *handle, uint16_t Reg, uint8_t *pData, uint16_t Length) {
    OV5640_Object_t *pObj = (OV5640_Object_t *)handle;

    return pObj->IO.ReadReg(pObj->IO.Address, Reg, pData, Length);
}

/**
 * @brief  Wrap component WriteReg to Bus Write function
 * @param  handle  Component object handle
 * @param  Reg  The target register address to write
 * @param  pData  The target register value to be written
 * @param  Length  buffer size to be written
 * @retval error status
 */
static int32_t OV5640_WriteRegWrap(void *handle, uint16_t Reg, uint8_t *pData, uint16_t Length) {
    OV5640_Object_t *pObj = (OV5640_Object_t *)handle;

    return pObj->IO.WriteReg(pObj->IO.Address, Reg, pData, Length);
}

/**
 * @}
 */

static uint16_t ov5640_uxga_init_reg_tbl[][2] = {
    // 24MHz input clock, 24MHz PCLK
    {0x3008, 0x42}, // software power down, bit[6]
    {0x3103, 0x03}, // system clock from PLL, bit[1]
    {0x3017, 0xff}, // FREX, Vsync, HREF, PCLK, D[9:6] output enable
    {0x3018, 0xff}, // D[5:0], GPIO[1:0] output enable
    {0x3034, 0x1a}, // MIPI 10-bit
    {0x3037, 0x13}, // PLL root divider, bit[4], PLL pre-divider, bit[3:0]
    {0x3108, 0x01}, // PCLK root divider, bit[5:4], SCLK2x root divider, bit[3:2]

    // SCLK root divider, bit[1:0]
    {0x3630, 0x36},
    {0x3631, 0x0e},
    {0x3632, 0xe2},
    {0x3633, 0x12},
    {0x3621, 0xe0},
    {0x3704, 0xa0},
    {0x3703, 0x5a},
    {0x3715, 0x78},
    {0x3717, 0x01},
    {0x370b, 0x60},
    {0x3705, 0x1a},
    {0x3905, 0x02},
    {0x3906, 0x10},
    {0x3901, 0x0a},
    {0x3731, 0x12},
    {0x3600, 0x08}, // VCM control
    {0x3601, 0x33}, // VCM control
    {0x302d, 0x60}, // system control
    {0x3620, 0x52},
    {0x371b, 0x20},
    {0x471c, 0x50},
    {0x3a13, 0x43}, // pre-gain = 1.047x
    {0x3a18, 0x00}, // gain ceiling
    {0x3a19, 0xf8}, // gain ceiling = 15.5x
    {0x3635, 0x13},
    {0x3636, 0x03},
    {0x3634, 0x40},
    {0x3622, 0x01},
    // 50/60Hz detection 50/60Hz 灯光条纹过滤
    {0x3c01, 0x34}, // Band auto, bit[7]
    {0x3c04, 0x28}, // threshold low sum
    {0x3c05, 0x98}, // threshold high sum
    {0x3c06, 0x00}, // light meter 1 threshold[15:8]
    {0x3c07, 0x08}, // light meter 1 threshold[7:0]
    {0x3c08, 0x00}, // light meter 2 threshold[15:8]
    {0x3c09, 0x1c}, // light meter 2 threshold[7:0]
    {0x3c0a, 0x9c}, // sample number[15:8]
    {0x3c0b, 0x40}, // sample number[7:0]
    {0x3810, 0x00}, // Timing Hoffset[11:8]
    {0x3811, 0x10}, // Timing Hoffset[7:0]
    {0x3812, 0x00}, // Timing Voffset[10:8]
    {0x3708, 0x64},
    {0x4001, 0x02}, // BLC start from line 2
    {0x4005, 0x1a}, // BLC always update
    {0x3000, 0x00}, // enable blocks
    {0x3004, 0xff}, // enable clocks
    {0x300e, 0x58}, // MIPI power down, DVP enable
    {0x302e, 0x00},
    {0x4300, 0x30}, // YUV 422, YUYV
    {0x501f, 0x00}, // YUV 422
    {0x440e, 0x00},
    {0x5000, 0xa7}, // Lenc on, raw gamma on, BPC on, WPC on, CIP on
    // AEC target 自动曝光控制
    {0x3a0f, 0x30}, // stable range in high
    {0x3a10, 0x28}, // stable range in low
    {0x3a1b, 0x30}, // stable range out high
    {0x3a1e, 0x26}, // stable range out low
    {0x3a11, 0x60}, // fast zone high
    {0x3a1f, 0x14}, // fast zone low
    // Lens correction for ? 镜头补偿
    {0x5800, 0x23},
    {0x5801, 0x14},
    {0x5802, 0x0f},
    {0x5803, 0x0f},
    {0x5804, 0x12},
    {0x5805, 0x26},
    {0x5806, 0x0c},
    {0x5807, 0x08},
    {0x5808, 0x05},
    {0x5809, 0x05},
    {0x580a, 0x08},

    {0x580b, 0x0d},
    {0x580c, 0x08},
    {0x580d, 0x03},
    {0x580e, 0x00},
    {0x580f, 0x00},
    {0x5810, 0x03},
    {0x5811, 0x09},
    {0x5812, 0x07},
    {0x5813, 0x03},
    {0x5814, 0x00},
    {0x5815, 0x01},
    {0x5816, 0x03},
    {0x5817, 0x08},
    {0x5818, 0x0d},
    {0x5819, 0x08},
    {0x581a, 0x05},
    {0x581b, 0x06},
    {0x581c, 0x08},
    {0x581d, 0x0e},
    {0x581e, 0x29},
    {0x581f, 0x17},
    {0x5820, 0x11},
    {0x5821, 0x11},
    {0x5822, 0x15},
    {0x5823, 0x28},
    {0x5824, 0x46},
    {0x5825, 0x26},
    {0x5826, 0x08},
    {0x5827, 0x26},
    {0x5828, 0x64},
    {0x5829, 0x26},
    {0x582a, 0x24},
    {0x582b, 0x22},
    {0x582c, 0x24},
    {0x582d, 0x24},
    {0x582e, 0x06},
    {0x582f, 0x22},
    {0x5830, 0x40},
    {0x5831, 0x42},
    {0x5832, 0x24},
    {0x5833, 0x26},
    {0x5834, 0x24},
    {0x5835, 0x22},
    {0x5836, 0x22},
    {0x5837, 0x26},
    {0x5838, 0x44},
    {0x5839, 0x24},
    {0x583a, 0x26},
    {0x583b, 0x28},
    {0x583c, 0x42},
    {0x583d, 0xce}, // lenc BR offset
    // AWB 自动白平衡
    {0x5180, 0xff}, // AWB B block
    {0x5181, 0xf2}, // AWB control
    {0x5182, 0x00}, // [7:4] max local counter, [3:0] max fast counter
    {0x5183, 0x14}, // AWB advanced
    {0x5184, 0x25},
    {0x5185, 0x24},
    {0x5186, 0x09},
    {0x5187, 0x09},
    {0x5188, 0x09},
    {0x5189, 0x75},
    {0x518a, 0x54},
    {0x518b, 0xe0},
    {0x518c, 0xb2},
    {0x518d, 0x42},
    {0x518e, 0x3d},
    {0x518f, 0x56},
    {0x5190, 0x46},
    {0x5191, 0xf8}, // AWB top limit
    {0x5192, 0x04}, // AWB bottom limit
    {0x5193, 0x70}, // red limit
    {0x5194, 0xf0}, // green limit
    {0x5195, 0xf0}, // blue limit
    {0x5196, 0x03}, // AWB control
    {0x5197, 0x01}, // local limit
    {0x5198, 0x04},
    {0x5199, 0x12},
    {0x519a, 0x04},
    {0x519b, 0x00},
    {0x519c, 0x06},
    {0x519d, 0x82},
    {0x519e, 0x38}, // AWB control
    // Gamma 伽玛曲线
    {0x5480, 0x01}, // Gamma bias plus on, bit[0]
    {0x5481, 0x08},
    {0x5482, 0x14},
    {0x5483, 0x28},
    {0x5484, 0x51},
    {0x5485, 0x65},
    {0x5486, 0x71},
    {0x5487, 0x7d},
    {0x5488, 0x87},
    {0x5489, 0x91},
    {0x548a, 0x9a},
    {0x548b, 0xaa},
    {0x548c, 0xb8},
    {0x548d, 0xcd},
    {0x548e, 0xdd},
    {0x548f, 0xea},
    {0x5490, 0x1d},
    // color matrix 色彩矩阵
    {0x5381, 0x1e}, // CMX1 for Y
    {0x5382, 0x5b}, // CMX2 for Y
    {0x5383, 0x08}, // CMX3 for Y
    {0x5384, 0x0a}, // CMX4 for U
    {0x5385, 0x7e}, // CMX5 for U
    {0x5386, 0x88}, // CMX6 for U
    {0x5387, 0x7c}, // CMX7 for V
    {0x5388, 0x6c}, // CMX8 for V
    {0x5389, 0x10}, // CMX9 for V
    {0x538a, 0x01}, // sign[9]
    {0x538b, 0x98}, // sign[8:1]
    // UV adjust UV 色彩饱和度调整
    {0x5580, 0x06}, // saturation on, bit[1]
    {0x5583, 0x40},
    {0x5584, 0x10},
    {0x5589, 0x10},
    {0x558a, 0x00},
    {0x558b, 0xf8},
    {0x501d, 0x40}, // enable manual offset of contrast
    // CIP 锐化和降噪
    {0x5300, 0x08}, // CIP sharpen MT threshold 1
    {0x5301, 0x30}, // CIP sharpen MT threshold 2
    {0x5302, 0x10}, // CIP sharpen MT offset 1
    {0x5303, 0x00}, // CIP sharpen MT offset 2
    {0x5304, 0x08}, // CIP DNS threshold 1
    {0x5305, 0x30}, // CIP DNS threshold 2
    {0x5306, 0x08}, // CIP DNS offset 1
    {0x5307, 0x16}, // CIP DNS offset 2
    {0x5309, 0x08}, // CIP sharpen TH threshold 1
    {0x530a, 0x30}, // CIP sharpen TH threshold 2
    {0x530b, 0x04}, // CIP sharpen TH offset 1
    {0x530c, 0x06}, // CIP sharpen TH offset 2
    {0x5025, 0x00},
    {0x3008, 0x02}, // wake up from standby, bit[6]
    // 自行添加的设置
    {0x4740, 0X21}, // VSYNC 高有效
};

static uint16_t OV5640_jpeg_reg_tbl[][2] = {
    {0x4300, 0x30}, // YUV 422, YUYV
    {0x501f, 0x00}, // YUV 422
    // Input clock = 24Mhz
    {0x3035, 0x21}, // PLL
    {0x3036, 0x69}, // PLL
    {0x3c07, 0x07}, // lightmeter 1 threshold[7:0]
    {0x3820, 0x46}, // flip
    {0x3821, 0x20}, // mirror
    {0x3814, 0x11}, // timing X inc
    {0x3815, 0x11}, // timing Y inc
    {0x3800, 0x00}, // HS
    {0x3801, 0x00}, // HS
    {0x3802, 0x00}, // VS
    {0x3803, 0x00}, // VS
    {0x3804, 0x0a}, // HW (HE)
    {0x3805, 0x3f}, // HW (HE)
    {0x3806, 0x07}, // VH (VE)
    {0x3807, 0x9f}, // VH (VE)

    {0x3808, 0x02}, // DVPHO
    {0x3809, 0x80}, // DVPHO
    {0x380a, 0x01}, // DVPVO
    {0x380b, 0xe0}, // DVPVO

    {0x380c, 0x0b}, // HTS 		//
    {0x380d, 0x1c}, // HTS
    {0x380e, 0x07}, // VTS 		//
    {0x380f, 0xb0}, // VTS
    {0x3813, 0x04}, // timing V offset   04
    {0x3618, 0x04},
    {0x3612, 0x2b},
    {0x3709, 0x12},
    {0x370c, 0x00},

    {0x4004, 0x06}, // BLC line number
    {0x3002, 0x00}, // enable JFIFO, SFIFO, JPG
    {0x3006, 0xff}, // enable clock of JPEG2x, JPEG
    {0x4713, 0x03}, // JPEG mode 3
    {0x4407, 0x01}, // Quantization sacle
    {0x460b, 0x35},
    {0x460c, 0x22},
    {0x4837, 0x16}, // MIPI global timing
    {0x3824, 0x02}, // PCLK manual divider
    {0x5001, 0xA3}, // SDE on, Scaling on, CMX on, AWB on
    {0x3503, 0x00}, // AEC/AGC on
};

static uint16_t ov5640_rgb565_reg_tbl[][2] = {
    {0x4300, 0X6F},
    {0X501F, 0x01},
    // 1280x800, 15fps
    // input clock 24Mhz, PCLK 42Mhz
    {0x3035, 0x41}, // PLL
    {0x3036, 0x69}, // PLL
    {0x3c07, 0x07}, // lightmeter 1 threshold[7:0]
    {0x3820, 0x46}, // flip
    {0x3821, 0x00}, // mirror
    {0x3814, 0x31}, // timing X inc
    {0x3815, 0x31}, // timing Y inc
    {0x3800, 0x00}, // HS
    {0x3801, 0x00}, // HS
    {0x3802, 0x00}, // VS
    {0x3803, 0x00}, // VS
    {0x3804, 0x0a}, // HW (HE)
    {0x3805, 0x3f}, // HW (HE)
    {0x3806, 0x06}, // VH (VE)
    {0x3807, 0xa9}, // VH (VE)
    {0x3808, 0x05}, // DVPHO
    {0x3809, 0x00}, // DVPHO
    {0x380a, 0x02}, // DVPVO
    {0x380b, 0xd0}, // DVPVO
    {0x380c, 0x05}, // HTS
    {0x380d, 0xF8}, // HTS
    {0x380e, 0x03}, // VTS
    {0x380f, 0x84}, // VTS
    {0x3813, 0x04}, // timing V offset
    {0x3618, 0x00},
    {0x3612, 0x29},
    {0x3709, 0x52},
    {0x370c, 0x03},
    {0x3a02, 0x02}, // 60Hz max exposure
    {0x3a03, 0xe0}, // 60Hz max exposure

    {0x3a14, 0x02}, // 50Hz max exposure
    {0x3a15, 0xe0}, // 50Hz max exposure
    {0x4004, 0x02}, // BLC line number
    {0x3002, 0x1c}, // reset JFIFO, SFIFO, JPG
    {0x3006, 0xc3}, // disable clock of JPEG2x, JPEG
    {0x4713, 0x03}, // JPEG mode 3
    {0x4407, 0x04}, // Quantization scale
    {0x460b, 0x37},
    {0x460c, 0x20},
    {0x4837, 0x16}, // MIPI global timing
    {0x3824, 0x04}, // PCLK manual divider
    {0x5001, 0xA3}, // SDE on, scale on, UV average off, color matrix on, AWB on
    {0x3503, 0x00}, // AEC/AGC on
};

uint16_t solution_table[][2] = {
    { 160,  120}, // QQVGA
    { 320,  240}, // QVGA
    { 480,  272}, // PSP
    { 640,  480}, // VGA
    { 800,  480}, // WVGA
    { 800,  600}, // SVGA
    {1024,  768}, // XGA
    {1280,  800}, // WXGA
    {1440,  900}, // WXGA+
    {1280, 1024}, // SXGA
    {1600, 1200}, // UXGA
    {1920, 1080}, // 1080P
    {2048, 1536}, // QXGA
    {2100, 1575}, // 500W
};



const uint8_t OV5640_AF_Config[] = {
    0x02, 0x0f, 0xd6, 0x02, 0x0a, 0x39, 0xc2, 0x01, 0x22, 0x22, 0x00, 0x02, 0x0f, 0xb2, 0xe5, 0x1f, // 0x8000,
    0x70, 0x72, 0xf5, 0x1e, 0xd2, 0x35, 0xff, 0xef, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xe4, 0xf6, 0x08, // 0x8010,
    0xf6, 0x0f, 0xbf, 0x34, 0xf2, 0x90, 0x0e, 0x93, 0xe4, 0x93, 0xff, 0xe5, 0x4b, 0xc3, 0x9f, 0x50, // 0x8020,
    0x04, 0x7f, 0x05, 0x80, 0x02, 0x7f, 0xfb, 0x78, 0xbd, 0xa6, 0x07, 0x12, 0x0f, 0x04, 0x40, 0x04, // 0x8030,
    0x7f, 0x03, 0x80, 0x02, 0x7f, 0x30, 0x78, 0xbc, 0xa6, 0x07, 0xe6, 0x18, 0xf6, 0x08, 0xe6, 0x78, // 0x8040,
    0xb9, 0xf6, 0x78, 0xbc, 0xe6, 0x78, 0xba, 0xf6, 0x78, 0xbf, 0x76, 0x33, 0xe4, 0x08, 0xf6, 0x78, // 0x8050,
    0xb8, 0x76, 0x01, 0x75, 0x4a, 0x02, 0x78, 0xb6, 0xf6, 0x08, 0xf6, 0x74, 0xff, 0x78, 0xc1, 0xf6, // 0x8060,
    0x08, 0xf6, 0x75, 0x1f, 0x01, 0x78, 0xbc, 0xe6, 0x75, 0xf0, 0x05, 0xa4, 0xf5, 0x4b, 0x12, 0x0a, // 0x8070,
    0xff, 0xc2, 0x37, 0x22, 0x78, 0xb8, 0xe6, 0xd3, 0x94, 0x00, 0x40, 0x02, 0x16, 0x22, 0xe5, 0x1f, // 0x8080,
    0xb4, 0x05, 0x23, 0xe4, 0xf5, 0x1f, 0xc2, 0x01, 0x78, 0xb6, 0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x78, // 0x8090,
    0x4e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0xa2, 0x37, 0xe4, 0x33, 0xf5, 0x3c, 0x90, 0x30, 0x28, 0xf0, // 0x80a0,
    0x75, 0x1e, 0x10, 0xd2, 0x35, 0x22, 0xe5, 0x4b, 0x75, 0xf0, 0x05, 0x84, 0x78, 0xbc, 0xf6, 0x90, // 0x80b0,
    0x0e, 0x8c, 0xe4, 0x93, 0xff, 0x25, 0xe0, 0x24, 0x0a, 0xf8, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x78, // 0x80c0,
    0xbc, 0xe6, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0xef, 0x12, 0x0f, 0x0b, // 0x80d0,
    0xd3, 0x78, 0xb7, 0x96, 0xee, 0x18, 0x96, 0x40, 0x0d, 0x78, 0xbc, 0xe6, 0x78, 0xb9, 0xf6, 0x78, // 0x80e0,
    0xb6, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x90, 0x0e, 0x8c, 0xe4, 0x93, 0x12, 0x0f, 0x0b, 0xc3, 0x78, // 0x80f0,
    0xc2, 0x96, 0xee, 0x18, 0x96, 0x50, 0x0d, 0x78, 0xbc, 0xe6, 0x78, 0xba, 0xf6, 0x78, 0xc1, 0xa6, // 0x8100,
    0x06, 0x08, 0xa6, 0x07, 0x78, 0xb6, 0xe6, 0xfe, 0x08, 0xe6, 0xc3, 0x78, 0xc2, 0x96, 0xff, 0xee, // 0x8110,
    0x18, 0x96, 0x78, 0xc3, 0xf6, 0x08, 0xa6, 0x07, 0x90, 0x0e, 0x95, 0xe4, 0x18, 0x12, 0x0e, 0xe9, // 0x8120,
    0x40, 0x02, 0xd2, 0x37, 0x78, 0xbc, 0xe6, 0x08, 0x26, 0x08, 0xf6, 0xe5, 0x1f, 0x64, 0x01, 0x70, // 0x8130,
    0x4a, 0xe6, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xdf, 0x40, 0x05, 0x12, 0x0e, 0xda, 0x40, 0x39, 0x12, // 0x8140,
    0x0f, 0x02, 0x40, 0x04, 0x7f, 0xfe, 0x80, 0x02, 0x7f, 0x02, 0x78, 0xbd, 0xa6, 0x07, 0x78, 0xb9, // 0x8150,
    0xe6, 0x24, 0x03, 0x78, 0xbf, 0xf6, 0x78, 0xb9, 0xe6, 0x24, 0xfd, 0x78, 0xc0, 0xf6, 0x12, 0x0f, // 0x8160,
    0x02, 0x40, 0x06, 0x78, 0xc0, 0xe6, 0xff, 0x80, 0x04, 0x78, 0xbf, 0xe6, 0xff, 0x78, 0xbe, 0xa6, // 0x8170,
    0x07, 0x75, 0x1f, 0x02, 0x78, 0xb8, 0x76, 0x01, 0x02, 0x02, 0x4a, 0xe5, 0x1f, 0x64, 0x02, 0x60, // 0x8180,
    0x03, 0x02, 0x02, 0x2a, 0x78, 0xbe, 0xe6, 0xff, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xe0, 0x40, 0x08, // 0x8190,
    0x12, 0x0e, 0xda, 0x50, 0x03, 0x02, 0x02, 0x28, 0x12, 0x0f, 0x02, 0x40, 0x04, 0x7f, 0xff, 0x80, // 0x81a0,
    0x02, 0x7f, 0x01, 0x78, 0xbd, 0xa6, 0x07, 0x78, 0xb9, 0xe6, 0x04, 0x78, 0xbf, 0xf6, 0x78, 0xb9, // 0x81b0,
    0xe6, 0x14, 0x78, 0xc0, 0xf6, 0x18, 0x12, 0x0f, 0x04, 0x40, 0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, // 0x81c0,
    0x00, 0x78, 0xbf, 0xa6, 0x07, 0xd3, 0x08, 0xe6, 0x64, 0x80, 0x94, 0x80, 0x40, 0x04, 0xe6, 0xff, // 0x81d0,
    0x80, 0x02, 0x7f, 0x00, 0x78, 0xc0, 0xa6, 0x07, 0xc3, 0x18, 0xe6, 0x64, 0x80, 0x94, 0xb3, 0x50, // 0x81e0,
    0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, 0x33, 0x78, 0xbf, 0xa6, 0x07, 0xc3, 0x08, 0xe6, 0x64, 0x80, // 0x81f0,
    0x94, 0xb3, 0x50, 0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, 0x33, 0x78, 0xc0, 0xa6, 0x07, 0x12, 0x0f, // 0x8200,
    0x02, 0x40, 0x06, 0x78, 0xc0, 0xe6, 0xff, 0x80, 0x04, 0x78, 0xbf, 0xe6, 0xff, 0x78, 0xbe, 0xa6, // 0x8210,
    0x07, 0x75, 0x1f, 0x03, 0x78, 0xb8, 0x76, 0x01, 0x80, 0x20, 0xe5, 0x1f, 0x64, 0x03, 0x70, 0x26, // 0x8220,
    0x78, 0xbe, 0xe6, 0xff, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xe0, 0x40, 0x05, 0x12, 0x0e, 0xda, 0x40, // 0x8230,
    0x09, 0x78, 0xb9, 0xe6, 0x78, 0xbe, 0xf6, 0x75, 0x1f, 0x04, 0x78, 0xbe, 0xe6, 0x75, 0xf0, 0x05, // 0x8240,
    0xa4, 0xf5, 0x4b, 0x02, 0x0a, 0xff, 0xe5, 0x1f, 0xb4, 0x04, 0x10, 0x90, 0x0e, 0x94, 0xe4, 0x78, // 0x8250,
    0xc3, 0x12, 0x0e, 0xe9, 0x40, 0x02, 0xd2, 0x37, 0x75, 0x1f, 0x05, 0x22, 0x30, 0x01, 0x03, 0x02, // 0x8260,
    0x04, 0xc0, 0x30, 0x02, 0x03, 0x02, 0x04, 0xc0, 0x90, 0x51, 0xa5, 0xe0, 0x78, 0x93, 0xf6, 0xa3, // 0x8270,
    0xe0, 0x08, 0xf6, 0xa3, 0xe0, 0x08, 0xf6, 0xe5, 0x1f, 0x70, 0x3c, 0x75, 0x1e, 0x20, 0xd2, 0x35, // 0x8280,
    0x12, 0x0c, 0x7a, 0x78, 0x7e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8b, 0xa6, 0x09, 0x18, 0x76, // 0x8290,
    0x01, 0x12, 0x0c, 0x5b, 0x78, 0x4e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8b, 0xe6, 0x78, 0x6e, // 0x82a0,
    0xf6, 0x75, 0x1f, 0x01, 0x78, 0x93, 0xe6, 0x78, 0x90, 0xf6, 0x78, 0x94, 0xe6, 0x78, 0x91, 0xf6, // 0x82b0,
    0x78, 0x95, 0xe6, 0x78, 0x92, 0xf6, 0x22, 0x79, 0x90, 0xe7, 0xd3, 0x78, 0x93, 0x96, 0x40, 0x05, // 0x82c0,
    0xe7, 0x96, 0xff, 0x80, 0x08, 0xc3, 0x79, 0x93, 0xe7, 0x78, 0x90, 0x96, 0xff, 0x78, 0x88, 0x76, // 0x82d0,
    0x00, 0x08, 0xa6, 0x07, 0x79, 0x91, 0xe7, 0xd3, 0x78, 0x94, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, // 0x82e0,
    0x80, 0x08, 0xc3, 0x79, 0x94, 0xe7, 0x78, 0x91, 0x96, 0xff, 0x12, 0x0c, 0x8e, 0x79, 0x92, 0xe7, // 0x82f0,
    0xd3, 0x78, 0x95, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, 0x80, 0x08, 0xc3, 0x79, 0x95, 0xe7, 0x78, // 0x8300,
    0x92, 0x96, 0xff, 0x12, 0x0c, 0x8e, 0x12, 0x0c, 0x5b, 0x78, 0x8a, 0xe6, 0x25, 0xe0, 0x24, 0x4e, // 0x8310,
    0xf8, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8a, 0xe6, 0x24, 0x6e, 0xf8, 0xa6, 0x09, 0x78, 0x8a, // 0x8320,
    0xe6, 0x24, 0x01, 0xff, 0xe4, 0x33, 0xfe, 0xd3, 0xef, 0x94, 0x0f, 0xee, 0x64, 0x80, 0x94, 0x80, // 0x8330,
    0x40, 0x04, 0x7f, 0x00, 0x80, 0x05, 0x78, 0x8a, 0xe6, 0x04, 0xff, 0x78, 0x8a, 0xa6, 0x07, 0xe5, // 0x8340,
    0x1f, 0xb4, 0x01, 0x0a, 0xe6, 0x60, 0x03, 0x02, 0x04, 0xc0, 0x75, 0x1f, 0x02, 0x22, 0x12, 0x0c, // 0x8350,
    0x7a, 0x78, 0x80, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x12, 0x0c, 0x7a, 0x78, 0x82, 0xa6, 0x06, 0x08, // 0x8360,
    0xa6, 0x07, 0x78, 0x6e, 0xe6, 0x78, 0x8c, 0xf6, 0x78, 0x6e, 0xe6, 0x78, 0x8d, 0xf6, 0x7f, 0x01, // 0x8370,
    0xef, 0x25, 0xe0, 0x24, 0x4f, 0xf9, 0xc3, 0x78, 0x81, 0xe6, 0x97, 0x18, 0xe6, 0x19, 0x97, 0x50, // 0x8380,
    0x0a, 0x12, 0x0c, 0x82, 0x78, 0x80, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0x74, 0x6e, 0x2f, 0xf9, 0x78, // 0x8390,
    0x8c, 0xe6, 0xc3, 0x97, 0x50, 0x08, 0x74, 0x6e, 0x2f, 0xf8, 0xe6, 0x78, 0x8c, 0xf6, 0xef, 0x25, // 0x83a0,
    0xe0, 0x24, 0x4f, 0xf9, 0xd3, 0x78, 0x83, 0xe6, 0x97, 0x18, 0xe6, 0x19, 0x97, 0x40, 0x0a, 0x12, // 0x83b0,
    0x0c, 0x82, 0x78, 0x82, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0x74, 0x6e, 0x2f, 0xf9, 0x78, 0x8d, 0xe6, // 0x83c0,
    0xd3, 0x97, 0x40, 0x08, 0x74, 0x6e, 0x2f, 0xf8, 0xe6, 0x78, 0x8d, 0xf6, 0x0f, 0xef, 0x64, 0x10, // 0x83d0,
    0x70, 0x9e, 0xc3, 0x79, 0x81, 0xe7, 0x78, 0x83, 0x96, 0xff, 0x19, 0xe7, 0x18, 0x96, 0x78, 0x84, // 0x83e0,
    0xf6, 0x08, 0xa6, 0x07, 0xc3, 0x79, 0x8c, 0xe7, 0x78, 0x8d, 0x96, 0x08, 0xf6, 0xd3, 0x79, 0x81, // 0x83f0,
    0xe7, 0x78, 0x7f, 0x96, 0x19, 0xe7, 0x18, 0x96, 0x40, 0x05, 0x09, 0xe7, 0x08, 0x80, 0x06, 0xc3, // 0x8400,
    0x79, 0x7f, 0xe7, 0x78, 0x81, 0x96, 0xff, 0x19, 0xe7, 0x18, 0x96, 0xfe, 0x78, 0x86, 0xa6, 0x06, // 0x8410,
    0x08, 0xa6, 0x07, 0x79, 0x8c, 0xe7, 0xd3, 0x78, 0x8b, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, 0x80, // 0x8420,
    0x08, 0xc3, 0x79, 0x8b, 0xe7, 0x78, 0x8c, 0x96, 0xff, 0x78, 0x8f, 0xa6, 0x07, 0xe5, 0x1f, 0x64, // 0x8430,
    0x02, 0x70, 0x69, 0x90, 0x0e, 0x91, 0x93, 0xff, 0x18, 0xe6, 0xc3, 0x9f, 0x50, 0x72, 0x12, 0x0c, // 0x8440,
    0x4a, 0x12, 0x0c, 0x2f, 0x90, 0x0e, 0x8e, 0x12, 0x0c, 0x38, 0x78, 0x80, 0x12, 0x0c, 0x6b, 0x7b, // 0x8450,
    0x04, 0x12, 0x0c, 0x1d, 0xc3, 0x12, 0x06, 0x45, 0x50, 0x56, 0x90, 0x0e, 0x92, 0xe4, 0x93, 0xff, // 0x8460,
    0x78, 0x8f, 0xe6, 0x9f, 0x40, 0x02, 0x80, 0x11, 0x90, 0x0e, 0x90, 0xe4, 0x93, 0xff, 0xd3, 0x78, // 0x8470,
    0x89, 0xe6, 0x9f, 0x18, 0xe6, 0x94, 0x00, 0x40, 0x03, 0x75, 0x1f, 0x05, 0x12, 0x0c, 0x4a, 0x12, // 0x8480,
    0x0c, 0x2f, 0x90, 0x0e, 0x8f, 0x12, 0x0c, 0x38, 0x78, 0x7e, 0x12, 0x0c, 0x6b, 0x7b, 0x40, 0x12, // 0x8490,
    0x0c, 0x1d, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x18, 0x75, 0x1f, 0x05, 0x22, 0xe5, 0x1f, 0xb4, 0x05, // 0x84a0,
    0x0f, 0xd2, 0x01, 0xc2, 0x02, 0xe4, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, 0x33, 0xd2, 0x36, // 0x84b0,
    0x22, 0xef, 0x8d, 0xf0, 0xa4, 0xa8, 0xf0, 0xcf, 0x8c, 0xf0, 0xa4, 0x28, 0xce, 0x8d, 0xf0, 0xa4, // 0x84c0,
    0x2e, 0xfe, 0x22, 0xbc, 0x00, 0x0b, 0xbe, 0x00, 0x29, 0xef, 0x8d, 0xf0, 0x84, 0xff, 0xad, 0xf0, // 0x84d0,
    0x22, 0xe4, 0xcc, 0xf8, 0x75, 0xf0, 0x08, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, 0xec, 0x33, 0xfc, // 0x84e0,
    0xee, 0x9d, 0xec, 0x98, 0x40, 0x05, 0xfc, 0xee, 0x9d, 0xfe, 0x0f, 0xd5, 0xf0, 0xe9, 0xe4, 0xce, // 0x84f0,
    0xfd, 0x22, 0xed, 0xf8, 0xf5, 0xf0, 0xee, 0x84, 0x20, 0xd2, 0x1c, 0xfe, 0xad, 0xf0, 0x75, 0xf0, // 0x8500,
    0x08, 0xef, 0x2f, 0xff, 0xed, 0x33, 0xfd, 0x40, 0x07, 0x98, 0x50, 0x06, 0xd5, 0xf0, 0xf2, 0x22, // 0x8510,
    0xc3, 0x98, 0xfd, 0x0f, 0xd5, 0xf0, 0xea, 0x22, 0xe8, 0x8f, 0xf0, 0xa4, 0xcc, 0x8b, 0xf0, 0xa4, // 0x8520,
    0x2c, 0xfc, 0xe9, 0x8e, 0xf0, 0xa4, 0x2c, 0xfc, 0x8a, 0xf0, 0xed, 0xa4, 0x2c, 0xfc, 0xea, 0x8e, // 0x8530,
    0xf0, 0xa4, 0xcd, 0xa8, 0xf0, 0x8b, 0xf0, 0xa4, 0x2d, 0xcc, 0x38, 0x25, 0xf0, 0xfd, 0xe9, 0x8f, // 0x8540,
    0xf0, 0xa4, 0x2c, 0xcd, 0x35, 0xf0, 0xfc, 0xeb, 0x8e, 0xf0, 0xa4, 0xfe, 0xa9, 0xf0, 0xeb, 0x8f, // 0x8550,
    0xf0, 0xa4, 0xcf, 0xc5, 0xf0, 0x2e, 0xcd, 0x39, 0xfe, 0xe4, 0x3c, 0xfc, 0xea, 0xa4, 0x2d, 0xce, // 0x8560,
    0x35, 0xf0, 0xfd, 0xe4, 0x3c, 0xfc, 0x22, 0x75, 0xf0, 0x08, 0x75, 0x82, 0x00, 0xef, 0x2f, 0xff, // 0x8570,
    0xee, 0x33, 0xfe, 0xcd, 0x33, 0xcd, 0xcc, 0x33, 0xcc, 0xc5, 0x82, 0x33, 0xc5, 0x82, 0x9b, 0xed, // 0x8580,
    0x9a, 0xec, 0x99, 0xe5, 0x82, 0x98, 0x40, 0x0c, 0xf5, 0x82, 0xee, 0x9b, 0xfe, 0xed, 0x9a, 0xfd, // 0x8590,
    0xec, 0x99, 0xfc, 0x0f, 0xd5, 0xf0, 0xd6, 0xe4, 0xce, 0xfb, 0xe4, 0xcd, 0xfa, 0xe4, 0xcc, 0xf9, // 0x85a0,
    0xa8, 0x82, 0x22, 0xb8, 0x00, 0xc1, 0xb9, 0x00, 0x59, 0xba, 0x00, 0x2d, 0xec, 0x8b, 0xf0, 0x84, // 0x85b0,
    0xcf, 0xce, 0xcd, 0xfc, 0xe5, 0xf0, 0xcb, 0xf9, 0x78, 0x18, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, // 0x85c0,
    0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xeb, 0x33, 0xfb, 0x10, 0xd7, 0x03, 0x99, 0x40, 0x04, 0xeb, // 0x85d0,
    0x99, 0xfb, 0x0f, 0xd8, 0xe5, 0xe4, 0xf9, 0xfa, 0x22, 0x78, 0x18, 0xef, 0x2f, 0xff, 0xee, 0x33, // 0x85e0,
    0xfe, 0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xc9, 0x33, 0xc9, 0x10, 0xd7, 0x05, 0x9b, 0xe9, 0x9a, // 0x85f0,
    0x40, 0x07, 0xec, 0x9b, 0xfc, 0xe9, 0x9a, 0xf9, 0x0f, 0xd8, 0xe0, 0xe4, 0xc9, 0xfa, 0xe4, 0xcc, // 0x8600,
    0xfb, 0x22, 0x75, 0xf0, 0x10, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, 0xed, 0x33, 0xfd, 0xcc, 0x33, // 0x8610,
    0xcc, 0xc8, 0x33, 0xc8, 0x10, 0xd7, 0x07, 0x9b, 0xec, 0x9a, 0xe8, 0x99, 0x40, 0x0a, 0xed, 0x9b, // 0x8620,
    0xfd, 0xec, 0x9a, 0xfc, 0xe8, 0x99, 0xf8, 0x0f, 0xd5, 0xf0, 0xda, 0xe4, 0xcd, 0xfb, 0xe4, 0xcc, // 0x8630,
    0xfa, 0xe4, 0xc8, 0xf9, 0x22, 0xeb, 0x9f, 0xf5, 0xf0, 0xea, 0x9e, 0x42, 0xf0, 0xe9, 0x9d, 0x42, // 0x8640,
    0xf0, 0xe8, 0x9c, 0x45, 0xf0, 0x22, 0xe8, 0x60, 0x0f, 0xec, 0xc3, 0x13, 0xfc, 0xed, 0x13, 0xfd, // 0x8650,
    0xee, 0x13, 0xfe, 0xef, 0x13, 0xff, 0xd8, 0xf1, 0x22, 0xe8, 0x60, 0x0f, 0xef, 0xc3, 0x33, 0xff, // 0x8660,
    0xee, 0x33, 0xfe, 0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xd8, 0xf1, 0x22, 0xe4, 0x93, 0xfc, 0x74, // 0x8670,
    0x01, 0x93, 0xfd, 0x74, 0x02, 0x93, 0xfe, 0x74, 0x03, 0x93, 0xff, 0x22, 0xe6, 0xfb, 0x08, 0xe6, // 0x8680,
    0xf9, 0x08, 0xe6, 0xfa, 0x08, 0xe6, 0xcb, 0xf8, 0x22, 0xec, 0xf6, 0x08, 0xed, 0xf6, 0x08, 0xee, // 0x8690,
    0xf6, 0x08, 0xef, 0xf6, 0x22, 0xa4, 0x25, 0x82, 0xf5, 0x82, 0xe5, 0xf0, 0x35, 0x83, 0xf5, 0x83, // 0x86a0,
    0x22, 0xd0, 0x83, 0xd0, 0x82, 0xf8, 0xe4, 0x93, 0x70, 0x12, 0x74, 0x01, 0x93, 0x70, 0x0d, 0xa3, // 0x86b0,
    0xa3, 0x93, 0xf8, 0x74, 0x01, 0x93, 0xf5, 0x82, 0x88, 0x83, 0xe4, 0x73, 0x74, 0x02, 0x93, 0x68, // 0x86c0,
    0x60, 0xef, 0xa3, 0xa3, 0xa3, 0x80, 0xdf, 0x90, 0x38, 0x04, 0x78, 0x52, 0x12, 0x0b, 0xfd, 0x90, // 0x86d0,
    0x38, 0x00, 0xe0, 0xfe, 0xa3, 0xe0, 0xfd, 0xed, 0xff, 0xc3, 0x12, 0x0b, 0x9e, 0x90, 0x38, 0x10, // 0x86e0,
    0x12, 0x0b, 0x92, 0x90, 0x38, 0x06, 0x78, 0x54, 0x12, 0x0b, 0xfd, 0x90, 0x38, 0x02, 0xe0, 0xfe, // 0x86f0,
    0xa3, 0xe0, 0xfd, 0xed, 0xff, 0xc3, 0x12, 0x0b, 0x9e, 0x90, 0x38, 0x12, 0x12, 0x0b, 0x92, 0xa3, // 0x8700,
    0xe0, 0xb4, 0x31, 0x07, 0x78, 0x52, 0x79, 0x52, 0x12, 0x0c, 0x13, 0x90, 0x38, 0x14, 0xe0, 0xb4, // 0x8710,
    0x71, 0x15, 0x78, 0x52, 0xe6, 0xfe, 0x08, 0xe6, 0x78, 0x02, 0xce, 0xc3, 0x13, 0xce, 0x13, 0xd8, // 0x8720,
    0xf9, 0x79, 0x53, 0xf7, 0xee, 0x19, 0xf7, 0x90, 0x38, 0x15, 0xe0, 0xb4, 0x31, 0x07, 0x78, 0x54, // 0x8730,
    0x79, 0x54, 0x12, 0x0c, 0x13, 0x90, 0x38, 0x15, 0xe0, 0xb4, 0x71, 0x15, 0x78, 0x54, 0xe6, 0xfe, // 0x8740,
    0x08, 0xe6, 0x78, 0x02, 0xce, 0xc3, 0x13, 0xce, 0x13, 0xd8, 0xf9, 0x79, 0x55, 0xf7, 0xee, 0x19, // 0x8750,
    0xf7, 0x79, 0x52, 0x12, 0x0b, 0xd9, 0x09, 0x12, 0x0b, 0xd9, 0xaf, 0x47, 0x12, 0x0b, 0xb2, 0xe5, // 0x8760,
    0x44, 0xfb, 0x7a, 0x00, 0xfd, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x5a, 0xa6, 0x06, 0x08, 0xa6, // 0x8770,
    0x07, 0xaf, 0x45, 0x12, 0x0b, 0xb2, 0xad, 0x03, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x56, 0xa6, // 0x8780,
    0x06, 0x08, 0xa6, 0x07, 0xaf, 0x48, 0x78, 0x54, 0x12, 0x0b, 0xb4, 0xe5, 0x43, 0xfb, 0xfd, 0x7c, // 0x8790,
    0x00, 0x12, 0x04, 0xd3, 0x78, 0x5c, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0xaf, 0x46, 0x7e, 0x00, 0x78, // 0x87a0,
    0x54, 0x12, 0x0b, 0xb6, 0xad, 0x03, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x58, 0xa6, 0x06, 0x08, // 0x87b0,
    0xa6, 0x07, 0xc3, 0x78, 0x5b, 0xe6, 0x94, 0x08, 0x18, 0xe6, 0x94, 0x00, 0x50, 0x05, 0x76, 0x00, // 0x87c0,
    0x08, 0x76, 0x08, 0xc3, 0x78, 0x5d, 0xe6, 0x94, 0x08, 0x18, 0xe6, 0x94, 0x00, 0x50, 0x05, 0x76, // 0x87d0,
    0x00, 0x08, 0x76, 0x08, 0x78, 0x5a, 0x12, 0x0b, 0xc6, 0xff, 0xd3, 0x78, 0x57, 0xe6, 0x9f, 0x18, // 0x87e0,
    0xe6, 0x9e, 0x40, 0x0e, 0x78, 0x5a, 0xe6, 0x13, 0xfe, 0x08, 0xe6, 0x78, 0x57, 0x12, 0x0c, 0x08, // 0x87f0,
    0x80, 0x04, 0x7e, 0x00, 0x7f, 0x00, 0x78, 0x5e, 0x12, 0x0b, 0xbe, 0xff, 0xd3, 0x78, 0x59, 0xe6, // 0x8800,
    0x9f, 0x18, 0xe6, 0x9e, 0x40, 0x0e, 0x78, 0x5c, 0xe6, 0x13, 0xfe, 0x08, 0xe6, 0x78, 0x59, 0x12, // 0x8810,
    0x0c, 0x08, 0x80, 0x04, 0x7e, 0x00, 0x7f, 0x00, 0xe4, 0xfc, 0xfd, 0x78, 0x62, 0x12, 0x06, 0x99, // 0x8820,
    0x78, 0x5a, 0x12, 0x0b, 0xc6, 0x78, 0x57, 0x26, 0xff, 0xee, 0x18, 0x36, 0xfe, 0x78, 0x66, 0x12, // 0x8830,
    0x0b, 0xbe, 0x78, 0x59, 0x26, 0xff, 0xee, 0x18, 0x36, 0xfe, 0xe4, 0xfc, 0xfd, 0x78, 0x6a, 0x12, // 0x8840,
    0x06, 0x99, 0x12, 0x0b, 0xce, 0x78, 0x66, 0x12, 0x06, 0x8c, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x08, // 0x8850,
    0x12, 0x0b, 0xce, 0x78, 0x66, 0x12, 0x06, 0x99, 0x78, 0x54, 0x12, 0x0b, 0xd0, 0x78, 0x6a, 0x12, // 0x8860,
    0x06, 0x8c, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x0a, 0x78, 0x54, 0x12, 0x0b, 0xd0, 0x78, 0x6a, 0x12, // 0x8870,
    0x06, 0x99, 0x78, 0x61, 0xe6, 0x90, 0x60, 0x01, 0xf0, 0x78, 0x65, 0xe6, 0xa3, 0xf0, 0x78, 0x69, // 0x8880,
    0xe6, 0xa3, 0xf0, 0x78, 0x55, 0xe6, 0xa3, 0xf0, 0x7d, 0x01, 0x78, 0x61, 0x12, 0x0b, 0xe9, 0x24, // 0x8890,
    0x01, 0x12, 0x0b, 0xa6, 0x78, 0x65, 0x12, 0x0b, 0xe9, 0x24, 0x02, 0x12, 0x0b, 0xa6, 0x78, 0x69, // 0x88a0,
    0x12, 0x0b, 0xe9, 0x24, 0x03, 0x12, 0x0b, 0xa6, 0x78, 0x6d, 0x12, 0x0b, 0xe9, 0x24, 0x04, 0x12, // 0x88b0,
    0x0b, 0xa6, 0x0d, 0xbd, 0x05, 0xd4, 0xc2, 0x0e, 0xc2, 0x06, 0x22, 0x85, 0x08, 0x41, 0x90, 0x30, // 0x88c0,
    0x24, 0xe0, 0xf5, 0x3d, 0xa3, 0xe0, 0xf5, 0x3e, 0xa3, 0xe0, 0xf5, 0x3f, 0xa3, 0xe0, 0xf5, 0x40, // 0x88d0,
    0xa3, 0xe0, 0xf5, 0x3c, 0xd2, 0x34, 0xe5, 0x41, 0x12, 0x06, 0xb1, 0x09, 0x31, 0x03, 0x09, 0x35, // 0x88e0,
    0x04, 0x09, 0x3b, 0x05, 0x09, 0x3e, 0x06, 0x09, 0x41, 0x07, 0x09, 0x4a, 0x08, 0x09, 0x5b, 0x12, // 0x88f0,
    0x09, 0x73, 0x18, 0x09, 0x89, 0x19, 0x09, 0x5e, 0x1a, 0x09, 0x6a, 0x1b, 0x09, 0xad, 0x80, 0x09, // 0x8900,
    0xb2, 0x81, 0x0a, 0x1d, 0x8f, 0x0a, 0x09, 0x90, 0x0a, 0x1d, 0x91, 0x0a, 0x1d, 0x92, 0x0a, 0x1d, // 0x8910,
    0x93, 0x0a, 0x1d, 0x94, 0x0a, 0x1d, 0x98, 0x0a, 0x17, 0x9f, 0x0a, 0x1a, 0xec, 0x00, 0x00, 0x0a, // 0x8920,
    0x38, 0x12, 0x0f, 0x74, 0x22, 0x12, 0x0f, 0x74, 0xd2, 0x03, 0x22, 0xd2, 0x03, 0x22, 0xc2, 0x03, // 0x8930,
    0x22, 0xa2, 0x37, 0xe4, 0x33, 0xf5, 0x3c, 0x02, 0x0a, 0x1d, 0xc2, 0x01, 0xc2, 0x02, 0xc2, 0x03, // 0x8940,
    0x12, 0x0d, 0x0d, 0x75, 0x1e, 0x70, 0xd2, 0x35, 0x02, 0x0a, 0x1d, 0x02, 0x0a, 0x04, 0x85, 0x40, // 0x8950,
    0x4a, 0x85, 0x3c, 0x4b, 0x12, 0x0a, 0xff, 0x02, 0x0a, 0x1d, 0x85, 0x4a, 0x40, 0x85, 0x4b, 0x3c, // 0x8960,
    0x02, 0x0a, 0x1d, 0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x85, 0x40, 0x31, 0x85, 0x3f, 0x30, 0x85, 0x3e, // 0x8970,
    0x2f, 0x85, 0x3d, 0x2e, 0x12, 0x0f, 0x46, 0x80, 0x1f, 0x75, 0x22, 0x00, 0x75, 0x23, 0x01, 0x74, // 0x8980,
    0xff, 0xf5, 0x2d, 0xf5, 0x2c, 0xf5, 0x2b, 0xf5, 0x2a, 0x12, 0x0f, 0x46, 0x85, 0x2d, 0x40, 0x85, // 0x8990,
    0x2c, 0x3f, 0x85, 0x2b, 0x3e, 0x85, 0x2a, 0x3d, 0xe4, 0xf5, 0x3c, 0x80, 0x70, 0x12, 0x0f, 0x16, // 0x89a0,
    0x80, 0x6b, 0x85, 0x3d, 0x45, 0x85, 0x3e, 0x46, 0xe5, 0x47, 0xc3, 0x13, 0xff, 0xe5, 0x45, 0xc3, // 0x89b0,
    0x9f, 0x50, 0x02, 0x8f, 0x45, 0xe5, 0x48, 0xc3, 0x13, 0xff, 0xe5, 0x46, 0xc3, 0x9f, 0x50, 0x02, // 0x89c0,
    0x8f, 0x46, 0xe5, 0x47, 0xc3, 0x13, 0xff, 0xfd, 0xe5, 0x45, 0x2d, 0xfd, 0xe4, 0x33, 0xfc, 0xe5, // 0x89d0,
    0x44, 0x12, 0x0f, 0x90, 0x40, 0x05, 0xe5, 0x44, 0x9f, 0xf5, 0x45, 0xe5, 0x48, 0xc3, 0x13, 0xff, // 0x89e0,
    0xfd, 0xe5, 0x46, 0x2d, 0xfd, 0xe4, 0x33, 0xfc, 0xe5, 0x43, 0x12, 0x0f, 0x90, 0x40, 0x05, 0xe5, // 0x89f0,
    0x43, 0x9f, 0xf5, 0x46, 0x12, 0x06, 0xd7, 0x80, 0x14, 0x85, 0x40, 0x48, 0x85, 0x3f, 0x47, 0x85, // 0x8a00,
    0x3e, 0x46, 0x85, 0x3d, 0x45, 0x80, 0x06, 0x02, 0x06, 0xd7, 0x12, 0x0d, 0x7e, 0x90, 0x30, 0x24, // 0x8a10,
    0xe5, 0x3d, 0xf0, 0xa3, 0xe5, 0x3e, 0xf0, 0xa3, 0xe5, 0x3f, 0xf0, 0xa3, 0xe5, 0x40, 0xf0, 0xa3, // 0x8a20,
    0xe5, 0x3c, 0xf0, 0x90, 0x30, 0x23, 0xe4, 0xf0, 0x22, 0xc0, 0xe0, 0xc0, 0x83, 0xc0, 0x82, 0xc0, // 0x8a30,
    0xd0, 0x90, 0x3f, 0x0c, 0xe0, 0xf5, 0x32, 0xe5, 0x32, 0x30, 0xe3, 0x74, 0x30, 0x36, 0x66, 0x90, // 0x8a40,
    0x60, 0x19, 0xe0, 0xf5, 0x0a, 0xa3, 0xe0, 0xf5, 0x0b, 0x90, 0x60, 0x1d, 0xe0, 0xf5, 0x14, 0xa3, // 0x8a50,
    0xe0, 0xf5, 0x15, 0x90, 0x60, 0x21, 0xe0, 0xf5, 0x0c, 0xa3, 0xe0, 0xf5, 0x0d, 0x90, 0x60, 0x29, // 0x8a60,
    0xe0, 0xf5, 0x0e, 0xa3, 0xe0, 0xf5, 0x0f, 0x90, 0x60, 0x31, 0xe0, 0xf5, 0x10, 0xa3, 0xe0, 0xf5, // 0x8a70,
    0x11, 0x90, 0x60, 0x39, 0xe0, 0xf5, 0x12, 0xa3, 0xe0, 0xf5, 0x13, 0x30, 0x01, 0x06, 0x30, 0x33, // 0x8a80,
    0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x09, 0x30, 0x02, 0x06, 0x30, 0x33, 0x03, 0xd3, 0x80, 0x01, // 0x8a90,
    0xc3, 0x92, 0x0a, 0x30, 0x33, 0x0c, 0x30, 0x03, 0x09, 0x20, 0x02, 0x06, 0x20, 0x01, 0x03, 0xd3, // 0x8aa0,
    0x80, 0x01, 0xc3, 0x92, 0x0b, 0x90, 0x30, 0x01, 0xe0, 0x44, 0x40, 0xf0, 0xe0, 0x54, 0xbf, 0xf0, // 0x8ab0,
    0xe5, 0x32, 0x30, 0xe1, 0x14, 0x30, 0x34, 0x11, 0x90, 0x30, 0x22, 0xe0, 0xf5, 0x08, 0xe4, 0xf0, // 0x8ac0,
    0x30, 0x00, 0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x08, 0xe5, 0x32, 0x30, 0xe5, 0x12, 0x90, 0x56, // 0x8ad0,
    0xa1, 0xe0, 0xf5, 0x09, 0x30, 0x31, 0x09, 0x30, 0x05, 0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x0d, // 0x8ae0,
    0x90, 0x3f, 0x0c, 0xe5, 0x32, 0xf0, 0xd0, 0xd0, 0xd0, 0x82, 0xd0, 0x83, 0xd0, 0xe0, 0x32, 0x90, // 0x8af0,
    0x0e, 0x7e, 0xe4, 0x93, 0xfe, 0x74, 0x01, 0x93, 0xff, 0xc3, 0x90, 0x0e, 0x7c, 0x74, 0x01, 0x93, // 0x8b00,
    0x9f, 0xff, 0xe4, 0x93, 0x9e, 0xfe, 0xe4, 0x8f, 0x3b, 0x8e, 0x3a, 0xf5, 0x39, 0xf5, 0x38, 0xab, // 0x8b10,
    0x3b, 0xaa, 0x3a, 0xa9, 0x39, 0xa8, 0x38, 0xaf, 0x4b, 0xfc, 0xfd, 0xfe, 0x12, 0x05, 0x28, 0x12, // 0x8b20,
    0x0d, 0xe1, 0xe4, 0x7b, 0xff, 0xfa, 0xf9, 0xf8, 0x12, 0x05, 0xb3, 0x12, 0x0d, 0xe1, 0x90, 0x0e, // 0x8b30,
    0x69, 0xe4, 0x12, 0x0d, 0xf6, 0x12, 0x0d, 0xe1, 0xe4, 0x85, 0x4a, 0x37, 0xf5, 0x36, 0xf5, 0x35, // 0x8b40,
    0xf5, 0x34, 0xaf, 0x37, 0xae, 0x36, 0xad, 0x35, 0xac, 0x34, 0xa3, 0x12, 0x0d, 0xf6, 0x8f, 0x37, // 0x8b50,
    0x8e, 0x36, 0x8d, 0x35, 0x8c, 0x34, 0xe5, 0x3b, 0x45, 0x37, 0xf5, 0x3b, 0xe5, 0x3a, 0x45, 0x36, // 0x8b60,
    0xf5, 0x3a, 0xe5, 0x39, 0x45, 0x35, 0xf5, 0x39, 0xe5, 0x38, 0x45, 0x34, 0xf5, 0x38, 0xe4, 0xf5, // 0x8b70,
    0x22, 0xf5, 0x23, 0x85, 0x3b, 0x31, 0x85, 0x3a, 0x30, 0x85, 0x39, 0x2f, 0x85, 0x38, 0x2e, 0x02, // 0x8b80,
    0x0f, 0x46, 0xe0, 0xa3, 0xe0, 0x75, 0xf0, 0x02, 0xa4, 0xff, 0xae, 0xf0, 0xc3, 0x08, 0xe6, 0x9f, // 0x8b90,
    0xf6, 0x18, 0xe6, 0x9e, 0xf6, 0x22, 0xff, 0xe5, 0xf0, 0x34, 0x60, 0x8f, 0x82, 0xf5, 0x83, 0xec, // 0x8ba0,
    0xf0, 0x22, 0x78, 0x52, 0x7e, 0x00, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x02, 0x04, 0xc1, 0xe4, 0xfc, // 0x8bb0,
    0xfd, 0x12, 0x06, 0x99, 0x78, 0x5c, 0xe6, 0xc3, 0x13, 0xfe, 0x08, 0xe6, 0x13, 0x22, 0x78, 0x52, // 0x8bc0,
    0xe6, 0xfe, 0x08, 0xe6, 0xff, 0xe4, 0xfc, 0xfd, 0x22, 0xe7, 0xc4, 0xf8, 0x54, 0xf0, 0xc8, 0x68, // 0x8bd0,
    0xf7, 0x09, 0xe7, 0xc4, 0x54, 0x0f, 0x48, 0xf7, 0x22, 0xe6, 0xfc, 0xed, 0x75, 0xf0, 0x04, 0xa4, // 0x8be0,
    0x22, 0x12, 0x06, 0x7c, 0x8f, 0x48, 0x8e, 0x47, 0x8d, 0x46, 0x8c, 0x45, 0x22, 0xe0, 0xfe, 0xa3, // 0x8bf0,
    0xe0, 0xfd, 0xee, 0xf6, 0xed, 0x08, 0xf6, 0x22, 0x13, 0xff, 0xc3, 0xe6, 0x9f, 0xff, 0x18, 0xe6, // 0x8c00,
    0x9e, 0xfe, 0x22, 0xe6, 0xc3, 0x13, 0xf7, 0x08, 0xe6, 0x13, 0x09, 0xf7, 0x22, 0xad, 0x39, 0xac, // 0x8c10,
    0x38, 0xfa, 0xf9, 0xf8, 0x12, 0x05, 0x28, 0x8f, 0x3b, 0x8e, 0x3a, 0x8d, 0x39, 0x8c, 0x38, 0xab, // 0x8c20,
    0x37, 0xaa, 0x36, 0xa9, 0x35, 0xa8, 0x34, 0x22, 0x93, 0xff, 0xe4, 0xfc, 0xfd, 0xfe, 0x12, 0x05, // 0x8c30,
    0x28, 0x8f, 0x37, 0x8e, 0x36, 0x8d, 0x35, 0x8c, 0x34, 0x22, 0x78, 0x84, 0xe6, 0xfe, 0x08, 0xe6, // 0x8c40,
    0xff, 0xe4, 0x8f, 0x37, 0x8e, 0x36, 0xf5, 0x35, 0xf5, 0x34, 0x22, 0x90, 0x0e, 0x8c, 0xe4, 0x93, // 0x8c50,
    0x25, 0xe0, 0x24, 0x0a, 0xf8, 0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x22, 0xe6, 0xfe, 0x08, 0xe6, 0xff, // 0x8c60,
    0xe4, 0x8f, 0x3b, 0x8e, 0x3a, 0xf5, 0x39, 0xf5, 0x38, 0x22, 0x78, 0x4e, 0xe6, 0xfe, 0x08, 0xe6, // 0x8c70,
    0xff, 0x22, 0xef, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x22, 0x78, 0x89, // 0x8c80,
    0xef, 0x26, 0xf6, 0x18, 0xe4, 0x36, 0xf6, 0x22, 0x75, 0x89, 0x03, 0x75, 0xa8, 0x01, 0x75, 0xb8, // 0x8c90,
    0x04, 0x75, 0x34, 0xff, 0x75, 0x35, 0x0e, 0x75, 0x36, 0x15, 0x75, 0x37, 0x0d, 0x12, 0x0e, 0x9a, // 0x8ca0,
    0x12, 0x00, 0x09, 0x12, 0x0f, 0x16, 0x12, 0x00, 0x06, 0xd2, 0x00, 0xd2, 0x34, 0xd2, 0xaf, 0x75, // 0x8cb0,
    0x34, 0xff, 0x75, 0x35, 0x0e, 0x75, 0x36, 0x49, 0x75, 0x37, 0x03, 0x12, 0x0e, 0x9a, 0x30, 0x08, // 0x8cc0,
    0x09, 0xc2, 0x34, 0x12, 0x08, 0xcb, 0xc2, 0x08, 0xd2, 0x34, 0x30, 0x0b, 0x09, 0xc2, 0x36, 0x12, // 0x8cd0,
    0x02, 0x6c, 0xc2, 0x0b, 0xd2, 0x36, 0x30, 0x09, 0x09, 0xc2, 0x36, 0x12, 0x00, 0x0e, 0xc2, 0x09, // 0x8ce0,
    0xd2, 0x36, 0x30, 0x0e, 0x03, 0x12, 0x06, 0xd7, 0x30, 0x35, 0xd3, 0x90, 0x30, 0x29, 0xe5, 0x1e, // 0x8cf0,
    0xf0, 0xb4, 0x10, 0x05, 0x90, 0x30, 0x23, 0xe4, 0xf0, 0xc2, 0x35, 0x80, 0xc1, 0xe4, 0xf5, 0x4b, // 0x8d00,
    0x90, 0x0e, 0x7a, 0x93, 0xff, 0xe4, 0x8f, 0x37, 0xf5, 0x36, 0xf5, 0x35, 0xf5, 0x34, 0xaf, 0x37, // 0x8d10,
    0xae, 0x36, 0xad, 0x35, 0xac, 0x34, 0x90, 0x0e, 0x6a, 0x12, 0x0d, 0xf6, 0x8f, 0x37, 0x8e, 0x36, // 0x8d20,
    0x8d, 0x35, 0x8c, 0x34, 0x90, 0x0e, 0x72, 0x12, 0x06, 0x7c, 0xef, 0x45, 0x37, 0xf5, 0x37, 0xee, // 0x8d30,
    0x45, 0x36, 0xf5, 0x36, 0xed, 0x45, 0x35, 0xf5, 0x35, 0xec, 0x45, 0x34, 0xf5, 0x34, 0xe4, 0xf5, // 0x8d40,
    0x22, 0xf5, 0x23, 0x85, 0x37, 0x31, 0x85, 0x36, 0x30, 0x85, 0x35, 0x2f, 0x85, 0x34, 0x2e, 0x12, // 0x8d50,
    0x0f, 0x46, 0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x90, 0x0e, 0x72, 0x12, 0x0d, 0xea, 0x12, 0x0f, 0x46, // 0x8d60,
    0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x90, 0x0e, 0x6e, 0x12, 0x0d, 0xea, 0x02, 0x0f, 0x46, 0xe5, 0x40, // 0x8d70,
    0x24, 0xf2, 0xf5, 0x37, 0xe5, 0x3f, 0x34, 0x43, 0xf5, 0x36, 0xe5, 0x3e, 0x34, 0xa2, 0xf5, 0x35, // 0x8d80,
    0xe5, 0x3d, 0x34, 0x28, 0xf5, 0x34, 0xe5, 0x37, 0xff, 0xe4, 0xfe, 0xfd, 0xfc, 0x78, 0x18, 0x12, // 0x8d90,
    0x06, 0x69, 0x8f, 0x40, 0x8e, 0x3f, 0x8d, 0x3e, 0x8c, 0x3d, 0xe5, 0x37, 0x54, 0xa0, 0xff, 0xe5, // 0x8da0,
    0x36, 0xfe, 0xe4, 0xfd, 0xfc, 0x78, 0x07, 0x12, 0x06, 0x56, 0x78, 0x10, 0x12, 0x0f, 0x9a, 0xe4, // 0x8db0,
    0xff, 0xfe, 0xe5, 0x35, 0xfd, 0xe4, 0xfc, 0x78, 0x0e, 0x12, 0x06, 0x56, 0x12, 0x0f, 0x9d, 0xe4, // 0x8dc0,
    0xff, 0xfe, 0xfd, 0xe5, 0x34, 0xfc, 0x78, 0x18, 0x12, 0x06, 0x56, 0x78, 0x08, 0x12, 0x0f, 0x9a, // 0x8dd0,
    0x22, 0x8f, 0x3b, 0x8e, 0x3a, 0x8d, 0x39, 0x8c, 0x38, 0x22, 0x12, 0x06, 0x7c, 0x8f, 0x31, 0x8e, // 0x8de0,
    0x30, 0x8d, 0x2f, 0x8c, 0x2e, 0x22, 0x93, 0xf9, 0xf8, 0x02, 0x06, 0x69, 0x00, 0x00, 0x00, 0x00, // 0x8df0,
    0x12, 0x01, 0x17, 0x08, 0x31, 0x15, 0x53, 0x54, 0x44, 0x20, 0x20, 0x20, 0x20, 0x20, 0x13, 0x01, // 0x8e00,
    0x10, 0x01, 0x56, 0x40, 0x1a, 0x30, 0x29, 0x7e, 0x00, 0x30, 0x04, 0x20, 0xdf, 0x30, 0x05, 0x40, // 0x8e10,
    0xbf, 0x50, 0x03, 0x00, 0xfd, 0x50, 0x27, 0x01, 0xfe, 0x60, 0x00, 0x11, 0x00, 0x3f, 0x05, 0x30, // 0x8e20,
    0x00, 0x3f, 0x06, 0x22, 0x00, 0x3f, 0x01, 0x2a, 0x00, 0x3f, 0x02, 0x00, 0x00, 0x36, 0x06, 0x07, // 0x8e30,
    0x00, 0x3f, 0x0b, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x40, 0xbf, 0x30, 0x01, 0x00, // 0x8e40,
    0xbf, 0x30, 0x29, 0x70, 0x00, 0x3a, 0x00, 0x00, 0xff, 0x3a, 0x00, 0x00, 0xff, 0x36, 0x03, 0x36, // 0x8e50,
    0x02, 0x41, 0x44, 0x58, 0x20, 0x18, 0x10, 0x0a, 0x04, 0x04, 0x00, 0x03, 0xff, 0x64, 0x00, 0x00, // 0x8e60,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x06, 0x00, 0x03, 0x51, 0x00, 0x7a, // 0x8e70,
    0x50, 0x3c, 0x28, 0x1e, 0x10, 0x10, 0x50, 0x2d, 0x28, 0x16, 0x10, 0x10, 0x02, 0x00, 0x10, 0x0c, // 0x8e80,
    0x10, 0x04, 0x0c, 0x6e, 0x06, 0x05, 0x00, 0xa5, 0x5a, 0x00, 0xae, 0x35, 0xaf, 0x36, 0xe4, 0xfd, // 0x8e90,
    0xed, 0xc3, 0x95, 0x37, 0x50, 0x33, 0x12, 0x0f, 0xe2, 0xe4, 0x93, 0xf5, 0x38, 0x74, 0x01, 0x93, // 0x8ea0,
    0xf5, 0x39, 0x45, 0x38, 0x60, 0x23, 0x85, 0x39, 0x82, 0x85, 0x38, 0x83, 0xe0, 0xfc, 0x12, 0x0f, // 0x8eb0,
    0xe2, 0x74, 0x03, 0x93, 0x52, 0x04, 0x12, 0x0f, 0xe2, 0x74, 0x02, 0x93, 0x42, 0x04, 0x85, 0x39, // 0x8ec0,
    0x82, 0x85, 0x38, 0x83, 0xec, 0xf0, 0x0d, 0x80, 0xc7, 0x22, 0x78, 0xbe, 0xe6, 0xd3, 0x08, 0xff, // 0x8ed0,
    0xe6, 0x64, 0x80, 0xf8, 0xef, 0x64, 0x80, 0x98, 0x22, 0x93, 0xff, 0x7e, 0x00, 0xe6, 0xfc, 0x08, // 0x8ee0,
    0xe6, 0xfd, 0x12, 0x04, 0xc1, 0x78, 0xc1, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0xd3, 0xef, 0x9d, 0xee, // 0x8ef0,
    0x9c, 0x22, 0x78, 0xbd, 0xd3, 0xe6, 0x64, 0x80, 0x94, 0x80, 0x22, 0x25, 0xe0, 0x24, 0x0a, 0xf8, // 0x8f00,
    0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x22, 0xe5, 0x3c, 0xd3, 0x94, 0x00, 0x40, 0x0b, 0x90, 0x0e, 0x88, // 0x8f10,
    0x12, 0x0b, 0xf1, 0x90, 0x0e, 0x86, 0x80, 0x09, 0x90, 0x0e, 0x82, 0x12, 0x0b, 0xf1, 0x90, 0x0e, // 0x8f20,
    0x80, 0xe4, 0x93, 0xf5, 0x44, 0xa3, 0xe4, 0x93, 0xf5, 0x43, 0xd2, 0x06, 0x30, 0x06, 0x03, 0xd3, // 0x8f30,
    0x80, 0x01, 0xc3, 0x92, 0x0e, 0x22, 0xa2, 0xaf, 0x92, 0x32, 0xc2, 0xaf, 0xe5, 0x23, 0x45, 0x22, // 0x8f40,
    0x90, 0x0e, 0x5d, 0x60, 0x0e, 0x12, 0x0f, 0xcb, 0xe0, 0xf5, 0x2c, 0x12, 0x0f, 0xc8, 0xe0, 0xf5, // 0x8f50,
    0x2d, 0x80, 0x0c, 0x12, 0x0f, 0xcb, 0xe5, 0x30, 0xf0, 0x12, 0x0f, 0xc8, 0xe5, 0x31, 0xf0, 0xa2, // 0x8f60,
    0x32, 0x92, 0xaf, 0x22, 0xd2, 0x01, 0xc2, 0x02, 0xe4, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, // 0x8f70,
    0x33, 0xd2, 0x36, 0xd2, 0x01, 0xc2, 0x02, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, 0x33, 0x22, // 0x8f80,
    0xfb, 0xd3, 0xed, 0x9b, 0x74, 0x80, 0xf8, 0x6c, 0x98, 0x22, 0x12, 0x06, 0x69, 0xe5, 0x40, 0x2f, // 0x8f90,
    0xf5, 0x40, 0xe5, 0x3f, 0x3e, 0xf5, 0x3f, 0xe5, 0x3e, 0x3d, 0xf5, 0x3e, 0xe5, 0x3d, 0x3c, 0xf5, // 0x8fa0,
    0x3d, 0x22, 0xc0, 0xe0, 0xc0, 0x83, 0xc0, 0x82, 0x90, 0x3f, 0x0d, 0xe0, 0xf5, 0x33, 0xe5, 0x33, // 0x8fb0,
    0xf0, 0xd0, 0x82, 0xd0, 0x83, 0xd0, 0xe0, 0x32, 0x90, 0x0e, 0x5f, 0xe4, 0x93, 0xfe, 0x74, 0x01, // 0x8fc0,
    0x93, 0xf5, 0x82, 0x8e, 0x83, 0x22, 0x78, 0x7f, 0xe4, 0xf6, 0xd8, 0xfd, 0x75, 0x81, 0xcd, 0x02, // 0x8fd0,
    0x0c, 0x98, 0x8f, 0x82, 0x8e, 0x83, 0x75, 0xf0, 0x04, 0xed, 0x02, 0x06, 0xa5,                   // 0x8fe0
};

int32_t OV5640_OutSize_Set(OV5640_Object_t *pObj, uint16_t offx, uint16_t offy, uint16_t width, uint16_t height) {
    uint32_t index;
    uint8_t  tmp;
    int32_t  ret        = OV5640_OK;

    uint16_t datas[][2] = {
        {0X3212,          0X03},
        {0x3808,    width >> 8},
        {0x3809,  width & 0xff},
        {0x380a,   height >> 8},
        {0x380b, height & 0xff},
        {0x3810,     offx >> 8},
        {0x3811,   offx & 0xff},
        {0x3812,     offy >> 8},
        {0x3813,   offy & 0xff},
        {0X3212,          0X13},
        {0X3212,          0Xa3}
    };

    for (index = 0; index < (sizeof(datas) / 4U); index++) {
        tmp = (uint8_t)datas[index][1];

        if (ov5640_write_reg(&pObj->Ctx, datas[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

int32_t OV5640_ImageWin_Set(OV5640_Object_t *pObj, uint16_t offx, uint16_t offy, uint16_t width, uint16_t height) {
    uint16_t xst, yst, xend, yend;
    xst  = offx;
    yst  = offy;
    xend = offx + width - 1;
    yend = offy + height - 1;

    uint32_t index;
    uint8_t  tmp;
    int32_t  ret = OV5640_OK;

    //
    uint16_t datas[][2] = {
        {0X3212,        0X03},
        {0X3800,    xst >> 8},
        {0X3801,  xst & 0XFF},
        {0X3802,    yst >> 8},
        {0X3803,  yst & 0XFF},
        {0X3804,   xend >> 8},
        {0X3805, xend & 0XFF},
        {0X3806,   yend >> 8},
        {0X3807, yend & 0XFF},
        {0X3212,        0X13},
        {0X3212,        0Xa3}
    };

    for (index = 0; index < (sizeof(datas) / 4U); index++) {
        tmp = (uint8_t)datas[index][1];

        if (ov5640_write_reg(&pObj->Ctx, datas[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

int32_t OV5640_JPEG_Mode(OV5640_Object_t *pObj) {
    uint32_t index;
    uint8_t  tmp;
    int32_t  ret = OV5640_OK;

    for (index = 0; index < (sizeof(OV5640_jpeg_reg_tbl) / 4U); index++) {
        tmp = (uint8_t)OV5640_jpeg_reg_tbl[index][1];

        if (ov5640_write_reg(&pObj->Ctx, OV5640_jpeg_reg_tbl[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

int32_t OV5640_RGB565_Mode(OV5640_Object_t *pObj) {
    uint32_t index;
    uint8_t  tmp;
    int32_t  ret = OV5640_OK;

    for (index = 0; index < (sizeof(ov5640_rgb565_reg_tbl) / 4U); index++) {
        tmp = (uint8_t)ov5640_rgb565_reg_tbl[index][1];

        if (ov5640_write_reg(&pObj->Ctx, ov5640_rgb565_reg_tbl[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    return ret;
}

int32_t OV5640_Init_General_Mode(OV5640_Object_t *pObj, uint32_t Resolution, uint32_t PixelFormat) {
    uint32_t index;
    uint8_t  tmp;
    int32_t  ret = OV5640_OK;

    if (pObj->IsInitialized == 0U) {
        /* Check if resolution is supported */
        if ((Resolution > OV5640_R2592x1944) ||
            ((PixelFormat != OV5640_RGB565) && (PixelFormat != OV5640_JPEG))) {
            ret = OV5640_ERROR;
        }
        else {
            /* Set common parameters for all resolutions */
            for (index = 0; index < (sizeof(ov5640_uxga_init_reg_tbl) / 4U); index++) {
                if (ret != OV5640_ERROR) {
                    tmp = (uint8_t)ov5640_uxga_init_reg_tbl[index][1];

                    if (ov5640_write_reg(&pObj->Ctx, ov5640_uxga_init_reg_tbl[index][0], &tmp, 1) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                }
            }

            if (ret == OV5640_OK) {
                if (PixelFormat == OV5640_RGB565) {
                    if (OV5640_RGB565_Mode(pObj) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else if (OV5640_Set_Solution_More(pObj, Resolution) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else if (OV5640_SetPolarities(pObj, OV5640_POLARITY_PCLK_HIGH, OV5640_POLARITY_HREF_HIGH, OV5640_POLARITY_VSYNC_HIGH) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    OV5640_Focus_Init(pObj);
                }
                else if (PixelFormat == OV5640_JPEG) {
                    if (OV5640_JPEG_Mode(pObj) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else if (OV5640_Set_Solution_More(pObj, Resolution) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    else if (OV5640_SetPolarities(pObj, OV5640_POLARITY_PCLK_HIGH, OV5640_POLARITY_HREF_HIGH, OV5640_POLARITY_VSYNC_HIGH) != OV5640_OK) {
                        ret = OV5640_ERROR;
                    }
                    OV5640_Focus_Init(pObj);
                }
            }
        }
    }

    return ret;
}

int32_t OV5640_Set_Solution_More(OV5640_Object_t *pObj, uint32_t solution) {
    return OV5640_OutSize_Set(pObj, 4, 0, solution_table[solution][0], solution_table[solution][1]);
}

int32_t OV5640_Focus_Init(OV5640_Object_t *pObj) {
    size_t   i;
    uint16_t addr       = 0x8000;
    int32_t  ret        = OV5640_OK;

    uint16_t datas[][2] = {
        {0x3000, 0x20}
    };

    uint16_t datas2[][2] = {
        {0x3022, 0x00},
        {0x3023, 0x00},
        {0x3024, 0x00},
        {0x3025, 0x00},
        {0x3026, 0x00},
        {0x3027, 0x00},
        {0x3028, 0x00},
        {0x3029, 0x7f},
        {0x3000, 0x00}
    };

    uint8_t tmp;

    tmp = datas[0][1];
    ov5640_write_reg(&pObj->Ctx, datas[0][0], &tmp, 1);
    for (i = 0; i < sizeof(OV5640_AF_Config); i++) {
        tmp = OV5640_AF_Config[i];
        ov5640_write_reg(&pObj->Ctx, addr, &tmp, 1);
        addr++;
    }

    for (size_t index = 0; index < (sizeof(datas2) / 4U); index++) {
        tmp = (uint8_t)datas[index][1];

        if (ov5640_write_reg(&pObj->Ctx, datas[index][0], &tmp, 1) != OV5640_OK) {
            ret = OV5640_ERROR;
        }
    }

    i   = 0;
    tmp = 0x8F;
    do {
        ov5640_read_reg(&pObj->Ctx, 0x3029, &tmp, 1);
        vTaskDelay(pdMS_TO_TICKS(5));
        i++;
        if (i > 1000)
            return 1;
    }
    while (tmp != 0x70);
    return ret;
}

int32_t OV5640_Focus_Single(OV5640_Object_t *pObj) {
    uint8_t  temp;
    uint16_t retry = 0;

    temp           = 0x03;
    ov5640_write_reg(&pObj->Ctx, 0x3022, &temp, 1);
    while (1) {
        retry++;
        ov5640_read_reg(&pObj->Ctx, 0x3029, &temp, 1);
        if (temp == 0x10)
            break;
        vTaskDelay(pdMS_TO_TICKS(5));
        if (retry > 200)
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

int32_t OV5640_Focus_Constant(OV5640_Object_t *pObj) {
    uint8_t  temp  = 0;
    uint16_t retry = 0;

    temp           = 0x01;
    ov5640_write_reg(&pObj->Ctx, 0x3023, &temp, 1);
    temp = 0x08;
    ov5640_write_reg(&pObj->Ctx, 0x3022, &temp, 1);
    do {
        ov5640_read_reg(&pObj->Ctx, 0x3023, &temp, 1);
        retry++;
        if (retry > 200)
            return OV5640_ERROR;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    while (temp != 0x00);
    temp = 0x01;
    ov5640_write_reg(&pObj->Ctx, 0x3023, &temp, 1);
    temp = 0x04;
    ov5640_write_reg(&pObj->Ctx, 0x3022, &temp, 1);
    retry = 0;
    do {
        ov5640_read_reg(&pObj->Ctx, 0x3023, &temp, 1);
        retry++;
        if (retry > 200)
            return OV5640_ERROR;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    while (temp != 0x00);
    return OV5640_OK;
}

int32_t OV5640_Focus_Send_Single(OV5640_Object_t *pObj) {
    uint8_t temp;
    temp = 0x03;
    ov5640_write_reg(&pObj->Ctx, 0x3022, &temp, 1);
    return OV5640_OK;
}

uint8_t OV5640_Focus_Read_Single(OV5640_Object_t *pObj) {
    uint8_t temp;
    ov5640_read_reg(&pObj->Ctx, 0x3029, &temp, 1);
    if (temp == 0x10)
        return 1;
    else
        return 0;
}

int32_t OV5640_Focus_Send_Constant_IDLE(OV5640_Object_t *pObj) {
    uint8_t temp = 0;
    //
    temp = 0x01;
    ov5640_write_reg(&pObj->Ctx, 0x3023, &temp, 1);
    temp = 0x08;
    ov5640_write_reg(&pObj->Ctx, 0x3022, &temp, 1);

    return OV5640_OK;
}

uint8_t OV5640_Focus_Read_Constant(OV5640_Object_t *pObj) {
    uint8_t temp;
    ov5640_read_reg(&pObj->Ctx, 0x3023, &temp, 1);
    if (temp == 0x00)
        return 1;
    else
        return 0;
}

int32_t OV5640_Focus_Send_Constant_Focus(OV5640_Object_t *pObj) {
    uint8_t temp = 0;
    //
    temp = 0x01;
    ov5640_write_reg(&pObj->Ctx, 0x3023, &temp, 1);
    temp = 0x04;
    ov5640_write_reg(&pObj->Ctx, 0x3022, &temp, 1);

    return OV5640_OK;
}

const static uint8_t OV5640_SATURATION_TBL[7][6] = {
    {0X0C, 0x30, 0X3D, 0X3E, 0X3D, 0X01}, //-3
    {0X10, 0x3D, 0X4D, 0X4E, 0X4D, 0X01}, //-2
    {0X15, 0x52, 0X66, 0X68, 0X66, 0X02}, //-1
    {0X1A, 0x66, 0X80, 0X82, 0X80, 0X02}, //+0
    {0X1F, 0x7A, 0X9A, 0X9C, 0X9A, 0X02}, //+1
    {0X24, 0x8F, 0XB3, 0XB6, 0XB3, 0X03}, //+2
    {0X2B, 0xAB, 0XD6, 0XDA, 0XD6, 0X04}  //+3
};

void OV5640_Color_Saturation(OV5640_Object_t *pObj, uint8_t sat) {
    uint8_t i;
    uint8_t temp = 0;

    temp         = 0x03;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
    temp = 0x1c;
    ov5640_write_reg(&pObj->Ctx, 0x5381, &temp, 1);
    temp = 0x5a;
    ov5640_write_reg(&pObj->Ctx, 0x5382, &temp, 1);
    temp = 0x06;
    ov5640_write_reg(&pObj->Ctx, 0x5383, &temp, 1);

    for (i = 0; i < 6; i++) {
        temp = OV5640_SATURATION_TBL[sat][i];
        ov5640_write_reg(&pObj->Ctx, 0x5384, &temp, 1);
    }

    temp = 0x98;
    ov5640_write_reg(&pObj->Ctx, 0x538b, &temp, 1);
    temp = 0x01;
    ov5640_write_reg(&pObj->Ctx, 0x538a, &temp, 1);
    temp = 0x13;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
    temp = 0xa3;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
}

void OV5640_Contrast(OV5640_Object_t *pObj, uint8_t contrast) {
    uint8_t reg0val = 0X00;
    uint8_t reg1val = 0X20;
    uint8_t temp    = 0;
    switch (contrast) {
    case 0: //-3
        reg1val = reg0val = 0X14;
        break;
    case 1: //-2
        reg1val = reg0val = 0X18;
        break;
    case 2: //-1
        reg1val = reg0val = 0X1C;
        break;
    case 4: // 1
        reg0val = 0X10;
        reg1val = 0X24;
        break;
    case 5: // 2
        reg0val = 0X18;
        reg1val = 0X28;
        break;
    case 6: // 3
        reg0val = 0X1C;
        reg1val = 0X2C;
        break;
    }

    temp = 0x03;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
    temp = reg0val;
    ov5640_write_reg(&pObj->Ctx, 0x5585, &temp, 1);
    temp = reg1val;
    ov5640_write_reg(&pObj->Ctx, 0x5586, &temp, 1);
    temp = 0x13;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
    temp = 0xa3;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
}

void OV5640_Sharpness(OV5640_Object_t *pObj, uint8_t sharp) {
    uint8_t temp = 0;
#define wr_reg(x, y) \
    temp = (y);      \
    ov5640_write_reg(&pObj->Ctx, (x), &temp, 1)

    if (sharp < 33) {
        wr_reg(0x5308, 0x65);
        wr_reg(0x5302, sharp);
    }
    else {
        wr_reg(0x5308, 0x25);
        wr_reg(0x5300, 0x08);
        wr_reg(0x5301, 0x30);
        wr_reg(0x5302, 0x10);
        wr_reg(0x5303, 0x00);
        wr_reg(0x5309, 0x08);
        wr_reg(0x530a, 0x30);
        wr_reg(0x530b, 0x04);
        wr_reg(0x530c, 0x06);
    }
}

void OV5640_StartGroup(OV5640_Object_t *pObj) {
    uint8_t temp    = 0;
    temp = 0x03;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
}

void OV5640_UseGroup(OV5640_Object_t *pObj) {
    uint8_t temp    = 0;
    temp = 0x13;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
    temp = 0xa3;
    ov5640_write_reg(&pObj->Ctx, 0x3212, &temp, 1);
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

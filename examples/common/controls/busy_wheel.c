/*
 * Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*============================ INCLUDES ======================================*/
#include "./busy_wheel.h"
#include "./__common.h"
#include "arm_2d.h"
#include <math.h>
#include <time.h>
#include <assert.h>

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunknown-warning-option"
#   pragma clang diagnostic ignored "-Wreserved-identifier"
#   pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wpadded"
#   pragma clang diagnostic ignored "-Wcast-qual"
#   pragma clang diagnostic ignored "-Wcast-align"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wunused-const-variable"
#   pragma clang diagnostic ignored "-Wmissing-declarations"
#   pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

/*============================ MACROS ========================================*/
#define __PI        (3.1415926f)
#define __RADIUS    (30.0f)

#ifndef BUSY_WHEEL_SPIN_SPEED
#   define BUSY_WHEEL_SPIN_SPEED            60
#endif

#if __GLCD_CFG_COLOUR_DEPTH__ == 8

#   define c_tileWhiteDot           c_tileWhiteDotAlpha

#elif __GLCD_CFG_COLOUR_DEPTH__ == 16

#   define c_tileWhiteDot           c_tileWhiteDotRGB565

#elif __GLCD_CFG_COLOUR_DEPTH__ == 32

#   define c_tileWhiteDot           c_tileWhiteDotCCCA8888
#else
#   error Unsupported colour depth!
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
extern uint32_t SystemCoreClock;
extern
const arm_2d_tile_t c_tileWhiteDot;
extern
const arm_2d_tile_t c_tileWhiteDotMask;
/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/

ARM_NOINIT static arm_2d_location_t s_tDotsLocation[8];
ARM_NOINIT static uint8_t s_tAlphaTable[8];
ARM_NOINIT static int64_t s_lLastTime, s_lLastTime2;
ARM_NOINIT static uint32_t s_wUnit;
/*============================ IMPLEMENTATION ================================*/


void busy_wheel_init(void)
{
    int16_t iXOffset = c_tileWhiteDot.tRegion.tSize.iWidth >> 2;
    int16_t iYOffset = c_tileWhiteDot.tRegion.tSize.iHeight>> 2;
    
    for (uint_fast8_t n = 0; n < 8; n++) {
        s_tDotsLocation[n].iY = (int16_t)(sinf(__PI * (float)n / 4.0f) * __RADIUS) + iXOffset;
        s_tDotsLocation[n].iX = (int16_t)(cosf(__PI * (float)n / 4.0f) * __RADIUS) + iYOffset;
        s_tAlphaTable[n] = (uint8_t)((float)n * 255.0f / 8.0f);
    }
    s_lLastTime = clock();
    s_lLastTime2 = s_lLastTime;
    s_wUnit = (SystemCoreClock  / 1000) * BUSY_WHEEL_SPIN_SPEED;
}

void busy_wheel_show(const arm_2d_tile_t *ptTarget, bool bIsNewFrame)
{
    assert(NULL != ptTarget);
    static uint8_t s_chOffset = 0;
    arm_2d_location_t tBasePoint = {
        .iX = ptTarget->tRegion.tSize.iWidth / 2 - 10,
        .iY = ptTarget->tRegion.tSize.iHeight / 2 - 10,
    };
    arm_2d_region_t tTargetRegion = c_tileWhiteDot.tRegion;
    
    if (bIsNewFrame) {
        int64_t lClocks = clock();
        int32_t nElapsed = (int32_t)((lClocks - s_lLastTime));
        if (nElapsed >= (int32_t)s_wUnit) {
            s_lLastTime = lClocks;
            s_chOffset++;
            s_chOffset &= 0x07;
        }
    }
    
    for (uint_fast8_t n = 0; n < 8; n++) {
        uint_fast8_t chIndex = n;
        chIndex += s_chOffset;
        chIndex &= 0x07;
        
        tTargetRegion.tLocation = tBasePoint;
        tTargetRegion.tLocation.iX += s_tDotsLocation[chIndex].iX;
        tTargetRegion.tLocation.iY += s_tDotsLocation[chIndex].iY;
    
        arm_2d_alpha_blending_with_colour_keying(   &c_tileWhiteDot,
                                                    ptTarget,
                                                    &tTargetRegion,
                                                    s_tAlphaTable[n],
                                                    (__arm_2d_color_t){0});
                                                
        arm_2d_op_wait_async(NULL);
    }
}


void busy_wheel2_show(const arm_2d_tile_t *ptTarget, bool bIsNewFrame)
{
    assert(NULL != ptTarget);
    static uint8_t s_chOffset = 0;
    arm_2d_location_t tBasePoint = {
        .iX = ptTarget->tRegion.tSize.iWidth / 2 - 10,
        .iY = ptTarget->tRegion.tSize.iHeight / 2 - 10,
    };
    arm_2d_region_t tTargetRegion = c_tileWhiteDot.tRegion;
    
    if (bIsNewFrame) {
        int64_t lClocks = clock();
        int32_t nElapsed = (int32_t)((lClocks - s_lLastTime2));
        if (nElapsed >= (int32_t)s_wUnit) {
            s_lLastTime2 = lClocks;
            s_chOffset++;
            s_chOffset &= 0x07;
        }
    }
    
    for (uint_fast8_t n = 0; n < 8; n++) {
        uint_fast8_t chIndex = n;
        chIndex += s_chOffset;
        chIndex &= 0x07;
        
        tTargetRegion.tLocation = tBasePoint;
        tTargetRegion.tLocation.iX += s_tDotsLocation[chIndex].iX;
        tTargetRegion.tLocation.iY += s_tDotsLocation[chIndex].iY;
    
        arm_2d_fill_colour_with_mask_and_opacity(   
                                    ptTarget,
                                    &tTargetRegion,
                                    &c_tileWhiteDotMask,
                                    (__arm_2d_color_t){GLCD_COLOR_WHITE},
                                    s_tAlphaTable[n]);
                                                
        arm_2d_op_wait_async(NULL);
    }
}


#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

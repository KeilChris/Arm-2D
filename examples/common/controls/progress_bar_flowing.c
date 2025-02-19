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
#include "./progress_bar_flowing.h"
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

#ifndef PROGRESS_BAR_WAVE_SPEED
#   define PROGRESS_BAR_WAVE_SPEED     15
#endif


/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
extern uint32_t SystemCoreClock;

/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
extern const arm_2d_tile_t c_tileWaveMask;

ARM_NOINIT static int32_t s_nLastTime;
ARM_NOINIT static int32_t s_tWavingTime;
ARM_NOINIT static uint32_t s_wUnit;
static volatile bool s_bWaving = false;

/*============================ IMPLEMENTATION ================================*/


void progress_bar_flowing_init(void)
{
    s_nLastTime = clock();
    s_wUnit = (SystemCoreClock  / 1000) * PROGRESS_BAR_WAVE_SPEED;
    s_tWavingTime = s_nLastTime + SystemCoreClock  * 3;
}

void progress_bar_flowing_show( const arm_2d_tile_t *ptTarget, 
                                int_fast16_t iProgress, 
                                bool bIsNewFrame)
{
    int_fast16_t iWidth = ptTarget->tRegion.tSize.iWidth * 3 >> 3;         //!< 3/8 Width
 
    assert(NULL != ptTarget);
    if (iProgress > 1000) {
        iProgress = 0;
    }
 
    arm_2d_region_t tBarRegion = {
        .tLocation = {
           .iX = (ptTarget->tRegion.tSize.iWidth - (int16_t)iWidth) / 2,
           .iY = (ptTarget->tRegion.tSize.iHeight - c_tileWaveMask.tRegion.tSize.iHeight) / (int16_t)2,
        },
        .tSize = {
            .iWidth = (int16_t)iWidth,
            .iHeight = c_tileWaveMask.tRegion.tSize.iHeight,
        },
    };
    
    //! draw a white box
    arm_2d_fill_colour(ptTarget, &tBarRegion, __RGB(0xa5, 0xc6, 0xef)/*0xA63D*/);
    
    //! pave inter texture
    tBarRegion.tSize.iHeight-=2;
    tBarRegion.tSize.iWidth-=2;
    tBarRegion.tLocation.iX += 1;
    tBarRegion.tLocation.iY += 1;
    
    arm_2d_fill_colour(ptTarget, &tBarRegion, GLCD_COLOR_WHITE);

    if (iProgress > 0) {
        //! calculate the width of the inner stripe 
        tBarRegion.tSize.iWidth = tBarRegion.tSize.iWidth * (int16_t)iProgress / 1000;
    } 
    
    //! draw the inner stripe
    arm_2d_fill_colour(ptTarget, &tBarRegion, __RGB(0x94, 0xd2, 0x52) /*0x968A */);
    
    //! draw wave
    do {
        static int16_t s_iOffset = 0;
        arm_2d_region_t tInnerRegion = {
            .tSize = c_tileWaveMask.tRegion.tSize,
            .tLocation = {
                .iX = - c_tileWaveMask.tRegion.tSize.iWidth + s_iOffset,
            },
        };
        arm_2d_tile_t tileInnerSlot;
    
        //! generate a child tile for texture paving
        arm_2d_tile_generate_child(ptTarget, &tBarRegion, &tileInnerSlot, false);

        arm_2d_fill_colour_with_mask(   &tileInnerSlot,
                                        &tInnerRegion,
                                        &c_tileWaveMask,
                                        (__arm_2d_color_t) {GLCD_COLOR_WHITE});
        
        //! update offset
        if (bIsNewFrame) {
            int32_t nClocks = clock();
            int32_t nElapsed = (int32_t)((nClocks - s_nLastTime));

            if (nClocks >= s_tWavingTime) {
                s_tWavingTime = nClocks + SystemCoreClock  * 3;
                s_bWaving = true;
            }

            if (nElapsed >= (int32_t)s_wUnit) {
                s_nLastTime = nClocks;
                
                if (s_bWaving) {
                    s_iOffset+=2;
                    if (s_iOffset >= tBarRegion.tSize.iWidth + c_tileWaveMask.tRegion.tSize.iWidth) {
                        s_iOffset = 0;
                        s_bWaving = false;
                    }
                }
            }
        }

    } while(0);
    
}

#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

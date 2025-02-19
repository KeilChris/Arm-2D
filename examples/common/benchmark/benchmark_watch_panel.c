/*
 * Copyright (c) 2009-2022 Arm Limited. All rights reserved.
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
#include "benchmark_watch_panel.h"
#include "arm_extra_controls.h"
#include <math.h>

#if defined(__clang__)
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
#   pragma clang diagnostic ignored "-Wmissing-prototypes"
#   pragma clang diagnostic ignored "-Wgnu-statement-expression"
#elif defined(__IS_COMPILER_GCC__)
#   pragma GCC diagnostic ignored "-Wmissing-braces"
#   pragma GCC diagnostic ignored "-Wunused-value"
#endif

/*============================ MACROS ========================================*/
#ifndef __GLCD_CFG_SCEEN_WIDTH__
#   warning Please specify the screen width by defining the macro __GLCD_CFG_SCEEN_WIDTH__, default value 320 is used for now
#   define __GLCD_CFG_SCEEN_WIDTH__      320
#endif

#ifndef __GLCD_CFG_SCEEN_HEIGHT__
#   warning Please specify the screen height by defining the macro __GLCD_CFG_SCEEN_HEIGHT__, default value 240 is used for now
#   define __GLCD_CFG_SCEEN_HEIGHT__      320
#endif


#if __GLCD_CFG_COLOUR_DEPTH__ == 8

#   define c_tileGear01             c_tileGear01Mask
#   define c_tileGear02             c_tileGear02Mask
#   define c_tilePointerSec         c_tilePointerSecMask
#   define c_tileBackground         c_tileBackgroundCHNB
#   define c_tileWatchPanel         c_tileWatchPanelCHNB
#   define c_tileStar               c_tileStarMask

#elif __GLCD_CFG_COLOUR_DEPTH__ == 16

#   define c_tileGear01             c_tileGear01RGB565
#   define c_tileGear02             c_tileGear02RGB565
#   define c_tilePointerSec         c_tilePointerSecRGB565
#   define c_tileBackground         c_tileBackgroundRGB565
#   define c_tileWatchPanel         c_tileWatchPanelRGB565
#   define c_tileStar               c_tileStarRGB565

#elif __GLCD_CFG_COLOUR_DEPTH__ == 32

#   define c_tileGear01             c_tileGear01CCCA8888
#   define c_tileGear02             c_tileGear02CCCA8888
#   define c_tilePointerSec         c_tilePointerSecCCCA8888
#   define c_tileBackground         c_tileBackgroundCCCA8888
#   define c_tileWatchPanel         c_tileWatchPanelCCCA8888
#   define c_tileStar               c_tileStarCCCA8888

#else
#   error Unsupported colour depth!
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/

#define arm_2d_layer(__TILE_ADDR, __TRANS, __X, __Y, ...)                       \
    {                                                                           \
        .ptTile = (__TILE_ADDR),                                                \
        .tRegion.tLocation.iX = (__X),                                          \
        .tRegion.tLocation.iY = (__Y),                                          \
        .chTransparency = (__TRANS),                                            \
        __VA_ARGS__                                                             \
    }

/*============================ TYPES =========================================*/

typedef struct arm_2d_layer_t {
    const arm_2d_tile_t *ptTile;
    arm_2d_region_t tRegion;
    uint32_t    wMode;
    uint8_t     chTransparency;
    uint8_t     bIsIrregular    : 1;
    uint8_t                     : 7;
    uint16_t    hwMaskingColour;
} arm_2d_layer_t;

typedef struct floating_range_t {
    arm_2d_region_t tRegion;
    arm_2d_layer_t *ptLayer;
    arm_2d_location_t tOffset;
} floating_range_t;



typedef struct {
    arm_2d_op_trans_opa_t tOP;
    const arm_2d_tile_t *ptTile;
    float fAngle;
    float fAngleSpeed;
    arm_2d_location_t tCentre;
    arm_2d_location_t *ptTargetCentre;
    arm_2d_region_t *ptRegion;
    uint8_t chOpacity;
} demo_gears_t;

/*============================ GLOBAL VARIABLES ==============================*/

extern
const arm_2d_tile_t c_tileGear01;
extern
const arm_2d_tile_t c_tileGear02;
extern
const arm_2d_tile_t c_tilePointerSec;

extern 
const arm_2d_tile_t c_tileWatchPanel;
 
extern 
const arm_2d_tile_t c_tileBackground;

extern const arm_2d_tile_t c_tileStar;
extern const arm_2d_tile_t c_tileStarMask;
extern const arm_2d_tile_t c_tileStarMask2;

extern const arm_2d_tile_t c_tileCircleBackgroundMask;

/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/

static arm_2d_layer_t s_ptRefreshLayers[] = {
    arm_2d_layer(&c_tileBackground, 0, 0, 0),
};

static floating_range_t s_ptFloatingBoxes[] = {
    {
        .tRegion = {{   0-(559 - 240), 
                        0-(260 - 240)
                    }, 
                    {   240 + ((559 - 240) * 2), 
                        240 + ((260 - 240) * 2)
                    }
                   },
        .ptLayer = &s_ptRefreshLayers[0],
        .tOffset = {-1, -1},
    },
};

static
demo_gears_t s_tGears[] = {

    {
        .ptTile = &c_tileGear02,
        .fAngleSpeed = 3.0f,
        .tCentre = {
            .iX = 20,
            .iY = 20,
        },

        .ptRegion = (arm_2d_region_t []){ {
            .tLocation = {
                .iX = ((__GLCD_CFG_SCEEN_WIDTH__ - 41) >> 1) + 30,
                .iY = ((__GLCD_CFG_SCEEN_HEIGHT__ - 41) >>1) + 30,
            },
            .tSize = {
                .iWidth = 41,
                .iHeight = 41,
            },
            }
        },
    #if 0  /*! a demo shows how to specifiy the centre of rotation on the target tile */
        .ptTargetCentre = (arm_2d_location_t []){
            {
                .iX = ((__GLCD_CFG_SCEEN_WIDTH__ - 41) >> 1) + 30,
                .iY = ((__GLCD_CFG_SCEEN_HEIGHT__ - 41) >>1) + 30,
            },
        },
    #endif
        .chOpacity = 255,
    },

    {
        .ptTile = &c_tileGear01,
        .fAngleSpeed = -0.5f,
        .tCentre = {
            .iX = 61,
            .iY = 60,
        },
        .ptRegion = (arm_2d_region_t []){ {
            .tLocation = {
                .iX = ((__GLCD_CFG_SCEEN_WIDTH__ - 120) >> 1),
                .iY = ((__GLCD_CFG_SCEEN_HEIGHT__ - 120) >>1),
            },
            .tSize = {
                .iWidth = 120,
                .iHeight = 120,
            },
        }},
        .chOpacity = 128,
    },

    {
        .ptTile = &c_tilePointerSec,
        .fAngleSpeed = 1.0f,
        .tCentre = {
            .iX = 7,
            .iY = 110,
        },
        .ptRegion = (arm_2d_region_t []){ {
            .tLocation = {
                .iX = ((__GLCD_CFG_SCEEN_WIDTH__ - 222) >> 1),
                .iY = ((__GLCD_CFG_SCEEN_HEIGHT__ - 222) >>1),
            },
            .tSize = {
                .iWidth = 222,
                .iHeight = 222,
            },
        }},
        .chOpacity = 255,
    },
};


/*============================ IMPLEMENTATION ================================*/

static volatile uint32_t s_wSystemTimeInMs = 0;
static volatile bool s_bTimeout = false;
extern void platform_1ms_event_handler(void);

void platform_1ms_event_handler(void)
{
    s_wSystemTimeInMs++;
    if (!(s_wSystemTimeInMs & (_BV(10) - 1))) {
        s_bTimeout = true;
    }
}

void example_gui_init(void)
{
    arm_extra_controls_init();

    arm_foreach(arm_2d_layer_t, s_ptRefreshLayers) {
        arm_2d_region_t tRegion = _->tRegion;
        if (!tRegion.tSize.iHeight) {
            tRegion.tSize.iHeight = _->ptTile->tRegion.tSize.iHeight;
        }
        if (!tRegion.tSize.iWidth) {
            tRegion.tSize.iWidth = _->ptTile->tRegion.tSize.iWidth;
        }
        
        _->tRegion = tRegion;
    }
}

static void example_update_boxes(floating_range_t *ptBoxes, uint_fast16_t hwCount)
{
    assert(NULL != ptBoxes);
    assert(hwCount > 0);

    do {
        arm_2d_region_t tOldRegion = ptBoxes->ptLayer->tRegion;
        if (   (tOldRegion.tLocation.iX + tOldRegion.tSize.iWidth + ptBoxes->tOffset.iX)
            >= ptBoxes->tRegion.tLocation.iX + ptBoxes->tRegion.tSize.iWidth) {
            ptBoxes->tOffset.iX = -ptBoxes->tOffset.iX;
        }

        if (    (tOldRegion.tLocation.iX + ptBoxes->tOffset.iX)
            <   (ptBoxes->tRegion.tLocation.iX)) {
            ptBoxes->tOffset.iX = -ptBoxes->tOffset.iX;
        }

        if (   (tOldRegion.tLocation.iY + tOldRegion.tSize.iHeight + ptBoxes->tOffset.iY)
            >= ptBoxes->tRegion.tLocation.iY + ptBoxes->tRegion.tSize.iHeight) {
            ptBoxes->tOffset.iY = -ptBoxes->tOffset.iY;
        }

        if (    (tOldRegion.tLocation.iY + ptBoxes->tOffset.iY)
            <   (ptBoxes->tRegion.tLocation.iY)) {
            ptBoxes->tOffset.iY = -ptBoxes->tOffset.iY;
        }

        ptBoxes->ptLayer->tRegion.tLocation.iX += ptBoxes->tOffset.iX;
        ptBoxes->ptLayer->tRegion.tLocation.iY += ptBoxes->tOffset.iY;

        ptBoxes++;

    }while(--hwCount);
}


void example_gui_do_events(void)
{
    example_update_boxes(s_ptFloatingBoxes, dimof(s_ptFloatingBoxes));
}


__WEAK
void example_gui_on_refresh_evt_handler(const arm_2d_tile_t *ptFrameBuffer)
{
     ARM_2D_UNUSED(ptFrameBuffer);
}


void example_gui_refresh(const arm_2d_tile_t *ptTile, bool bIsNewFrame)
{

#if 1


    arm_2d_fill_colour(ptTile, NULL, GLCD_COLOR_BLACK);

    static arm_2d_tile_t s_tPanelTile;
    
    arm_2d_tile_generate_child( ptTile,
                                (const arm_2d_region_t []) {
                                    {
                                        .tSize = {240, 240},
                                        .tLocation = {
                                            .iX = ((__GLCD_CFG_SCEEN_WIDTH__ - 240) >> 1),
                                            .iY = ((__GLCD_CFG_SCEEN_HEIGHT__ - 240) >> 1),
                                        },
                                    },
                                },
                                &s_tPanelTile,
                                false);
    
    arm_2d_tile_copy_with_des_mask(
        s_ptRefreshLayers->ptTile,
        &s_tPanelTile,
        &c_tileCircleBackgroundMask,
        &(s_ptRefreshLayers->tRegion),
        ARM_2D_CP_MODE_COPY
    );
    
    //! draw the watch panel with transparency effect
    do {
        static const arm_2d_region_t tPanelRegion = {
            .tLocation = {
                .iX = ((__GLCD_CFG_SCEEN_WIDTH__ - 221) >> 1),
                .iY = ((__GLCD_CFG_SCEEN_HEIGHT__ - 221) >> 1),
            },
            .tSize = {
                .iWidth = 221,
                .iHeight = 221,
            },
        };
        arm_2d_alpha_blending_with_colour_keying(
                                    &c_tileWatchPanel,
                                    ptTile,
                                    &tPanelRegion,
                                    64,    //!< 50% opacity
                                    (__arm_2d_color_t){GLCD_COLOR_BLACK});


    } while(0);

    /*! for each item (ptItem) inside array s_tGears */
    arm_foreach (demo_gears_t, s_tGears, ptItem) {

        if (bIsNewFrame) {
            ptItem->fAngle += ARM_2D_ANGLE(ptItem->fAngleSpeed);

            ptItem->fAngle = fmodf(ptItem->fAngle,ARM_2D_ANGLE(360));

        }

        if (255 == ptItem->chOpacity) {
        
            arm_2dp_tile_rotation(   (arm_2d_op_trans_t *)&(_->tOP),
                                            ptItem->ptTile,     //!< source tile
                                            ptTile,             //!< target tile
                                            ptItem->ptRegion,   //!< target region
                                            ptItem->tCentre,    //!< center point
                                            ptItem->fAngle,     //!< rotation angle
                                            GLCD_COLOR_BLACK,   //!< masking colour
                                            ptItem->ptTargetCentre);
        } else {
            arm_2dp_tile_rotation_with_alpha(
                                            &(ptItem->tOP),
                                            ptItem->ptTile,     //!< source tile
                                            ptTile,             //!< target tile
                                            ptItem->ptRegion,   //!< target region
                                            ptItem->tCentre,    //!< center point
                                            ptItem->fAngle,     //!< rotation angle
                                            GLCD_COLOR_BLACK,   //!< masking colour
                                            ptItem->chOpacity,  //!< Opacity
                                            ptItem->ptTargetCentre);
        }
    }
#else
    (void)s_tGears;
    arm_2d_fill_colour(ptTile, NULL, GLCD_COLOR_WHITE);
    
    const arm_2d_region_t c_tRightScreen = {
        .tLocation = {
            .iX = GLCD_WIDTH >> 1,
        },
        .tSize = {
            .iWidth = GLCD_WIDTH >> 1,
            .iHeight = GLCD_HEIGHT,
        },
    };
    
    static arm_2d_tile_t s_tTempTile;
    arm_2d_tile_generate_child(ptTile, &c_tRightScreen, &s_tTempTile, false);
    
    spinning_wheel_show(&s_tTempTile, bIsNewFrame);
#endif

    //! demo for transform
    do {
        static volatile float s_fAngle = 0.0f;
        static float s_fScale = 0.5f;
        static uint8_t s_chOpacity = 255;
    
        if (bIsNewFrame) {
            
            s_fAngle +=ARM_2D_ANGLE(10.0f);
            s_fAngle = fmodf(s_fAngle,ARM_2D_ANGLE(360));
            s_fScale += 0.05f;
            
            
            if (s_chOpacity >= 8) {
                s_chOpacity -= 8;
            } else {
                s_fScale = 0.5f;
                s_chOpacity = 255;
            }
        }
        
        arm_2d_location_t tCentre = {
            .iX = c_tileStar.tRegion.tSize.iWidth >> 1,
            .iY = c_tileStar.tRegion.tSize.iHeight >> 1,
        };
    #if 0
        static arm_2d_op_trans_opa_t s_tStarOP;
        arm_2dp_tile_transform_with_opacity(
            &s_tStarOP,         //!< control block
            &c_tileStar,        //!< source tile
            ptTile,             //!< target tile
            NULL,               //!< target region
            tCentre,            //!< pivot on source
            s_fAngle,           //!< rotation angle 
            s_fScale,           //!< zoom scale 
            GLCD_COLOR_BLACK,   //!< chromekeying colour
            s_chOpacity         //!< opacity
        );               
     #else
        static arm_2d_op_trans_msk_opa_t s_tStarOP;
        const arm_2d_tile_t *ptSrcMask = 
        #if __ARM_2D_CFG_SUPPORT_COLOUR_CHANNEL_ACCESS__
            &c_tileStarMask2; //!< 8in32 channel mask
        #else
            &c_tileStarMask;    //!< normal 8bit mask
        #endif
        
        arm_2dp_tile_transform_with_src_mask_and_opacity(
            &s_tStarOP,         //!< control block
            &c_tileStar,        //!< source tile
            ptSrcMask,          //!< source mask
            ptTile,             //!< target tile
            NULL,               //!< target region
            tCentre,            //!< pivot on source
            s_fAngle,           //!< rotation angle 
            s_fScale           //!< zoom scale 
            ,s_chOpacity         //!< opacity
        );               
     #endif
    } while(0); 

    example_gui_on_refresh_evt_handler(ptTile);

}


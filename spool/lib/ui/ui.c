#include "ui.h"
#include "lvgl.h"

#define MY_DISP_HOR_RES 128
#define MY_DISP_VER_RES 64

static void my_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                        lv_color_t *color_p);

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv = {
    .hor_res = MY_DISP_HOR_RES,
    .ver_res = MY_DISP_VER_RES,
    .physical_hor_res = -1,
    .physical_ver_res = -1,

    .offset_x = 0,
    .offset_y = 0,
    .draw_buf = &disp_buf,

    .direct_mode = 0,
    .full_refresh = 0,
    .sw_rotate = 0,
    .antialiasing = 0,
    .rotated = 0,
    .screen_transp = 0,
    .dpi = 50,

/* TODO */
    .flush_cb = my_flush_cb,
};

static lv_color_t buf_1[MY_DISP_HOR_RES * 10];
/* optional buffer */
static lv_color_t buf_2[MY_DISP_HOR_RES * 10];

static void put_px(int32_t x, int32_t y, lv_color_t colop)
{
}

static void my_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                        lv_color_t *color_p)
{
    /* The most simple case (but also the slowest) to put all pixels to the screen one-by-one */
    int32_t x, y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            put_px(x, y, *color_p);
            color_p++;
        }
    }

    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

void uiInit(void)
{
    /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, MY_DISP_HOR_RES * 10);
    lv_disp_drv_init(&disp_drv);
}
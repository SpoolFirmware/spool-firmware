#include "ui.h"
#include "lvgl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "misc.h"
#include "manual/mcu.h"
#include "hal/stm32/hal.h"
#include "dbgprintf.h"
#include "platform/platform.h"
#include "drf.h"
#include "compiler.h"

#define MY_DISP_HOR_RES 128
#define MY_DISP_VER_RES 64

/**
 * @brief default implementation, ui should not start if display does not exist
 */
__attribute__((weak)) void uiWriteTile(uint8_t x0, uint8_t x1, uint8_t y,
                                       const uint8_t *buf)
{
    panic();
}

void vApplicationTickHook(void)
{
    STATIC_ASSERT(1000 / configTICK_RATE_HZ >= 1);
    lv_tick_inc(1000 / configTICK_RATE_HZ);
}

static void my_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                        lv_color_t *color_p);

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv = {
};

static lv_disp_t *disp;

static uint8_t lcd_fb[MY_DISP_HOR_RES * MY_DISP_VER_RES / 8];

static lv_color_t buf_1[MY_DISP_HOR_RES * 10];
/* optional buffer */
static lv_color_t buf_2[MY_DISP_HOR_RES * 10];
STATIC_ASSERT(ARRAY_SIZE(buf_1) == ARRAY_SIZE(buf_2));

static void display_sync(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    for (int32_t y = y1; y <= y2; ++y) {
        uiWriteTile(x1, x2, y, &lcd_fb[y * MY_DISP_HOR_RES / 8]);
    }
}

static void my_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                        lv_color_t *color_p)
{
    /*Return if the area is out the screen*/
    if (area->x2 < 0)
        return;
    if (area->y2 < 0)
        return;
    if (area->x1 > MY_DISP_HOR_RES - 1)
        return;
    if (area->y1 > MY_DISP_VER_RES - 1)
        return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > MY_DISP_HOR_RES - 1 ? MY_DISP_HOR_RES - 1 : area->x2;
    int32_t act_y2 = area->y2 > MY_DISP_VER_RES - 1 ? MY_DISP_VER_RES - 1 : area->y2;

    int32_t x, y;

    /*Refresh frame buffer*/
    for (y = act_y1; y <= act_y2; y++) {
        for (x = act_x1; x <= act_x2; x++) {
            if (lv_color_to1(*color_p) != 0) {
                lcd_fb[x / 8 + y * MY_DISP_HOR_RES / 8] &= ~(1 << (7 - (x % 8)));
            } else {
                lcd_fb[x / 8 + y * MY_DISP_HOR_RES / 8] |= (1 << (7 - (x % 8)));
            }
            color_p++;
        }

        color_p += area->x2 - act_x2; /*Next row*/
    }

    display_sync(act_x1, act_y1, act_x2, act_y2);
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

void lv_example_line_1(void)
{
    /*Create an array for the points of the line*/
    static lv_point_t line_points[] = {
        { 5, 5 }, { 70, 70 }, { 120, 10 }, { 180, 60 }, { 240, 10 }
    };

    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));

    /*Create a line and apply the new style*/
    lv_obj_t *line1;
    line1 = lv_line_create(lv_scr_act());
    lv_line_set_points(line1, line_points, 5); /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
    lv_obj_center(line1);
}

static TaskHandle_t uiTaskHandle;
static SemaphoreHandle_t uiSem;

portTASK_FUNCTION(uiTask, pvParameters)
{
    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, ARRAY_SIZE(buf_1));

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.physical_hor_res = -1;
    disp_drv.physical_ver_res = -1;
    disp_drv.offset_x = 0;
    disp_drv.offset_y = 0;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = my_flush_cb;

    disp = lv_disp_drv_register(&disp_drv);

    lv_obj_t *label1 = lv_label_create(lv_scr_act());
    if (!label1) {
        panic();
    }
    lv_obj_set_width(label1, 100);
    lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(label1, "It is a circularly scrolling text. ");
    lv_obj_set_style_anim_speed(label1, 10, LV_PART_MAIN);
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);

    lv_example_line_1();

    for (;;) {
        xSemaphoreTake(uiSem, portMAX_DELAY);
        lv_timer_handler();
        xSemaphoreGive(uiSem);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
void uiInit(void)
{
    if ((uiSem = xSemaphoreCreateMutex()) == NULL) {
        panic();
    }
    if (xTaskCreate(uiTask, "ui", 512, NULL,
                    tskIDLE_PRIORITY, &uiTaskHandle) != pdTRUE) {
        panic();
    }
}
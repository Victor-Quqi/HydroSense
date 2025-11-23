/**
 * @file lv_conf.h
 * Configuration file for LVGL
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*================
 * DYNAMIC MEMORY
 *================*/

/* Memory size which will be used by the library
 * to store the graphical objects and other data */
#define LV_MEM_SIZE (32U * 1024U)

/*================
 * FONT USAGE
 *================*/

/* Enable built-in fonts */
#define LV_USE_FONT_DEJAVU_16_PERSIAN_HEBREW 1
#define LV_USE_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14  // Default English font

/*================
 * THEME USAGE
 *================*/

/* Always enable the default theme */
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_LIGHT 1
#define LV_THEME_DEFAULT_FONT &lv_font_dejavu_16_persian_hebrew

/*================
 * OTHER FEATURES
 *================*/

/* Enable animations */
#define LV_USE_ANIMATION 1

/* Enable shadows */
#define LV_USE_SHADOW 1

/* Enable screenshot */
#define LV_USE_SYSMON 1

/*================
 * COMPILER SETTINGS
 *================*/

/* For big endian systems set to 1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/* Define the color depth */
#define LV_COLOR_DEPTH 1

/*================
 * HAL SETTINGS
 *================*/

/* Default display refresh period */
#define LV_DISP_DEF_REFR_PERIOD 30

/*================
 * LOG SETTINGS
 *================*/

/* Enable logging */
#define LV_USE_LOG 1
#if LV_USE_LOG
/*Can be printf or custom function*/
#define LV_LOG_PRINTF(...) printf(__VA_ARGS__)
#endif

#endif /*LV_CONF_H*/
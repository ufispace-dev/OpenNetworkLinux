#ifndef __LEDI_MAIN_H__
#define __LEDI_MAIN_H__

#define S3IP_LED_COLOR_DARK                       0
#define S3IP_LED_COLOR_GREEN                      1
#define S3IP_LED_COLOR_YELLOW                     2
#define S3IP_LED_COLOR_RED                        3
#define S3IP_LED_COLOR_BLUE                       4
#define S3IP_LED_COLOR_GREEN_BLINK                5
#define S3IP_LED_COLOR_YELLOW_BLINK               6
#define S3IP_LED_COLOR_RED_BLINK                  7
#define S3IP_LED_COLOR_BLUE_BLINK                 8
#define S3IP_LED_COLOR_CYAN                       100
#define S3IP_LED_COLOR_MAGENTA                    101
#define S3IP_LED_COLOR_WHITE                      102
#define S3IP_LED_COLOR_CYAN_BLINK                 103
#define S3IP_LED_COLOR_MAGENTA_BLINK              104
#define S3IP_LED_COLOR_WHITE_BLINK                105

#define STR_OFF                     "dark"
#define STR_ON                      "on"
#define STR_RED                     "red"
#define STR_RED_BLINK               "red_blink"
#define STR_ORANGE                  "orange"
#define STR_ORANGE_BLINK            "orange_blink"
#define STR_YELLOW                  "yellow"
#define STR_YELLOW_BLINK            "yellow_blink"
#define STR_GREEN                   "green"
#define STR_GREEN_BLINK             "green_blink"
#define STR_BLUE                    "blue"
#define STR_BLUE_BLINK              "blue_blink"
#define STR_PURPLE                  "purple"
#define STR_PURPLE_BLINK            "purple_blink"

typedef struct {
    int id;
    int parent;
    int valid;
} led_elems;

int _parse_led_on(char *str, int *mode);
int _parse_led_capability(char *str, uint32_t *caps);
int _update_led_status(int color, uint32_t *status);
int _s3ip_to_onl_color(int s3ip_color, int *onl_color);
int _onl_to_s3ip_color(int onl_color, int *s3ip_color);
int _onlp_led_total_get(int *total);
#endif  /* __LEDI_MAIN_H__ */
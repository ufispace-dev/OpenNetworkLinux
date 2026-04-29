#ifndef __PSUI_MAIN_H__
#define __PSUI_MAIN_H__

#define PSU_TEMP_MAX   16

typedef struct {
    int id;
    int fan_id;
    int thermal_id[PSU_TEMP_MAX];
    int fan_num;
    int temp_num;
    int parent;
    int valid;
} psu_elems;

int _onlp_psu_fan_pos_off_get(int *off);
int _onlp_psu_thermal_pos_off_get(int *off);
int _onlp_psu_total_get(int *total);
#endif  /* __PSUI_MAIN_H__ */
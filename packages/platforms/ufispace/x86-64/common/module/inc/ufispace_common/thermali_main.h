#ifndef __THERMALI_MAIN_H__
#define __THERMALI_MAIN_H__

#define IS_THERMAL(_type)  (_type == TYPE_THERMAL)
#define IS_PSU(_type)      (_type == TYPE_PSU)

typedef enum thermal_type_e {
    TYPE_THERMAL = 0,
    TYPE_PSU,
} thermal_type_t;

typedef struct {
    int id;
    int type;
    int parent;
    int child;
    int valid;
} temp_elems;

int _onlp_temp_total_get(int *total);
#endif  /* __THERMALI_MAIN_H__ */
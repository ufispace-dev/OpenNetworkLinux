#ifndef __FANI_MAIN_H__
#define __FANI_MAIN_H__

#define IS_FANTRAY(_type)  (_type == TYPE_FANTRAY)
#define IS_PSU(_type)      (_type == TYPE_PSU)

typedef enum fan_type_e {
    TYPE_FANTRAY = 0,
    TYPE_PSU,
} fan_type_t;

typedef struct {
    int id;
    int type;
    int parent;
    int child;
    int valid;
} fan_elems;

int _onlp_fan_total_get(int *total);
#endif  /* __FANI_MAIN_H__ */
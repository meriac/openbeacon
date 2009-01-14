#ifndef SLIDER_H_
#define SLIDER_H_

#include <beacontypes.h>

#define SLIDER_BASE_VAL 0x8080
#define SLIDER_DECIMATION 0x20
#define SLIDER_MAX_SENSORS 13

struct slider_definition {
	unsigned int num_parts, part_diff, val_max;
	unsigned int base_val, decimation;
	unsigned int sensor_mask;
	unsigned char sensors[SLIDER_MAX_SENSORS];
};

#define SLIDER_DEFINE(num_parts, part_diff, sensor_mask, sensors...) {\
	num_parts, part_diff, (part_diff*(num_parts-1)), \
	SLIDER_BASE_VAL, SLIDER_DECIMATION, \
	sensor_mask, \
	{sensors} \
}

extern const struct slider_definition slider_x;
extern const struct slider_definition slider_y;

#define SLIDER_X_DEFINITION SLIDER_DEFINE(3, 128, 0x188, 7,3,8);
#define SLIDER_Y_DEFINITION SLIDER_DEFINE(7, 128, 0x07F, 0,1,2,3,4,5,6);

struct slider_state {
	int xval, yval;
	struct {
		int button1:1;
		int button2:1;
		int xtouching:1;
		int ytouching:1;
	} buttons ;
};

typedef void slider_update_callback_t(struct slider_state *new_state);

extern int slider_register_slider_update_callback(slider_update_callback_t callback);
extern void slider_update(const u_int16_t * const interrupt_status, struct slider_state *state);
extern void slider_print(const struct slider_state * const state);

#endif /*SLIDER_H_*/

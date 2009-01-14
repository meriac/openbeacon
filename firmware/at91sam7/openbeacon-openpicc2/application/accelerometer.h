#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

enum AXIS {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

enum ORIENTATION {
	ORIENTATION_Y_UP,
	ORIENTATION_X_UP,
	ORIENTATION_Y_DOWN,
	ORIENTATION_X_DOWN,
};

typedef void orientation_changed_callback_t(enum ORIENTATION new_orientation);

extern void accelerometer_init(void);
extern int accelerometer_get_raw(enum AXIS axis);
extern float accelerometer_get_g(enum AXIS axis);
extern enum ORIENTATION accelerometer_get_orientation(void);
extern int accelerometer_register_orientation_changed_callback(orientation_changed_callback_t callback);

#endif /*ACCELEROMETER_H_*/

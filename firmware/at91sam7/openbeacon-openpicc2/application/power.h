#ifndef POWER_H_
#define POWER_H_

enum power_bus_current {
	POWER_BUS_SUSPEND, /* Low current, 500uA */
	POWER_BUS_1,       /* Standard current, 100mA */
	POWER_BUS_5,       /* Full USB current, 500mA */
	POWER_BUS_10,      /* Wall-powered, 1A */
};

typedef void power_pressed_callback_t(void);

extern void power_on(void);
extern void power_off(void);
extern void power_set_bus_current(enum power_bus_current current);
extern int power_get_battery_voltage(void);
extern unsigned char power_is_pressed(void);
extern unsigned char power_is_usb_connected(void);
extern unsigned char power_init(void);
extern int power_set_pressed_callback(power_pressed_callback_t *callback);

#endif /*POWER_H_*/

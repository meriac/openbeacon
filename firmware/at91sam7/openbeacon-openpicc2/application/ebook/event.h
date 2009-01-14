#ifndef EVENTS_H_
#define EVENTS_H_

#define EVENT_MAX_WAITING 10

enum event_class {
	EVENT_NONE,
	EVENT_ROTATED,
	EVENT_CLICKED,
	EVENT_SCROLL_RELATIVE_Y,
	EVENT_POWEROFF,
};

enum click_type {
	CLICK_BUTTON_1,
	CLICK_BUTTON_2,
	CLICK_SLIDER_Y,
	SWIPE_SLIDER_X_1,
	SWIPE_SLIDER_X_2,
	CLICK_MAGIC,
};

typedef struct {
	enum event_class class;
	int param;
} event_t;

extern int event_receive(event_t *next_event, portBASE_TYPE wait_time);
extern int event_send(enum event_class event_class, int param);
extern int event_init(void);

#endif /*EVENTS_H_*/

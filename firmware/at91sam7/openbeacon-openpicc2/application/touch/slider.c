#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "touch/ad7147.h"
#include "touch/slider.h"

const struct slider_definition slider_x = SLIDER_X_DEFINITION;
const struct slider_definition slider_y = SLIDER_Y_DEFINITION;

/* Weighted average. Each sensor is assigned a logical position starting with zero
 * and incrementing by slider->part_diff. The (somewhat) normalized value of each sensor
 * reading is calculated as  value = max(0, sensor_reading - base_val)) / decimation
 * Then the slider position is calculated as  sum( (pos * value) / sum( value )
 */
static int slider_calc_pos(const u_int16_t * const raw_values, const struct slider_definition *slider)
{
	u_int32_t values[slider->num_parts], result=0, sum = 0;
	unsigned int i;
	for(i=0; i<slider->num_parts; i++) {
		if(raw_values[slider->sensors[i]] < (slider->base_val)) values[i] = 0;
		else values[i] = (raw_values[slider->sensors[i]] - (slider->base_val)) / (slider->decimation);
		sum += values[i];
		values[i] *= (i+1)*(slider->part_diff);
		result += values[i];
		//printf("%3lu ", values[i]);
	}
	//printf("\n");
	if(sum == 0) return 0;
	result = result/sum - (slider->part_diff);
	if(result > slider->val_max) return slider->val_max;
	else return result;
}

static slider_update_callback_t *slider_update_callback = NULL;
void slider_update(const u_int16_t * const interrupt_status, struct slider_state *state)
{
	state->xval = slider_calc_pos(interrupt_status+3, &slider_x);
	state->yval = slider_calc_pos(interrupt_status+3, &slider_y);
	
	state->buttons.xtouching = !!(interrupt_status[1] & slider_x.sensor_mask);
	state->buttons.ytouching = !!(interrupt_status[1] & slider_y.sensor_mask);
	
	state->buttons.button1 = !!(interrupt_status[1] & (1L<<9));
	state->buttons.button2 = !!(interrupt_status[1] & (1L<<10));
	
	if(slider_update_callback != NULL)
		slider_update_callback(state);
}

static void _print_slider(const unsigned int segments, const int value, const struct slider_definition *slider)
{
	char line[80];
	
	printf("%i ", value);
	memset(line, ' ', segments);
	line[segments-1] = 0;
	int val_line = value/((slider->val_max)/(segments-1));
	if(val_line < 0) line[0] = '#';
	else if((unsigned)val_line > segments-2) line[segments-2] = '#';
	else line[ val_line ] = '#';
	printf("%s ", line);
}

void slider_print(const struct slider_state * const state)
{
	printf(" %c %c ", (state->buttons.button1)?'1':' ', (state->buttons.button2)?'2':' ');
	if(state->buttons.xtouching) {
		_print_slider(30,  state->xval, &slider_x);
	} else {
		char dummy[32] = { [0 ... 30] = ' ', [31] = 0 };
		printf("%s ", dummy);
	}
	if(state->buttons.ytouching) {
		_print_slider(50,  state->yval, &slider_y);
	} else {
		char dummy[52] = { [0 ... 50] = ' ', [51] = 0 };
		printf("%s ", dummy);
	}
	
	printf("\n");
}

int slider_register_slider_update_callback(slider_update_callback_t callback)
{
	if(slider_update_callback != NULL)
		return -EBUSY;
	slider_update_callback = callback;
	return 0;
}

#include <beacontypes.h>
#include <string.h>
#include <board.h>

#include "touch/ad7147_registers.h"

ad7147_registers_t ad7147_registers;

/* Configuration for 2 sliders and 2 buttons:
 * Stages 0 thru 6 measure inputs 2 thru 8. (y-slider)
 * Stages 7 and 8 measure inputs 11 and 12. (x-slider, shares input 5 with y-slider)
 * Stages 9 and 10 measure inputs 0 and 10. (button 1 and button 2)
 * 
 * This means 11 stages (max.: STAGE10) and no differential configuration
 */

void ad7147_registers_create_initial_configuration(void)
{
	int i;
	memset(&ad7147_registers, 0, sizeof(ad7147_registers));
	
	/* Bank 1 */
	ad7147_registers.banks.bank1.reg[0] = 0x8AA0;
	ad7147_registers.banks.bank1.reg[1] = 0;
	ad7147_registers.banks.bank1.reg[2] = 0x3230;
	ad7147_registers.banks.bank1.reg[3] = 0x419;
	ad7147_registers.banks.bank1.reg[4] = 832;
	ad7147_registers.banks.bank1.reg[5] = 0x7FF; /* STAGEx_LOW_INT_ENABLE */
	ad7147_registers.banks.bank1.reg[6] = 0x7FF; /* STAGEx_HIGH_INT_ENABLE */
	ad7147_registers.banks.bank1.reg[7] = 0; /* STAGEx_COMPLETE_INT_ENABLE */

	
	/* Bank 2 */
	for(i=0; i<12; i++) {
		ad7147_registers.banks.bank2.stage_configuration[i].connection0 = 0x3FFF;
		ad7147_registers.banks.bank2.stage_configuration[i].connection1 = 0x3FFF;
		if(i <= 5) {
			ad7147_registers.banks.bank2.stage_configuration[i].connection0 &= ~(0x1<<((i+2)*2));
		} else if(i==6) {
			ad7147_registers.banks.bank2.stage_configuration[i].connection1 &= ~(0x1<<(((i+2)-7)*2));
		} else if(i<=8) {
			ad7147_registers.banks.bank2.stage_configuration[i].connection1 &= ~(0x1<<(((i+4)-7)*2));
		} else if(i==9) {
			ad7147_registers.banks.bank2.stage_configuration[i].connection0 &= ~(0x1<<(0*2));
		} else if(i==10) {
			ad7147_registers.banks.bank2.stage_configuration[i].connection1 &= ~(0x1<<((10-7)*2));
		}
		
		ad7147_registers.banks.bank2.stage_configuration[i].afe_offset.raw = 0;
		ad7147_registers.banks.bank2.stage_configuration[i].sensitivity = 0x2626;
		ad7147_registers.banks.bank2.stage_configuration[i].offset_low = 1000;
		ad7147_registers.banks.bank2.stage_configuration[i].offset_high = 1000;
		ad7147_registers.banks.bank2.stage_configuration[i].offset_low_clamp = 5000;
		ad7147_registers.banks.bank2.stage_configuration[i].offset_high_clamp = 5000;
	}

	/* Fine tuning */
#ifdef TXTR_PLEXIGLASS
	for(i=0; i<11; i++)
		ad7147_registers.banks.bank2.stage_configuration[i].afe_offset.v.pos_afe_offset = 8;
	ad7147_registers.banks.bank2.stage_configuration[5].afe_offset.v.pos_afe_offset = 0;
	ad7147_registers.banks.bank2.stage_configuration[8].afe_offset.v.pos_afe_offset += 2;
	ad7147_registers.banks.bank2.stage_configuration[9].afe_offset.v.pos_afe_offset += 2;
#else
	for(i=0; i<11; i++)
		ad7147_registers.banks.bank2.stage_configuration[i].afe_offset.v.pos_afe_offset = 1;
	ad7147_registers.banks.bank2.stage_configuration[0].afe_offset.v.pos_afe_offset = 2;
#endif
}

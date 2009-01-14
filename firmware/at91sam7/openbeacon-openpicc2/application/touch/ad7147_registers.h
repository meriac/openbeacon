#ifndef AD7147_REGISTERS_H_
#define AD7147_REGISTERS_H_

struct stage_configuration {
	u_int16_t connection0;
	u_int16_t connection1;
	union {
		struct {
			unsigned int neg_afe_offset:6;
			unsigned int zero1:1;
			unsigned int neg_afe_offset_swap:1;
			unsigned int pos_afe_offset:6;
			unsigned int zero2:1;
			unsigned int pos_afe_offset_swap:1;
		} __attribute__((packed)) v;
		u_int16_t raw;
	} afe_offset;
	u_int16_t sensitivity;
	u_int16_t offset_low;
	u_int16_t offset_high;
	u_int16_t offset_high_clamp;
	u_int16_t offset_low_clamp;
} __attribute__((packed));

typedef union {
	struct {
		union {
			u_int16_t reg[0x80];
		} bank1;
		union {
			u_int16_t reg[96];
			struct stage_configuration stage_configuration[12];
		} bank2;
		union {
			u_int16_t reg[432];
		} bank3;
	} __attribute__((packed)) banks;
	u_int16_t reg[0x290];
} ad7147_registers_t;

extern ad7147_registers_t ad7147_registers;
extern void ad7147_registers_create_initial_configuration(void);

#endif /*AD7147_REGISTERS_H_*/

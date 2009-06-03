#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <task.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "eink/eink.h"
#include "eink/eink_lowlevel.h"

#define EINK_MAX_IMAGE_BUFFER 20
#define EINK_MAX_JOB 32
#define EINK_MAX_JOB_PART 4

#define IMAGE_BUFFER_FREE 0
#define IMAGE_BUFFER_USED 1

struct eink_image_buffer {
	int state; /* enum { IMAGE_BUFFER_FREE, IMAGE_BUFFER_USED } state; */
	unsigned int start_address;
	int rotation_mode; /* enum eink_rotation_mode rotation_mode; */
};

#define JOB_FREE 0
#define JOB_FILLING 1
#define JOB_WAITING 2
#define JOB_RUNNING 3

//#define DEBUG_EINK_MGMT

struct eink_job {
	int state; /* enum { JOB_FREE, JOB_FILLING, JOB_WAITING, JOB_RUNNING } state; */
	unsigned int num_parts;
	unsigned int cookie, job_number;
	struct {
		struct eink_image_buffer *image_buffer;
		int waveform_mode; /* enum eink_waveform_mode waveform_mode; */
		int update_mode; /* enum eink_update_mode update_mode; */
		int is_area_update;
		unsigned int x, y, h, w, xoffset, yoffset;
		int lut, running;
	} parts[EINK_MAX_JOB_PART];
};

static struct eink_image_buffer *image_buffers;
static struct eink_job *jobs;
static unsigned int job_number = 0;

const unsigned int eink_mgmt_image_buffer_size = sizeof(struct eink_image_buffer);
const unsigned int eink_mgmt_job_size = sizeof(struct eink_job);
const unsigned int eink_mgmt_size = sizeof(struct eink_image_buffer)*EINK_MAX_IMAGE_BUFFER + sizeof(struct eink_job)*EINK_MAX_JOB;

static const u_int16_t rotation_mode_map[] = {
		[ROTATION_MODE_0] = EINK_ROTATION_MODE_0,
		[ROTATION_MODE_90] = EINK_ROTATION_MODE_90,
		[ROTATION_MODE_180] = EINK_ROTATION_MODE_180,
		[ROTATION_MODE_270] = EINK_ROTATION_MODE_270,
};

static const u_int16_t pack_mode_map[] = {
		[PACK_MODE_2BIT] = EINK_PACKED_MODE_2BIT,
		[PACK_MODE_3BIT] = EINK_PACKED_MODE_3BIT,
		[PACK_MODE_4BIT] = EINK_PACKED_MODE_4BIT,
		[PACK_MODE_1BYTE] = EINK_PACKED_MODE_1BYTE,
};

static const u_int16_t waveform_mode_map[] = {
		[WAVEFORM_MODE_INIT] = EINK_WAVEFORM_INIT,
		[WAVEFORM_MODE_DU] = EINK_WAVEFORM_DU,
		[WAVEFORM_MODE_GU] = EINK_WAVEFORM_GU,
		[WAVEFORM_MODE_GC] = EINK_WAVEFORM_GC,
};


/* MUST call this function to set up internal management information */
int eink_mgmt_init(void *memory, unsigned int size)
{
	if(size < eink_mgmt_size) {
		return -ENOMEM;
	}
	memset(memory, 0x00, size);
	image_buffers = (struct eink_image_buffer*)memory;
	jobs = (struct eink_job*)(memory + eink_mgmt_image_buffer_size*EINK_MAX_IMAGE_BUFFER);
	return 0;
}

int eink_image_buffer_acquire(eink_image_buffer_t *buf)
{
	int i;
	if(image_buffers == NULL) return -ENOMEM;
	if(buf == NULL) return -EINVAL;
	*buf = NULL;
	for(i=0; i<EINK_MAX_IMAGE_BUFFER; i++) {
		if(image_buffers[i].state == IMAGE_BUFFER_FREE) {
			image_buffers[i].state = IMAGE_BUFFER_USED;
			*buf = &image_buffers[i];
			image_buffers[i].start_address = EINK_IMAGE_BUFFER_START + i * EINK_IMAGE_SIZE;
			break;
		}
	}
	if(*buf != NULL) return 0;
	else return -EBUSY;
}

int eink_image_buffer_load(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode, 
		const unsigned char *data, unsigned int length)
{
	int r = eink_image_buffer_load_begin(buf, pack_mode, rotation_mode);
	if(r<0) return r;
	
	r = eink_image_buffer_load_stream(buf, data, length);
	if(r<0) return r;
	
	r = eink_image_buffer_load_end(buf);
	if(r<0) return r;
	
	return 0;
}

int eink_image_buffer_load_area(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h, 
		const unsigned char *data, unsigned int length)
{
	int r = eink_image_buffer_load_begin_area(buf, pack_mode, rotation_mode, x, y, w, h);
	if(r<0) return r;
	
	r = eink_image_buffer_load_stream(buf, data, length);
	if(r<0) return r;
	
	r = eink_image_buffer_load_end(buf);
	if(r<0) return r;
	
	return 0;
}

int eink_image_buffer_load_begin(eink_image_buffer_t buf, enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode)
{
	if(buf == NULL) return -EINVAL;
	if(buf->state == IMAGE_BUFFER_FREE) return -EINVAL;
	
	buf->rotation_mode = rotation_mode;
	
	eink_init_rotmode(rotation_mode_map[rotation_mode]);
	eink_set_load_address(buf->start_address);
	eink_display_streamed_image_begin(pack_mode_map[pack_mode]);
	
	return 0;
}

int eink_image_buffer_load_begin_area(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	if(buf == NULL) return -EINVAL;
	if(buf->state == IMAGE_BUFFER_FREE) return -EINVAL;
	
	buf->rotation_mode = rotation_mode;
	
	eink_init_rotmode(rotation_mode_map[rotation_mode]);
	eink_set_load_address(buf->start_address);
	eink_display_streamed_area_begin(pack_mode_map[pack_mode], x, y, w, h);
	
	return 0;
}


int eink_image_buffer_load_stream(eink_image_buffer_t buf, const unsigned char *data, unsigned int length)
{
	if(buf == NULL) return -EINVAL;
	if(buf->state == IMAGE_BUFFER_FREE) return -EINVAL;
	
	eink_display_streamed_image_update(data, length);
	
	return 0;
}

int eink_image_buffer_load_end(eink_image_buffer_t buf)
{
	if(buf == NULL) return -EINVAL;
	if(buf->state == IMAGE_BUFFER_FREE) return -EINVAL;
	
	if(!eink_display_streamed_image_end()) 
		return -EIO;
	else
		return 0;
}

int eink_image_buffer_release(eink_image_buffer_t *buf)
{
	if(buf == NULL) return -EINVAL;
	if(*buf == NULL) return -EINVAL;
	if( (*buf)->state != IMAGE_BUFFER_USED ) return -EINVAL;
	(*buf)->state = IMAGE_BUFFER_FREE;
	*buf = NULL;
	return 0;
}

int eink_job_begin(eink_job_t *job, int cookie)
{
	int i;
	if(jobs == NULL) return -ENOMEM;
	if(job == NULL) return -EINVAL;
	*job = NULL;
	for(i=0; i<EINK_MAX_JOB; i++) {
		if(jobs[i].state == JOB_FREE) {
			jobs[i].state = JOB_FILLING;
			*job = &jobs[i];
			break;
		}
	}
	if(*job == NULL) return -EBUSY;
	
	(*job)->cookie = cookie;
	(*job)->num_parts = 0;
	return 0;
}

static int job_add_common(eink_job_t job, eink_image_buffer_t buf, 
		enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	if(job == NULL) return -EINVAL;
	if(buf == NULL) return -EINVAL;
	
	int i = job->num_parts;
	if( (i + 1) >= EINK_MAX_JOB_PART ) return -ENOSPC;
	
	job->parts[i].image_buffer = buf;
	job->parts[i].waveform_mode = waveform_mode;
	job->parts[i].update_mode = update_mode;
	job->parts[i].x = x;
	job->parts[i].y = y;
	job->parts[i].h = h;
	job->parts[i].w = w;
	job->parts[i].xoffset = 0;
	job->parts[i].yoffset = 0;
	
	return i;
}

int eink_job_add(eink_job_t job, eink_image_buffer_t buf, enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode)
{
	int i = job_add_common(job, buf, waveform_mode, update_mode, 0, 0, 0, 0);
	if(i<0) return i;
	job->parts[i].is_area_update = 0;
	job->num_parts++;
	return 0;
}

int eink_job_add_area(eink_job_t job, eink_image_buffer_t buf,
		enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	if(update_mode == UPDATE_MODE_INIT) return -EINVAL;
	int i = job_add_common(job, buf, waveform_mode, update_mode, x, y, w, h);
	if(i<0) return i;
	job->parts[i].is_area_update = 1;
	job->num_parts++;
	return 0;
}

extern int eink_job_add_area_with_offset(eink_job_t job, eink_image_buffer_t buf,
		enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int xoffset, unsigned int yoffset)
{
	if(update_mode == UPDATE_MODE_INIT) return -EINVAL;
	int i = job_add_common(job, buf, waveform_mode, update_mode, x, y, w, h);
	if(i<0) return i;
	job->parts[i].is_area_update = 1;
	job->parts[i].xoffset = xoffset;
	job->parts[i].yoffset = yoffset;
	job->num_parts++;
	return 0;
}


static int eink_job_handle_part(eink_job_t job, unsigned int index);
static int eink_job_find_conflicting_luts(eink_job_t job, unsigned int index);
static int eink_find_free_luts(void);
static void eink_job_cleanup_finished(void);
int eink_job_commit(eink_job_t job)
{
	if(job==NULL) return -EINVAL;
	if(job->state != JOB_FILLING) return -EINVAL;
	job->job_number = job_number++;
	if(job->cookie != 0) {
		taskENTER_CRITICAL();
		int j;
		for(j=0; j<EINK_MAX_JOB; j++) {
			if(jobs[j].state != JOB_WAITING) continue;
			if(jobs[j].cookie == job->cookie)
				jobs[j].state = JOB_FREE;
		}
		taskEXIT_CRITICAL();
	}
	job->state = JOB_WAITING;
	return 0;
}

static int eink_job_execute(eink_job_t job)
{
	unsigned int i;
	int r, free_luts = eink_find_free_luts();
	job->state = JOB_RUNNING;
	eink_write_register(0x330, eink_read_register(0x330) & ~0x80);
#ifdef DEBUG_EINK_MGMT
	printf("pr %04X [%04X:%i] %04X (%04X)\n", eink_read_register(0x336), free_luts, ffs(free_luts), eink_read_register(0x338), eink_read_register(0x33A) );
#endif
	//eink_wait_display_idle();
	for(i=0; i<job->num_parts; i++) {
		int next_lut = ffs(free_luts);
		if(next_lut == 0) {
			job->state = JOB_FREE;
			printf("Aborted due to no LUT available\n");
			return -EBUSY;
		}
		next_lut = next_lut-1;
		free_luts &= ~(1L<<next_lut);
		if(next_lut >= 16) {
			job->state = JOB_FREE;
			printf("Aborted with %i out of %u\n", next_lut, free_luts);
			return -EBUSY;
		}
		job->parts[i].lut = next_lut;
		job->parts[i].running = 0;
	}
	for(i=0; i<job->num_parts; i++) {
#ifdef DEBUG_EINK_MGMT
		printf("pa %i (lut %i)\n", i, job->parts[i].lut);
#endif
		r = eink_job_handle_part(job, i);
		while(eink_read_register(0x338) & 1) ; /* Wait for operation trigger done */
#ifdef DEBUG_EINK_MGMT
		printf("in %04X %04X (%04X)\n", eink_read_register(0x336), eink_read_register(0x338), eink_read_register(0x33A) );
#endif
		if(r<0)
			return r;
	}
#ifdef DEBUG_EINK_MGMT
	printf("po %04X %04X (%04X)\n", eink_read_register(0x336), eink_read_register(0x338), eink_read_register(0x33A) );
	printf("\n");
#endif
	return 0;
}

static int eink_job_handle_part(eink_job_t job, unsigned int index)
{
	eink_init_rotmode(rotation_mode_map[job->parts[index].image_buffer->rotation_mode]);
	
	int lut = job->parts[index].lut;
	int lut_mask = eink_job_find_conflicting_luts(job, index);
	if(lut_mask != 0) printf("m  %04X\n", lut_mask);
	while( eink_read_register(0x336) & lut_mask );
	
	job->parts[index].running = 1;
	
	if(!job->parts[index].is_area_update) {
		eink_set_display_address(job->parts[index].image_buffer->start_address);
		switch(job->parts[index].update_mode) {
		case UPDATE_MODE_INIT:
			eink_perform_command(EINK_CMD_UPD_INIT, 0, 0, 0, 0);
			eink_wait_display_idle();
			break;
		case UPDATE_MODE_PART_SPECIAL: /* This is the same as UPDATE_MODE_PART, with different conflict semantics */ 
		case UPDATE_MODE_PART:
			eink_display_update_part(waveform_mode_map[job->parts[index].waveform_mode] | EINK_LUT_SELECT(lut));
			break;
		case UPDATE_MODE_FULL:
		default:
			eink_display_update_full(waveform_mode_map[job->parts[index].waveform_mode] | EINK_LUT_SELECT(lut));
			break;
		}
	} else {
		if(job->parts[index].xoffset == 0 && job->parts[index].yoffset == 0) {
			eink_set_display_address(job->parts[index].image_buffer->start_address);
		} else {
			int relative_off = job->parts[index].xoffset * EINK_CURRENT_DISPLAY_CONFIGURATION->hsize + job->parts[index].yoffset;
			/* FIXME Implement */
			eink_set_display_address(job->parts[index].image_buffer->start_address-relative_off);
		}
		switch(job->parts[index].update_mode) {
		case UPDATE_MODE_PART_SPECIAL: /* This is the same as UPDATE_MODE_PART, with different conflict semantics */ 
		case UPDATE_MODE_PART:
			eink_display_update_part_area(waveform_mode_map[job->parts[index].waveform_mode] | EINK_LUT_SELECT(lut), 
					job->parts[index].x, job->parts[index].y, job->parts[index].w, job->parts[index].h);
			break;
		case UPDATE_MODE_FULL:
		default:
			eink_display_update_full_area(waveform_mode_map[job->parts[index].waveform_mode] | EINK_LUT_SELECT(lut), 
					job->parts[index].x, job->parts[index].y, job->parts[index].w, job->parts[index].h);
			break;
		}
	}
	return 0;
}

/* Does the rectangle (x1, y1)-(x2,y2) contain the point (x,y)? */
static int __attribute__((pure)) rectangle_contains(int x1, int y1, int x2, int y2,  int x, int y) 
{
	return x1 <= x && x <= x2 && y1 <= y && y <= y2;
}

static int eink_job_conflicts(const eink_job_t job1, unsigned int index1,
		const eink_job_t job2, unsigned int index2)
{
	int x1_1 = job1->parts[index1].x,
		y1_1 = job1->parts[index1].y,
		x2_1 = job1->parts[index1].x + job1->parts[index1].w,
		y2_1 = job1->parts[index1].y + job1->parts[index1].h;
	int x1_2 = job2->parts[index2].x,
		y1_2 = job2->parts[index2].y,
		x2_2 = job2->parts[index2].x + job2->parts[index2].w,
		y2_2 = job2->parts[index2].y + job2->parts[index2].h;
	
	if(job1->parts[index1].update_mode == UPDATE_MODE_PART_SPECIAL &&
			job2->parts[index2].update_mode == UPDATE_MODE_PART_SPECIAL) {
		/* Assume that two UPDATE_MODE_PART_SPECIAL do not conflict with each other,
		 * (the application has to ensure that) */
		return 0; 
	}
	
	if(rectangle_contains(x1_1, y1_1, x2_1, y2_1,  x1_2, y1_2)) return 1;
	if(rectangle_contains(x1_1, y1_1, x2_1, y2_1,  x2_2, y1_2)) return 1;
	if(rectangle_contains(x1_1, y1_1, x2_1, y2_1,  x1_2, y2_2)) return 1;
	if(rectangle_contains(x1_1, y1_1, x2_1, y2_1,  x2_2, y2_2)) return 1;
	
	if(rectangle_contains(x1_2, y1_2, x2_2, y2_2,  x1_1, y1_1)) return 1;
	if(rectangle_contains(x1_2, y1_2, x2_2, y2_2,  x2_1, y1_1)) return 1;
	if(rectangle_contains(x1_2, y1_2, x2_2, y2_2,  x1_1, y2_1)) return 1;
	if(rectangle_contains(x1_2, y1_2, x2_2, y2_2,  x2_1, y2_1)) return 1;
	
	return 0;
}

static int eink_job_find_conflicting_luts(eink_job_t job, unsigned int index)
{
	int result = 0x0;
	unsigned int j,k;
	for(j=0; j<EINK_MAX_JOB; j++) {
		if(jobs[j].state != JOB_RUNNING) continue;
		for(k=0; k<jobs[j].num_parts; k++) {
			if((&jobs[j] == job) && (index == k)) continue;
			if(!jobs[j].parts[k].running) continue;
			if(!job->parts[index].is_area_update) {
				/* Full screen update conflicts with everything */
				result |= 1<<(jobs[j].parts[k].lut);
			} else {
				if(!jobs[j].parts[k].is_area_update || eink_job_conflicts(job, index, &jobs[j], k)) {
					result |= 1<<(jobs[j].parts[k].lut);
					//printf("%p:%i/%p:%i", job, index, &jobs[j], k);
				}
			}
		}
	}
	return result;
}

static int eink_find_free_luts(void)
{
	unsigned int i, j, result = 0xffff;
	for(i=0; i<EINK_MAX_JOB; i++) {
		if(jobs[i].state != JOB_RUNNING) continue;
		for(j=0; j<jobs[i].num_parts; j++) {
			if(!jobs[i].parts[j].running) continue;
			result &= ~(1L<<jobs[i].parts[j].lut);
		}
	}
	return result;
}

static unsigned int eink_count_free_luts(void)
{
	unsigned int i, free_luts = eink_find_free_luts(), result = 0;
	for(i=0; i<32; i++) {
		if(free_luts & 1) result++;
		free_luts >>= 1;
	}
	return result;
}

/* Check for each job whether all the LUTs assigned to that job
 * have finished, and if so, free that job
 */
static void eink_job_cleanup_finished(void)
{
	unsigned int j,k;
	for(j=0; j<EINK_MAX_JOB; j++) {
		if(jobs[j].state != JOB_RUNNING) continue;
		int lut_state = eink_read_register(0x336);
		for(k=0; k<jobs[j].num_parts; k++) {
			if(!jobs[j].parts[k].running) continue;
			if( !(lut_state & (1<<jobs[j].parts[k].lut)) )
				jobs[j].parts[k].running = 0;
		}
		int clean = 1;
		for(k=0; k<jobs[j].num_parts; k++) {
			if(jobs[j].parts[k].running) {
				clean = 0;
				break;
			}
		}
		if(clean)
			jobs[j].state = JOB_FREE;
	}
}

/* Find the smallest job_number'ed WAITING job that can be
 * executed (e.g. does not conflict anything that is RUNNING).
 */
static int eink_mgmt_find_runnable(void)
{
	int result = -1;
	unsigned int j, k;
	for(j=0; j<EINK_MAX_JOB; j++) {
		if(jobs[j].state != JOB_WAITING) continue;
		int eligible = 1;
		if(eink_count_free_luts() < jobs[j].num_parts) {
			eligible = 0;
		}
		for(k = 0; k<jobs[j].num_parts; k++) {
			if(!jobs[j].parts[k].is_area_update) {
				/* Full screen updates can only run when no other job runs */
				int l;
				for(l=0; l<EINK_MAX_JOB; l++) {
					if(jobs[l].state == JOB_RUNNING) {
						eligible = 0;
						break;
					}
				}
			} else {
				if(eink_job_find_conflicting_luts(&jobs[j], k) != 0) {
					eligible = 0;
					break;
				}
			}
			if(!eligible)
				break;
		}
		if(eligible) {
			/* From the set of eligible jobs, find the one with the smallest job_number */
			if(result == -1 || jobs[result].job_number > jobs[j].job_number) {
				result = j;
			}
		}
	}
	return result;
}

int eink_mgmt_flush_queue(void)
{
	eink_job_cleanup_finished();
	int j, modified = 0;
	while( (j=eink_mgmt_find_runnable()) >= 0 ) {
		eink_job_execute(&jobs[j]);
		eink_job_cleanup_finished();
		modified = 1;
	}
	return modified;
}

int eink_job_count_pending(void)
{
	int jobs_left = 0;
	unsigned int i;
	for(i=0; i<EINK_MAX_JOB; i++) 
		if(jobs[i].state != JOB_FREE) {
			jobs_left++;
		}
	return jobs_left;
}

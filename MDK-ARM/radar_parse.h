#ifndef __RADAR_PARSE_H
#define __RADAR_PARSE_H

#include <stdint.h>

/*======================= ????? =======================*/
#define RING_BUF_SIZE   256   /* ???2?? */
#define FRAME_MAX_LEN   64    /* ?? ?32??64,??????? */

/*======================= ????? =======================*/
#define FRAME_HEAD_1    0x53
#define FRAME_HEAD_2    0x59
#define FRAME_TAIL_1    0x54
#define FRAME_TAIL_2    0x43

/* ---- ??: Ctrl=0x81 ---- */
#define CTRL_BREATH         0x81
#define CMD_BREATH_RATE     0x02

/* ---- ??: Ctrl=0x85 (?? ????!??0x82) ---- */
#define CTRL_HEART          0x85
#define CMD_HEART_RATE      0x02

/*======================= ???? =======================*/
void RadarParser_Init(void);
void RadarRingBuf_Put(uint8_t byte);
void RadarParser_FeedFromRingBuf(void);

#endif /* __RADAR_PARSE_H */
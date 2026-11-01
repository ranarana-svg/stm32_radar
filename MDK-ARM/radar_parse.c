/**
  * @file    radar_parse.c
  * @brief   R60ABD1 ??????(??+??)
  *
  * ???:
  *   [0x53][0x59][Ctrl][Cmd][LenH][LenL][Data...][Chk][0x54][0x43]
  *
  * Chk = (0x53+0x59+Ctrl+Cmd+LenH+LenL+Data...) & 0xFF
  */

#include "radar_parse.h"
#include "main.h"
#include <string.h>

/*======================= ????? =======================*/
static volatile uint8_t  s_ring_buf[RING_BUF_SIZE];
static volatile uint8_t  s_ring_head = 0;
static volatile uint8_t  s_ring_tail = 0;

/*======================= ????? =======================*/
typedef enum {
    PARSE_WAIT_HEAD1 = 0,
    PARSE_WAIT_HEAD2,
    PARSE_RECV_BODY,
} ParseState_t;

static ParseState_t s_state = PARSE_WAIT_HEAD1;
static uint8_t  s_frame_buf[FRAME_MAX_LEN];
static uint16_t s_frame_idx   = 0;
static uint16_t s_frame_total = 0;
static uint16_t s_data_len    = 0;

/*======================= ?????? =======================*/
static void RingBuf_PutRaw(uint8_t byte);
static int  RingBuf_GetRaw(uint8_t *byte);
static void ParseOneByte(uint8_t byte);
static void ProcessCompleteFrame(void);

/*======================= ???? =======================*/

void RadarParser_Init(void)
{
    s_ring_head = 0;
    s_ring_tail = 0;
    s_state     = PARSE_WAIT_HEAD1;
    s_frame_idx = 0;
    s_frame_total = 0;
    s_data_len = 0;
    memset(s_frame_buf, 0, sizeof(s_frame_buf));
}

/** ?????:????1????buf */
void RadarRingBuf_Put(uint8_t byte)
{
    RingBuf_PutRaw(byte);
}

/** ?????:?buf???????? */
void RadarParser_FeedFromRingBuf(void)
{
    uint8_t byte;
    while (RingBuf_GetRaw(&byte))
    {
        ParseOneByte(byte);
    }
}

/*======================= ??????? =======================*/

static void RingBuf_PutRaw(uint8_t byte)
{
    uint8_t next = (s_ring_head + 1) & (RING_BUF_SIZE - 1);
    if (next == s_ring_tail)
    {
        /* buf?,?????? */
        s_ring_tail = (s_ring_tail + 1) & (RING_BUF_SIZE - 1);
    }
    s_ring_buf[s_ring_head] = byte;
    s_ring_head = next;
}

static int RingBuf_GetRaw(uint8_t *byte)
{
    if (s_ring_head == s_ring_tail)
        return 0;
    *byte = s_ring_buf[s_ring_tail];
    s_ring_tail = (s_ring_tail + 1) & (RING_BUF_SIZE - 1);
    return 1;
}

/*======================= ????? =======================*/

/**
  * s_frame_buf ??:
  *  [0] 0x53   ??1
  *  [1] 0x59   ??2
  *  [2] Ctrl   ???
  *  [3] Cmd    ???
  *  [4] LenH   ???????
  *  [5] LenL   ???????
  *  [6..6+N-1] Data
  *  [6+N]      Chk ??
  *  [7+N]      0x54 ??1
  *  [8+N]      0x43 ??2
  *
  *  frame_total = 9 + N  (??2+ctrl1+cmd1+len2+dataN+chk1+??2)
  */
static void ParseOneByte(uint8_t byte)
{
    switch (s_state)
    {
    case PARSE_WAIT_HEAD1:
        if (byte == FRAME_HEAD_1)
        {
            s_frame_buf[0] = byte;
            s_frame_idx = 1;
            s_state = PARSE_WAIT_HEAD2;
        }
        break;

    case PARSE_WAIT_HEAD2:
        if (byte == FRAME_HEAD_2)
        {
            s_frame_buf[1] = byte;
            s_frame_idx = 2;
            s_state = PARSE_RECV_BODY;
        }
        else
        {
            s_state = PARSE_WAIT_HEAD1;
        }
        break;

    case PARSE_RECV_BODY:
        if (s_frame_idx < FRAME_MAX_LEN)
        {
            s_frame_buf[s_frame_idx++] = byte;
        }
        else
        {
            /* ???????,?????? */
            s_state = PARSE_WAIT_HEAD1;
            s_frame_idx = 0;
            break;
        }

        /* ??? LenH ? LenL,??????(???) */
        if (s_frame_idx == 6)
        {
            s_data_len = ((uint16_t)s_frame_buf[4] << 8) | s_frame_buf[5];
            /* 2(Head)+1(Ctrl)+1(Cmd)+2(Len)+N(Data)+1(Chk)+2(Tail) = 9+N */
            s_frame_total = 9 + s_data_len;

            if (s_frame_total > FRAME_MAX_LEN)
            {
                /* ????,????? */
                s_state = PARSE_WAIT_HEAD1;
                s_frame_idx = 0;
            }
        }

        /* ??????,????? */
        if (s_frame_idx >= 6 && s_frame_idx >= s_frame_total)
        {
            ProcessCompleteFrame();
            s_state = PARSE_WAIT_HEAD1;
            s_frame_idx = 0;
        }
        break;

    default:
        s_state = PARSE_WAIT_HEAD1;
        s_frame_idx = 0;
        break;
    }
}

/*======================= ????? =======================*/

static void ProcessCompleteFrame(void)
{
    if (s_frame_buf[0] != FRAME_HEAD_1 || s_frame_buf[1] != FRAME_HEAD_2)
        return;

    uint8_t ctrl = s_frame_buf[2];
    uint8_t cmd  = s_frame_buf[3];
    uint16_t data_len = ((uint16_t)s_frame_buf[4] << 8) | s_frame_buf[5];

    // ???? (data_len=1?: buf[7]=0x54, buf[8]=0x43)
    if (s_frame_buf[7 + data_len] != FRAME_TAIL_1 ||
        s_frame_buf[8 + data_len] != FRAME_TAIL_2)
        return;

    // ???
    uint32_t sum = 0;
    for (uint16_t i = 0; i < 6 + data_len; i++)
        sum += s_frame_buf[i];

    if ((uint8_t)(sum & 0xFF) != s_frame_buf[6 + data_len])
        return;

    /* ? ????:data_len >= 1,?? buf[6] */
    if (ctrl == CTRL_BREATH && cmd == CMD_BREATH_RATE && data_len >= 1)
    {
        g_breathRate = s_frame_buf[6];  // ?? ???7??6
        g_br_updated = 1;
    }
    else if (ctrl == CTRL_HEART && cmd == CMD_HEART_RATE && data_len >= 1)
    {
        g_heartRate = s_frame_buf[6];   // ?? ???7??6
        g_hr_updated = 1;
    }
}
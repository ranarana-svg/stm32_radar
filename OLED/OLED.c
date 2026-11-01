#include "oled.h"
#include "i2c.h"
#include "oledfont.h"
#include <string.h>
#include <math.h>

// SSD1306初始化命令序列
static uint8_t CMD_Data[]={
0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F,
0xC8, 0xD3, 0x00, 0xD5, 0x80, 0xD8, 0x05, 0xD9, 0xF1, 0xDA, 0x12,
0xD8, 0x30, 0x8D, 0x14, 0xAF};

// 显存缓冲区 8页 × 128列，128*64
static uint8_t oled_buf[8][128];

// 内部：发送全部初始化命令，static不对外
static void WriteCmd(void)
{
	uint8_t i = 0;
	for(i=0; i<27; i++)
	{
		HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&CMD_Data[i],1,0x100);
	}
}

// 发送命令
static void OLED_WR_CMD(uint8_t cmd)
{
	HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}

// 发送显示点阵数据
static void OLED_WR_DATA(uint8_t data)
{
	HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
}

// 设置坐标 x:0~127  y:PAGE 0~7
static void OLED_Set_Pos(uint8_t x, uint8_t y)
{
	OLED_WR_CMD(0xb0+y);
	OLED_WR_CMD(((x&0xf0)>>4)|0x10);
	OLED_WR_CMD(x&0x0f);
}


// OLED初始化
void OLED_Init(void)
{
	HAL_Delay(200);
	WriteCmd();
	memset(oled_buf,0,sizeof(oled_buf));
}

// 将显存全部刷新输出到屏幕
void OLED_Refresh(void)
{
	uint8_t page, x;
	for(page = 0; page < 8; page++)
	{
		OLED_WR_CMD(0xb0 + page);
		OLED_WR_CMD(0x00);
		OLED_WR_CMD(0x10);
		for(x = 0; x < 128; x++)
		{
			OLED_WR_DATA(oled_buf[page][x]);
		}
	}
}

// 清屏：清空显存 + 清空硬件屏幕
void OLED_Clear(void)
{
	memset(oled_buf, 0, sizeof(oled_buf));
}

void OLED_ShowMap(uint8_t x, uint8_t y, const uint8_t *data, uint16_t width, uint16_t height)
{
    uint16_t dx, dy;
    if (data == NULL || width == 0 || height == 0 || x >= 128 || y >= 64)
        return;

    for (dy = 0; dy < height && dy < 64U - y; dy++)
    {
        for (dx = 0; dx < width && dx < 128U - x; dx++)
        {
            uint8_t pixel = (data[(uint32_t)(dy / 8U) * width + dx] >> (dy % 8U)) & 1U;
            OLED_DrawPoint((uint8_t)(x + dx), (uint8_t)(y + dy), pixel);
        }
    }
}

// 打开显示
void OLED_Display_On(void)
{
	OLED_WR_CMD(0X8D);
	OLED_WR_CMD(0X14);
	OLED_WR_CMD(0XAF);
}

// 关闭显示
void OLED_Display_Off(void)
{
	OLED_WR_CMD(0X8D);
	OLED_WR_CMD(0X10);
	OLED_WR_CMD(0XAE);
}

// 数字显示辅助幂次函数
static unsigned int oled_pow(uint8_t m,uint8_t n)
{
	unsigned int result=1;
	while(n--)result*=m;
	return result;
}

/**
 * @brief 画点
 * @param x 0~127 像素X
 * @param y 0~63 像素Y
 * @param mode 1点亮 0熄灭
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode)
{
	if(x>127 || y>63) return;
	uint8_t page = y / 8;
	uint8_t bit  = y % 8;
	if(mode)
	{
		oled_buf[page][x] |= (1 << bit);
	}
	else
	{
		oled_buf[page][x] &= ~(1 << bit);
	}
}

/**
 * @brief 画线
 * @param x1,y1 起点像素
 * @param x2,y2 终点像素
 */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	int dx, dy, sx, sy, err, e2;
	int x = x1;
	int y = y1;

	dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
	dy = (y2 > y1) ? (y1 - y2) : (y2 - y1);
	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;
	err = dx + dy;

	while(1)
	{
		OLED_DrawPoint((uint8_t)x, (uint8_t)y, 1);
		if(x == x2 && y == y2) break;
		e2 = 2 * err;
		if(e2 >= dy)
		{
			err += dy;
			x += sx;
		}
		if(e2 <= dx)
		{
			err += dx;
			y += sy;
		}
	}
}

/**
 * @brief 显示单个ASCII字符，写入显存
 * @param x,y 真实像素坐标
 * @param frontsize 12=6x8，16=8x16
 */
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t frontsize)
{
	unsigned char c=0,i=0;
	c=chr-' ';
	if(x>127){x=0;y+=frontsize;}

	if(frontsize ==16)
	{
		// 8x16字符，上下两个page
		for(i = 0; i < 8; i++)
		{
			uint8_t dat = F8X16[c*16+i];
			for(uint8_t bit=0;bit<8;bit++)
			{
				if(dat & (1<<bit))
					OLED_DrawPoint(x+i, y+bit, 1);
			}
		}
		for(i = 0; i < 8; i++)
		{
			uint8_t dat = F8X16[c*16+i+8];
			for(uint8_t bit=0;bit<8;bit++)
			{
				if(dat & (1<<bit))
					OLED_DrawPoint(x+i, y+8+bit, 1);
			}
		}
	}
	else
	{
		// 6x8字符
		for(i = 0; i < 6; i++)
		{
			uint8_t dat = F6x8[c][i];
			for(uint8_t bit=0;bit<8;bit++)
			{
				if(dat & (1<<bit))
					OLED_DrawPoint(x+i, y+bit, 1);
			}
		}
	}
}

/**
 * @brief 显示数字（写入显存）
 */
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t frontsize)
{
	uint8_t t,temp;
	uint8_t enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(frontsize/2)*t,y,' ',frontsize);
				continue;
			}else enshow=1;
		}
		OLED_ShowChar(x+(frontsize/2)*t,y,temp+'0',frontsize);
	}
}

/**
 * @brief 显示字符串（写入显存）
 */
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t frontsize)
{
	unsigned char j=0;
	while (chr[j]!='\0')
	{
		OLED_ShowChar(x,y,chr[j],frontsize);
		if(frontsize == 16)
			x+=8;
		else
			x+=6;

		if(x>120){x=0;y+=frontsize;}
		j++;
	}
}

/**
* @brief 显示点阵字
 * @param data   点阵字的数据
 * @param size   字体大小
 * @param index  编号
 * @note 默认：列行式 + 阴码
*       每次要使用 OLED_Refresh() 才会刷新显示
 */
void OLED_ShowChinese(uint8_t x, uint8_t y, const void *data, uint8_t size, uint16_t index)
{
    const uint8_t *glyph;
    uint16_t row_bytes;
    uint16_t dx, dy;

    if (data == NULL || size == 0 || size > 64 || x >= 128 || y >= 64)
        return;

    row_bytes = (uint16_t)size * ((size + 7U) / 8U);
    glyph = (const uint8_t *)data + (uint32_t)index * row_bytes;

    for (dy = 0; dy < size && (uint16_t)y + dy < 64; dy++)
    {
        for (dx = 0; dx < size && (uint16_t)x + dx < 128; dx++)
        {
            uint8_t pixel = (glyph[(dy / 8U) * size + dx] >> (dy % 8U)) & 1U;
            OLED_DrawPoint((uint8_t)(x + dx), (uint8_t)(y + dy), pixel);
        }
    }
}


/**
 * @brief 打点方式绘制正弦波
 * @param amp 振幅像素 10~28
 * @param centerY 波形中心Y，128*64屏一般32
 * @param phase 相位弧度，变化phase实现滚动
 */
void OLED_DrawSineWave_Point(uint8_t amp, uint8_t centerY, float phase)
{
    for(uint8_t x = 0; x < 128; x++)
    {
        float theta = 2.0f * 3.1415926f * x / 128.0f + phase;
        int16_t y_val = centerY - (int16_t)(amp * sinf(theta));

        //边界钳位
        if(y_val < 0)  y_val = 0;
        if(y_val > 63) y_val = 63;

        OLED_DrawPoint(x, (uint8_t)y_val, 1);
    }
}


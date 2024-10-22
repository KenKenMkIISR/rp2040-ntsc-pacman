/*----------------------------------------------------------------------------

Copyright (C) 2024, KenKen, all right reserved.

This program supplied herewith by KenKen is free software; you can
redistribute it and/or modify it under the terms of the same license written
here and only for non-commercial purpose.

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of FITNESS FOR A PARTICULAR
PURPOSE. The copyright owner and contributors are NOT LIABLE for any damages
caused by using this program.

----------------------------------------------------------------------------*/

// This signal generation program (using PWM and DMA) is the idea of @lovyan03.
// https://github.com/lovyan03/

//#pragma GCC optimize ("O3")

#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "rp2040_pwm_ntsc_textgraph.h"

// NTSC信号をPWM出力するピン
#define PIN_OUTPUT 27

// デバッグ用、割込み処理中HIGHになるピン
//#define PIN_DEBUG_BUSY 15

uint8_t TVRAM[ATTROFFSET*2+1];
uint8_t framebuffer[FRAME_WIDTH * FRAME_HEIGHT] __attribute__ ((aligned (4)));

volatile uint8_t drawing; //　映像区間処理中は-1、その他は0
volatile uint16_t drawcount=0; //　1画面表示終了ごとに1足す。アプリ側で0にする。

// DMAピンポンバッファ
uint16_t dma_buffer[2][(NUM_LINE_SAMPLES+3)&~3u] __attribute__ ((aligned (4)));

// カラーパレット
uint16_t color_tbl[4*256] __attribute__ ((aligned (4)));

static uint pwm_dma_chan0,pwm_dma_chan1;

static void makeDmaBuffer(uint16_t* buf, size_t line_num)
{
	static uint8_t* fbp = framebuffer;
	static uint8_t* tvp = TVRAM;
	static uint8_t tline = 0;
	uint16_t* b = buf;

	if (line_num < 2)
	{
		for (int j = 0; j < NUM_LINE_SAMPLES-H_SYNC; j++) *b++ = 0;
		while (b < buf + NUM_LINE_SAMPLES) *b++ = 2;
	}
		else if(line_num==V_SYNC || line_num==V_SYNC+1)
	{
		for (int j = 0; j < H_SYNC; j++) *b++ = 0;
		for (int j = 0; j < 8; j++) *b++ = 2;
		for (int j = 0; j < 9; j++)
		{
			*b++=2;
			*b++=1;
			*b++=2;
			*b++=3;
		}
		while (b < buf + NUM_LINE_SAMPLES) *b++ = 2;
	}
	else if(line_num>=V_SYNC+V_PREEQ && line_num<V_SYNC+V_PREEQ+FRAME_HEIGHT)
	{
		b+=H_PICTURE;
		if (line_num == V_SYNC + V_PREEQ)
		{
			fbp = framebuffer;
			tvp = TVRAM;
			tline = 0;
			drawing = -1;
		}
		for(int i=0;i<WIDTH_X;i++)
		{
			uint8_t d=FontData[*tvp *8 +tline];
			uint16_t* clp=color_tbl+(*(tvp+ATTROFFSET))*4;
			uint32_t c1=*((uint32_t*)clp);
			uint32_t c2=*((uint32_t*)(clp+2));
			for(int j=0;j<4;j++)
			{
			uint16_t t=*(uint16_t*)fbp;
			if(d & 0x80){
				*((uint32_t*)b)=c1;
			}
			else{
				*((uint32_t*)b)=*((uint32_t*)(color_tbl+(t & 0xff)*4));
			}
			b+=2;
			if(d & 0x40){
				*((uint32_t*)b)=c2;
			}
			else{
				*((uint32_t*)b)=*((uint32_t*)(color_tbl+(t >> 8)*4+2));
			}
			b+=2;
			fbp+=2;
			d<<=2;
			}
			tvp++;
		}
		tline++;
		if(tline<8) tvp-=WIDTH_X;
		else tline=0;
	}
	else if(line_num==V_SYNC+V_PREEQ+FRAME_HEIGHT || line_num==V_SYNC+V_PREEQ+FRAME_HEIGHT+1)
	{
		if(line_num==V_SYNC+V_PREEQ+FRAME_HEIGHT){
			drawing=0;
			drawcount++;
		}
		b+=H_PICTURE;
		for(int i=0;i<FRAME_WIDTH*2;i++) *b++ = 2;
	}
}

void set_palette(unsigned char c,unsigned char b,unsigned char r,unsigned char g)
{
	// カラーパレット設定
	// c:パレット番号0-255、r,g,b:0-255

	// 輝度Y=0.587*G+0.114*B+0.299*R
	// 信号N=Y+0.4921*(B-Y)*sinθ+0.8773*(R-Y)*cosθ
	// 出力データS=N*(white-black)/255+black  white=9,black=2

	int32_t y,s;
	y=(150*g+29*b+77*r+128)/256;

	// 微妙な色ズレを修正
/*
	float th=30.0/180*3.14159;
	int32_t b_y_1=(int32_t)((b-y)*256*7*0.4921*sinf(0+th));
	int32_t r_y_1=(int32_t)((r-y)*256*7*0.8773*cosf(0+th));
	int32_t b_y_2=(int32_t)((b-y)*256*7*0.4921*sinf(3.14159/2+th));
	int32_t r_y_2=(int32_t)((r-y)*256*7*0.8773*cosf(3.14159/2+th));
*/
	int32_t b_y_1=(b-y)*441;
	int32_t r_y_1=(r-y)*1361;
	int32_t b_y_2=(b-y)*764;
	int32_t r_y_2=(r-y)*(-786);

	s=(y*1792 + b_y_1 + r_y_1 + 2*65536+32768)/65536;
	color_tbl[c*4] = s<0 ? 0 : s;
	s=(y*1792 + b_y_2 + r_y_2 + 2*65536+32768)/65536;
	color_tbl[c*4+1] = s<0 ? 0 : s;
	s=(y*1792 - b_y_1 - r_y_1 + 2*65536+32768)/65536;
	color_tbl[c*4+2] = s<0 ? 0 : s;
	s=(y*1792 - b_y_2 - r_y_2 + 2*65536+32768)/65536;
	color_tbl[c*4+3] = s<0 ? 0 : s;
}

static void init_palette(void){
	//カラーパレット初期化
	int i;
	for(i=0;i<8;i++){
		set_palette(i,255*(i&1),255*((i>>1)&1),255*(i>>2));
	}
	for(i=0;i<8;i++){
		set_palette(i+8,128*(i&1),128*((i>>1)&1),128*(i>>2));
	}
	for(i=16;i<256;i++){
		set_palette(i,255,255,255);
	}
}

static void irq_handler(void) {
	static bool flip = true;
	static size_t scanline = 0;
	volatile uint32_t s0;

#if defined ( PIN_DEBUG_BUSY )
	gpio_put(PIN_DEBUG_BUSY, 1);
#endif
	s0=dma_hw->ints0;
	dma_hw->ints0 = s0;
	if(s0 & (1u << pwm_dma_chan1)){
		makeDmaBuffer(dma_buffer[1], scanline);
		dma_channel_set_read_addr(pwm_dma_chan1, dma_buffer[1], false);
	}
	else{
		makeDmaBuffer(dma_buffer[0], scanline);
		dma_channel_set_read_addr(pwm_dma_chan0, dma_buffer[0], false);
	}
	if (++scanline >= NUM_LINES) {
		scanline = 0;
	}
#if defined ( PIN_DEBUG_BUSY )
	gpio_put(PIN_DEBUG_BUSY, 0);
#endif
}

// グラフィック画面クリア
void g_clearscreen(void)
{
	unsigned int *vp;
	int i;
	vp=(unsigned int *)GVRAM;
	for(i=0;i<X_RES*Y_RES/4;i++) *vp++=0;
}
//テキスト画面クリア
void clearscreen(void)
{
	unsigned int *vp;
	int i;
	vp=(unsigned int *)TVRAM;
	for(i=0;i<WIDTH_X*WIDTH_Y*2/4;i++) *vp++=0;
}

void rp2040_pwm_ntsc_init(void)
{
#if defined ( PIN_DEBUG_BUSY )
	gpio_init(PIN_DEBUG_BUSY);
	gpio_set_dir(PIN_DEBUG_BUSY, GPIO_OUT);
#endif
	init_palette();
	g_clearscreen();
	clearscreen();

	// CPUを157.5MHzで動作させる
	uint32_t freq_khz = 157500;

	// PWM周期を11サイクルとする (157.5 [MHz] / 11 = 14318181 [Hz])
	uint32_t pwm_div = 11;

	// ※ NTSCのカラー信号を1周期4サンプルで出力する。
	// 出力されるカラーバースト信号は  14318181 [Hz] / 4 = 3579545 [Hz] となる。

	set_sys_clock_khz(freq_khz, true);

	gpio_set_function(PIN_OUTPUT, GPIO_FUNC_PWM);
	uint pwm_slice_num = pwm_gpio_to_slice_num(PIN_OUTPUT);

	pwm_config config = pwm_get_default_config();
	pwm_config_set_clkdiv(&config, 1);

	pwm_init(pwm_slice_num, &config, true);
	pwm_set_wrap(pwm_slice_num, pwm_div - 1);

	volatile void* wr_addr = &pwm_hw->slice[pwm_slice_num].cc;
	wr_addr = (volatile void*)(((uintptr_t)wr_addr) + 2);

	pwm_dma_chan0 = dma_claim_unused_channel(true);
	pwm_dma_chan1 = dma_claim_unused_channel(true);

	dma_channel_config c = dma_channel_get_default_config(pwm_dma_chan0);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
	channel_config_set_read_increment(&c, true);
	channel_config_set_write_increment(&c, false);
	channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + pwm_slice_num);
	channel_config_set_chain_to(&c, pwm_dma_chan1);
	dma_channel_configure(
		pwm_dma_chan0,
		&c,
		wr_addr,
		dma_buffer[0],
		NUM_LINE_SAMPLES,
		false
	);

	c = dma_channel_get_default_config(pwm_dma_chan1);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
	channel_config_set_read_increment(&c, true);
	channel_config_set_write_increment(&c, false);
	channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + pwm_slice_num);
	channel_config_set_chain_to(&c, pwm_dma_chan0);
	dma_channel_configure(
		pwm_dma_chan1,
		&c,
		wr_addr,
		dma_buffer[1],
		NUM_LINE_SAMPLES,
		false
	);

	makeDmaBuffer(dma_buffer[0], 0);
	makeDmaBuffer(dma_buffer[1], 1);

	dma_set_irq0_channel_mask_enabled((1u<<pwm_dma_chan0)|(1u<<pwm_dma_chan1),true);
//	dma_channel_set_irq0_enabled(pwm_dma_chan0, true);
	irq_set_exclusive_handler(DMA_IRQ_0, irq_handler);
	irq_set_enabled(DMA_IRQ_0, true);
	dma_start_channel_mask(1u << pwm_dma_chan0);
}

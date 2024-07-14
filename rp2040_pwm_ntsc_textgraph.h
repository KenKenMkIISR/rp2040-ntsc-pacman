#include <stdint.h>

#define FRAME_WIDTH 336
#define FRAME_HEIGHT 216
#define WIDTH_X 42
#define WIDTH_Y 27
#define ATTROFFSET (WIDTH_X*WIDTH_Y)
#define X_RES FRAME_WIDTH
#define Y_RES FRAME_HEIGHT
#define GVRAM framebuffer

// NTSC出力 1ラインあたりのサンプル数
//#define NUM_LINE_SAMPLES 908  // 227 * 4
#define NUM_LINE_SAMPLES 907  // 227 * 4 -1

// NTSC出力 走査線数
#define NUM_LINES 262  // 走査線262本
#define V_SYNC		10	// 垂直同期本数
#define V_PREEQ		26	// ブランキング区間上側
#define H_SYNC		68	// 水平同期幅、約4.7μsec
#define H_PICTURE (H_SYNC+8+9*4+60) // 映像開始位置

void g_clearscreen(void);
void clearscreen(void);
void rp2040_pwm_ntsc_init(void);
void set_palette(unsigned char c,unsigned char b,unsigned char r,unsigned char g);

extern volatile uint16_t drawcount;
extern uint8_t TVRAM[];
extern uint8_t framebuffer[];
extern const uint8_t FontData[];

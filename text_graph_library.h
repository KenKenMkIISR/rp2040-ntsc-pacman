#include <stdint.h>
#include "rp2040_pwm_ntsc_textgraph.h"

void g_pset(int x, int y, int c);
void g_putbmpmn(int x,int y,char m,char n,const unsigned char bmp[]);
void g_clrbmpmn(int x,int y,char m,char n);
void g_gline(int x1,int y1,int x2,int y2,unsigned int c);
void g_circle(int x0,int y0,unsigned int r,unsigned int c);
void g_hline(int x1,int x2,int y,unsigned int c);
void g_boxfill(int x1,int y1,int x2,int y2,unsigned int c);
void g_circlefill(int x0,int y0,unsigned int r,unsigned int c);
void g_putfont(int x,int y,unsigned int c,int bc,unsigned char n);
void g_printstr(int x,int y,unsigned int c,int bc,unsigned char *s);
void g_printnum(int x,int y,unsigned char c,int bc,unsigned int n);
void g_printnum2(int x,int y,unsigned char c,int bc,unsigned int n,unsigned char e);
unsigned int g_color(int x,int y);
void setcursor(unsigned char x,unsigned char y,unsigned char c);
void setcursorcolor(unsigned char c);
void printchar(unsigned char n);
void printstr(unsigned char *s);
void printnum(unsigned int n);
void printnum2(unsigned int n,unsigned char e);
void cls(void);

extern const unsigned char FontData[];
extern uint8_t *cursor;
extern uint8_t cursorcolor;


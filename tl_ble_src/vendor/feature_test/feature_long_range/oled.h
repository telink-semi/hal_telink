#ifndef    __OLED_H
#define    __OLED_H
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#define     OLED_ADDR                    0x78

extern char oled_show_buff[32];

void OLED_CLS(void);
void OLED_Fill(uint8_t fill_Data);
void OLED_FillY(uint8_t Y);
void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t ch[]);
void Printf_OLED(uint8_t x, uint8_t y, uint8_t ch[],uint32_t size);

void OLED_Show_Arrow(uint8_t y);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_ColorTurn(uint8_t i);
void OLED_Init(void);
void OLED_Test(void);
void clear_oled_buffer(void);

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
#endif

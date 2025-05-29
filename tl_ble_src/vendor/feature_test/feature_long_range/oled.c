#include "app_config.h"
#include "drivers.h"
#include "oled.h"
#include "font.h"

char oled_show_buff[32];

void WriteCmd(uint8_t I2C_Command)
{
    uint8_t write_buff[2] = {0x00,I2C_Command};
    i2c_master_write(OLED_ADDR,write_buff, 2);
}

void WriteDat(uint8_t I2C_Data)
{
    uint8_t write_buff[2] = {0x40,I2C_Data};
    i2c_master_write(OLED_ADDR,write_buff, 2);
}

void OLED_SetPos(uint8_t x, uint8_t y)
{
    WriteCmd(0xb0+y);
    WriteCmd(((x&0xf0)>>4)|0x10);
    WriteCmd((x&0x0f)|0x01);
}

void OLED_Fill(uint8_t fill_Data)
{
    uint8_t m,n;
    for(m=0;m<8;m++){
        WriteCmd(0xb0+m);
        WriteCmd(0x00);
        WriteCmd(0x10);
        for(n=0;n<128;n++){
            WriteDat(fill_Data);
        }
    }
}

void OLED_CLS(void)
{
    OLED_Fill(0x00);
}

void OLED_FillY(uint8_t Y)
{
    uint8_t m,n;
    for(m=Y; m < Y+2;m++){
        WriteCmd(0xb0+m);
        WriteCmd(0x00);
        WriteCmd(0x10);
        for(n=0;n<128;n++){
            WriteDat(0X00);
        }
    }
}

void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t ch[])
{
    uint8_t c = 0,i = 0,j = 0;
    while(ch[j] != '\0'){
        c = ch[j] - 32;
        if(x > 120){
            x = 0;
            y+=2;
        }
        OLED_SetPos(x,y);
        for(i=0;i<8;i++)
            WriteDat(F8X16[c*16+i]);
        OLED_SetPos(x,y+1);
        for(i=0;i<8;i++)
            WriteDat(F8X16[c*16+i+8]);
        x += 8;
        j++;
    }
}

void Printf_OLED(uint8_t x, uint8_t y, uint8_t ch[],uint32_t size)
{
    uint8_t c = 0,i = 0,j = 0;
//    while(ch[j] != '\0')
//    {
    for (j = 0; j < size; j++){
        c = ch[j] - 32;
        if(x > 120)
        {
            x = 0;
            y+=2;
        }
        OLED_SetPos(x,y);
        for(i=0;i<8;i++)
            WriteDat(F8X16[c*16+i]);
        OLED_SetPos(x,y+1);
        for(i=0;i<8;i++)
            WriteDat(F8X16[c*16+i+8]);
        x += 8;
        // j++;
    }
}

void OLED_Show_Arrow(uint8_t y)
{
    for(uint8_t j = 0; j < 8; j = j+2){
        if(j == y)  OLED_ShowStr(120,y,(uint8_t *)"<");
        else        OLED_ShowStr(120,j,(uint8_t *)" ");
    }
}

void OLED_ON(void)
{
    WriteCmd(0X8D);
    WriteCmd(0X14);
    WriteCmd(0XAF);
}

void OLED_OFF(void)
{
    WriteCmd(0X8D);
    WriteCmd(0X10);
    WriteCmd(0XAE);
}

void OLED_ColorTurn(uint8_t i)
{
    if(i == 0){
        WriteCmd(0xA6);
    }
    if(i == 1){
        WriteCmd(0xA7);
    }
}

void OLED_Init(void)
{
    delay_ms(100);
    WriteCmd(0xAE);
    WriteCmd(0xD5);
    WriteCmd(0x80);
    WriteCmd(0xA8);
    WriteCmd(0x3F);
    WriteCmd(0xD3);
    WriteCmd(0x00);
    WriteCmd(0xDA);
    WriteCmd(0x10);
    WriteCmd(0xA1);
    WriteCmd(0xC0);
    WriteCmd(0x81);
    WriteCmd(0x5F);
    WriteCmd(0xD9);
    WriteCmd(0x22);
    WriteCmd(0xDB);
    WriteCmd(0x30);
    WriteCmd(0x8D);
    WriteCmd(0x14);
    WriteCmd(0x2E);
    WriteCmd(0xA7);
    WriteCmd(0xA4);
    OLED_CLS();
    WriteCmd(0xAF);
    OLED_ColorTurn(0);
}

void clear_oled_buffer(void)
{
//    for(unsigned char i = 0;i < 32;i++)
//    {
//        oled_show_buff[i] = ' ';
//    }
}

void OLED_Test(void)
{
    static unsigned int number = 0;
    static unsigned char black_flag = 1;
//    uint8_t oled_show_buff[32];
    sprintf((char*)oled_show_buff,"number:%06d",number);
    OLED_ShowStr(10,0,(uint8_t *)oled_show_buff);
    OLED_ShowStr(10,2,(uint8_t *)oled_show_buff);
    OLED_ShowStr(10,4,(uint8_t *)oled_show_buff);
    OLED_ShowStr(10,6,(uint8_t *)oled_show_buff);
    OLED_Show_Arrow((number%4)*2);
    (number%4) == 0 ? black_flag=!black_flag:0;
    OLED_ColorTurn(black_flag);
    number++;
}

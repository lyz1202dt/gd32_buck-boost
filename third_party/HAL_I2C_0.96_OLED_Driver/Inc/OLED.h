#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "OLED_Data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*write)(void *context, uint8_t control, const uint8_t *data, size_t size);
    void (*delay_ms)(void *context, uint32_t milliseconds);
} OLED_Callbacks;

typedef struct {
    uint8_t buffer[8][128];
    OLED_Callbacks callbacks;
    void *context;
} OLED_Handle;

bool OLED_RegisterCallbacks(OLED_Handle *oled, const OLED_Callbacks *callbacks, void *context);

/*参数宏定义*********************/

/*FontSize参数取值*/
/*此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度*/
#define OLED_8X16				8
#define OLED_6X8				6

/*IsFilled参数数值*/
#define OLED_UNFILLED			0
#define OLED_FILLED				1

/*********************参数宏定义*/


/*函数声明*********************/

/*初始化函数*/
bool OLED_Init(OLED_Handle *oled);

/*更新函数*/
bool OLED_Update(OLED_Handle *oled);
bool OLED_UpdateArea(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显存控制函数*/
void OLED_Clear(OLED_Handle *oled);
void OLED_ClearArea(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(OLED_Handle *oled);
void OLED_ReverseArea(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显示函数*/
void OLED_ShowChar(OLED_Handle *oled, int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(OLED_Handle *oled, int16_t X, int16_t Y, const char *String, uint8_t FontSize);
void OLED_ShowNum(OLED_Handle *oled, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(OLED_Handle *oled, int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(OLED_Handle *oled, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(OLED_Handle *oled, int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(OLED_Handle *oled, int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowChinese(OLED_Handle *oled, int16_t X, int16_t Y, const char *Chinese);
void OLED_ShowImage(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t FontSize, const char *format, ...);

/*绘图函数*/
void OLED_DrawPoint(OLED_Handle *oled, int16_t X, int16_t Y);
uint8_t OLED_GetPoint(OLED_Handle *oled, int16_t X, int16_t Y);
void OLED_DrawLine(OLED_Handle *oled, int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(OLED_Handle *oled, int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(OLED_Handle *oled, int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

/*********************函数声明*/

#ifdef __cplusplus
}
#endif

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/

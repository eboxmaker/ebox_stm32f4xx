#include "graphics.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stm32f4xx.h>

#include "RA8875.h"
#include "spi_flash.h"
//#define LCDBUFSIZE	800*480
#define LCDW 480
#define LCDH 272

extern void DelayMs(u32 nMs);
extern const struct __ASC_ZK sysEnzk[];
extern const struct __HZK_ZK sysHzzk[];
extern u8 TOUCHSTS;
u16 fontbuf[256];
struct 
{
u8 R;
u8 G;
u8 B;
u8 RT;
u8 GT;
u8 BT;
u8 RA;
u8 GA;
u8 BA;
u16 RStartPix;
u16 REndPix;
u16 GStartPix;
u16 GEndPix;
u16 BStartPix;
u16 BEndPix;
u16 BarStartPix;
u16 BarLenth;
}RGBGradient;

//////////////////////////////////////////////���ú���////////////////////////////////////////                      
//code �ַ�ָ�뿪ʼ
//���ֿ��в��ҳ���ģ�׵�ַ
//code �ַ����Ŀ�ʼ��ַ,ascii��
//mat  ���ݴ�ŵ�ַ size*2 bytes��С
//����ԭ��@HYW
//CHECK:09/10/30
/***************************************************************
GBK������ת��Ϊ��λ��

***************************************************************/
void GetGBKData(unsigned char *code,unsigned char *Data)
{
	unsigned char ch,cl;
	unsigned char i;					  
	unsigned long foffset; 
	ch=*code;
	cl=*(++code);
	if(ch<0x81||cl<0x40||cl==0xff||ch==0xff)
	{   		    
	  for(i=0;i<(16*2);i++)
			*Data++=0x00;
	  return; 
	}          
	if(cl<0x7f)cl-=0x40;//ע��!
	else cl-=0x41;
	ch-=0x81;   
	foffset = STARTADDR + ((unsigned long)190*ch+cl)*32;//�õ��ֿ��е��ֽ�ƫ����  		  
	SPI_Flash_Read(Data,foffset,32);
} 
/***************************************************************
GB2321������ת��Ϊ��λ��
***************************************************************/
void Get_HzMat1(unsigned char *code,unsigned char *Data)
{
	unsigned char ch,cl;
	unsigned long foffset; 
	ch=*code;
	cl=*(++code);
	foffset = STARTADDR + ((ch-0xa0-1)*94+(cl-0xa0-1))*32;
	SPI_Flash_Read(Data,foffset,32);

} 


/**************************************************************************************
�������ܣ��������LCD��Ļ
���룺    Ŀ����ɫ
�����	  ��
**************************************************************************************/
void LcdClear(u16 color)
{
	Text_color(color); // �趨��ɫ
	Geometric_Coordinate(0,479,0,271); // �趨��������
	WriteCommand(0x90);
	WriteData(0xB0);
	RA8875_WAITSTATUS();
}

/**************************************************************************************
�������ܣ�����
���룺    u16  x,y    	        �������
	  	  u16  color		��ɫ
�����	  ��


**************************************************************************************/
void LcdPrintDot(u16 x, u16 y, u16 color)
{
	XY_Coordinate(x,y);
	WriteCommand(0x02);//MRWC
	WriteData(color);
}
/**************************************************************************************
�������ܣ���LCD�ϴ�ӡһ���ַ�
���룺    ch       	�ַ�
          x,y	      ��ʾλ������
					color	    ������ɫ
					bcolor    ������ɫ
�����	  ��
**************************************************************************************/
void LcdPrintchar(char ch,u16 x,u16 y,u16 color,u16 bcolor,u8 mode)
{
	u16 px;
	
	px = x;
	Text_Mode();
	if(mode == 1)
		Font_with_BackgroundTransparency();
	else
		Font_with_BackgroundColor();
	//Horizontal_FontEnlarge_x2();
	//Vertical_FontEnlarge_x2();
	Text_Background_Color(bcolor);
	Text_Foreground_Color(color);
	Font_Coordinate(px,y);//����
	Text_Mode();
	WriteCommand(0x02);
	WriteData(ch);
	px += 8;
	Graphic_Mode();//�л���ͼ��ģʽ
}
/**************************************************************************************
�������ܣ���LCD�ϴ�ӡһ������
���룺    *font      ����ָ��
          x,y	       ��ʾλ������
					color	     ������ɫ
					bcolor     ������ɫ
					mode			 ģʽ��1-͸��0-��͸��
�����	  ��
��ע��		���ֿ��ǰ������ϵ��£�Ȼ������ҵ�˳��ȡģ
**************************************************************************************/
void LcdPrintHz(u8 *font,u16 x,u16 y,u16 color,u16 bcolor,u8 mode)
{
	u16 i,j,fcode;
	u8 dzk1[32];	
	
	GetGBKData(font,dzk1);//�õ���Ӧ��С�ĵ�������
	for(i=0;i<16;i++)
	{
		fcode = dzk1[i*2]<<8 | dzk1[(i*2+1)];
		Memory_Write_TopDown_LeftRight();
			XY_Coordinate(x+i,y);
		for(j=0;j<16;j++)//����һ�е�����
		{	
			WriteCommand(0x02);
			if(fcode&(0x8000>>j))
			{
				WriteData(color);
			}
			else
			{
					WriteData(bcolor);

				
			}
		}
	}
}

/**************************************************************************************
��������: 	����Ļ��ʾһ���ַ���
��    ��: 	char* str     �ַ���ָ��
            u16 x,y				Ŀ����ʾλ������
						u16 color			������ɫ
						u16 bcolor		������ɫ
						u8  mode    	ģʽ��1-͸��0-��͸��
��    ��: 	��
**************************************************************************************/
void LcdPrintStr(char *str,u16 x,u16 y,u16 color,u16 bcolor,u8 mode)
{
	u16 px;
	px = x;
	while(*str != 0)
	{
		if(*str > 0x7F)//�Ǻ���
		{
			if(px > 480 - 16)
			{
				px = 0;
				y += 16;
			}
			LcdPrintHz(str,px,y,color,bcolor,mode);
			str += 2;
			px += 16;
		}
		else//�Ǻ���
		{
			if(px > 480 - 12)
			{
				px = 0;
				y += 16;
			}
			LcdPrintchar(*str,px,y,color,bcolor,mode);
			str++;
			px += 8;
		}
		RA8875_WAITSTATUS();
	}
}

void LcdPrintStrMid(u8 *str,u8 x,u16 y,u8 pixlen)
{
	u16 strpix , movpix;
   	strpix = strlen(str)*12;
	if(strpix > pixlen)
		LcdPrintStr(str,x,y,0XF800,0x0,1);
	else
	{
		movpix = (pixlen-strpix)/2;
	  LcdPrintStr(str,movpix+x,y,0XF800,0x0,1);
	}
} 
/**************************************************************************************
��������: ����Ļ����ʽ��ʾһ���ַ���
��    ��: 

��    ��: ��
**************************************************************************************/
void LcdPrintf(u16 x,u16 y,u16 color,u16 bcolor,u8 mode,char *fmt,...)
{
    va_list ap;
    char str[64];

    va_start(ap,fmt);
    vsprintf(str,fmt,ap);
    LcdPrintStr(str,x,y,color,bcolor,mode);
    va_end(ap);
}
/**************************************************************************************
�������ܣ�	��ˮƽֱ��
����	��WORD  x    		���ĺ�����
	  		WORD  y  	    ����������
	  		WORD  width		ֱ�߳���
	  		WORD  color		��ɫ
���	��	��
**************************************************************************************/
void LcdGeometryHorz(u16 x, u16 y, u16 width, u16 color)
{
	Text_color(color); // �趨��ɫ
	Geometric_Coordinate(x,x+width-1,y,y); // ����ˮƽ����ʼ��
	WriteCommand(0x90);//д0x90�Ĵ���
	WriteData(0x80);   //��0x90�Ĵ���д����
	RA8875_WAITSTATUS();
}
/**************************************************************************************
�������ܣ������������ŵ���
**************************************************************************************/
void LcdGeometryHorzD(u16 x, u16 y, u16 width, u16 color)
{
		LcdGeometryHorz(x, y, width, color);
		LcdGeometryHorz(x, y+1, width, color);
}
/**************************************************************************************
�������ܣ�  ����ֱֱ��
����	��  u16  x    	    ���ĺ�����
	  		u16  y  	    ����������
	  		u16  height		ֱ�߸߶�
	  		u16  color		��ɫ
���	��	��
**************************************************************************************/
void LcdGeometryVertical(u16 x, u16 y, u16 height, u16 color)
{
	Text_color(color); //�趨��ɫ
	Geometric_Coordinate(x,x,y,y+height-1); // ������ʼ������
	WriteCommand(0x90);
	WriteData(0x80);
	RA8875_WAITSTATUS();
}
/**************************************************************************************
�������ܣ���һ��б��
���룺    u16  x1   ���ĺ�����
	  u16  y1  	    ����������
	  u16  x2		�����ĺ�����
	  u16  y2		������������
	  u16  color		��ɫ
�����	  ��
**************************************************************************************/
void LcdGeometryBIAS(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	Text_color(color); // �趨��ɫ
	Geometric_Coordinate(x1,x2,y1,y2); // ������ʼ������
	WriteCommand(0x90);
	WriteData(0x80);
	RA8875_WAITSTATUS();
}
/**************************************************************************************
�������ܣ�������
���룺    ���ε����ϽǺ����½����꣬��Ŀ����ɫ
		  x1,y1  ���Ͻ�����
		  x2,y2  ���Ͻ�����
		  fill	 0  �����ο�
		  		 1  �������
�����	  ��
**************************************************************************************/
void LcdGeometryFillRec(u16 x1, u16 y1, u16 x2, u16 y2, u8 fill, u16 color)
{
	Text_color(color); // �趨��ɫ
	Geometric_Coordinate(x1,x2,y1,y2); // �趨��������
	WriteCommand(0x90);
	if (fill)
	{
		WriteData(0xB0);
	}
	else
	{
		WriteData(0x90);
	}
	RA8875_WAITSTATUS();
}

/**************************************************************************************
�������ܣ����һ����������
���룺    ���ε����ϽǺ����½����꣬��Ŀ����ɫ
�����	  ��
**************************************************************************************/
/*void Lcd_cursorFillRec(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)//LcdG_FillRec
{
	RA8875_WAITSTATUS();
	Text_color(color); // �趨��ɫ
	Geometric_Coordinate(x1,x2,y1,y2-1); // �趨��������
	WriteCommand(0x90);
	WriteData(0xB0);
	RA8875_WAITSTATUS();
}*/


/**************************************************************************************
��������: ����Ļ��ʾһ 16bit bmpͼ��
��    ��: u16* image    	ͼ�����������ַ
          u16 x,y			���Ͻ�����
	  	  u16 width,height	ͼ�εĿ��Ⱥ͸߶�
��    ��: ��
**************************************************************************************/
void LcdPrint16bitBmp(u16* image,u16 x,u16 y,u16 widht,u16 height)
{
	u16 w,h;
Memory_Write_LeftRight_TopDown();
	for(h=0;h<height;h++)
	{
		XY_Coordinate(x,y+h);
		WriteCommand(0x02);		//MRWC  REG[02h] -- Memory Read/Write Command
		for(w = 0; w< widht; w++)
		{
			WriteDataHS(*image++);
		//	Delay(20);
		}
	}
}

void LcdPrint8bitBmp(const u8* image,u16 x,u16 y,u16 widht,u16 height)
{
	//u8  color8;
	u16 w,h;
	//u16 r,g,b;
    
	for(h=0;h<height;h++)
	{
		XY_Coordinate(x,y+h);
		WriteCommand(0x02);		//MRWC  REG[02h] -- Memory Read/Write Command
		for(w = 0; w< widht; w++)
		{
			WriteData(*image++);
			
			//color8 = *image++;
			/*
			0123456700000000 
			67000 345000 01200
            0011 1000 1110 0011
	
			*/
			//r = (color8 & 0xE0)<<13; 
			//g = (color8 & 0x1C)<<6;
    		//b = (color8 & 0x03)<<3;
            
            //r = ((color8 & 0xE0)<<13)|0x1800; 
    		//g = ((color8 & 0x1C)<<6) |0xE0;
    		//b = ((color8 & 0x03)<<3) |0x07;
    		
            //r = (color8 & 0x03)<<14;
            //g = (color8 & 0x1C)<<6;
    		//b = (color8 & 0xE0)>>11; 
			
            //WriteData(r|g|b|0x38E3);
		}
	}
}
////////////////////////////////////////////////////////////////
void paint(u16 ColorBegin,u16 ColorEnd)
{
u8 r0 = (ColorBegin >> 11) & 0x1f;
u8 r1 = (ColorEnd >> 11) & 0x1f;
u8 g0 = (ColorBegin >> 5) & 0x1f;
u8 g1 = (ColorEnd >> 5) & 0x1f;
u8 b0 = ColorBegin & 0x1f;
u8 b1 = ColorEnd & 0x1f;
u16 i,F,rr,gg,bb,rgb;
for (i = 0; i < 400; i++)
{
F = (i << 16) / 400;
rr = r0 + ((F * (r1 - r0)) >> 16);
gg = g0 + ((F * (g1 - g0)) >> 16);
bb = b0 + ((F * (b1 - b0)) >> 16);
rgb = (rr << 11 | gg << 5 | bb);

LcdGeometryFillRec(i*1,0,(i+1)*1,49,1,rgb);

}
}
 

void BarGradient(void)
{
	u16 rr,gg,bb,i,rgb1,RStep,GStep,BStep,RLen,GLen,BLen,CurrentPix;
float RVarPerStep,GVarPerStep,BVarPerStep;

	RGBGradient.R = 0;
	RGBGradient.G = 0;
	RGBGradient.B = 0;	
	RGBGradient.RT = 1;
	RGBGradient.GT = 1;
	RGBGradient.BT = 0;	
	RGBGradient.RA = 1;
	RGBGradient.GA = 1;
	RGBGradient.BA = 1;
	RGBGradient.RStartPix = 0;
	RGBGradient.REndPix = 299;
	RGBGradient.GStartPix = 0;
	RGBGradient.GEndPix = 299;
	RGBGradient.BStartPix = 0;
	RGBGradient.BEndPix = 299;
	RGBGradient.BarLenth = 300;
	RGBGradient.BarStartPix=0;
	
	RStep = RGBGradient.REndPix - RGBGradient.RStartPix+1;
	RVarPerStep = (63-RGBGradient.R)/(RStep*1.0);
	
	GStep = RGBGradient.GEndPix - RGBGradient.GStartPix+1;
	GVarPerStep = (31-RGBGradient.G)/(GStep*1.0);
	
	BStep = RGBGradient.BEndPix - RGBGradient.BStartPix+1;
	BVarPerStep = (31-RGBGradient.B)/(BStep*1.0);
	
	for(i=0;i<RGBGradient.BarLenth;i++)
	{
		CurrentPix = RGBGradient.BarStartPix+i;
		if((CurrentPix>=RGBGradient.RStartPix)&&(CurrentPix<=RGBGradient.REndPix))
			if(RGBGradient.RT == 0)
				rr = RGBGradient.R + Myround((CurrentPix - RGBGradient.RStartPix) * RVarPerStep);
			else
				rr = 63- (RGBGradient.R + Myround((CurrentPix - RGBGradient.RStartPix) * RVarPerStep));
		else
			rr=0;
		
		
		if((CurrentPix>=RGBGradient.GStartPix)&&(CurrentPix<=RGBGradient.GEndPix))
		{
			if(RGBGradient.GT == 0)
				gg =  RGBGradient.R + Myround((CurrentPix - RGBGradient.GStartPix) * GVarPerStep);
			else
				gg =31-(RGBGradient.R + Myround((CurrentPix - RGBGradient.GStartPix) * GVarPerStep));
		}
		else
			gg=0;
		
		if(RGBGradient.BA == 0)
			bb=0;
		else
		{
			if((CurrentPix>=RGBGradient.BStartPix)&&(CurrentPix<=RGBGradient.BEndPix))
				bb = RGBGradient.B + Myround((CurrentPix - RGBGradient.BStartPix) * BVarPerStep);
			else
				bb=0;
		}
		
		
		rgb1 = (rr<<10)+(gg<<5)+(bb<<0);
		
		LcdGeometryFillRec(RGBGradient.BarStartPix+i*1,0,RGBGradient.BarStartPix+(i+1)*1,49,1,(rr<<10));
		LcdGeometryFillRec(RGBGradient.BarStartPix+i*1,50,RGBGradient.BarStartPix+(i+1)*1,99,1,(gg<<5));
		LcdGeometryFillRec(RGBGradient.BarStartPix+i*1,100,RGBGradient.BarStartPix+(i+1)*1,149,1,(bb<<0));
		LcdGeometryFillRec(RGBGradient.BarStartPix+i*1,150,RGBGradient.BarStartPix+(i+1)*1,199,1,rgb1);
	}

}
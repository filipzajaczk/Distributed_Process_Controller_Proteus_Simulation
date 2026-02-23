#include <REGX52.H>
#include <string.h>
#include <stdio.h>

//adresy urzadzen
#define KLAWIATURA	0xF0
#define LCD			0xE0
#define BCD    	0xD0
#define SILNIK  0xB0

//komendy
#define OBROTY  0x01
#define MOC  0x02
#define KIERUNEK      0x03
#define GRZANIE_L1    0x04
#define GRZANIE_L2    0x05
#define START         0x06
#define MENU 					0x07
#define POKAZ_MENU 		0x08

volatile bit odebrano_nowy_pakiet = 0;
volatile unsigned char odebrana_komenda = 0;
volatile unsigned char odebrana_dana = 0;

extern void LcdInit();
extern void LcdWelcome();
extern void Lcd_Cursor (char row, char column);
extern void Lcd_DisplayCharacter (char a_char);
extern void Lcd_Clear(void); 
extern void Lcd_Print(char *string);
	
extern void Lcd_DisplayString (char row, char column, char *string);
extern void Lcd_DefineChar(unsigned char code_pos, unsigned char *pattern);

sbit DE = P3^4;
unsigned char data Var1, Var2, Var3;
volatile unsigned char data Kod;
volatile bit updateScreen = 0;


void SerialInit(void)
{
	TMOD = 0x20;
	TH1 = 0xFD;   
	TL1 = 0xFD;
	SCON = 0x50;
	TR1 = 1;
	ES = 1;
	EA = 1;
}

void odebranie() interrupt 4
{
	static unsigned char czekam = 0;
	static unsigned char temp = 0;
	unsigned char bajt;
	
	if(RI)
	{
		bajt = SBUF;
		RI = 0;
		
		if (czekam == 0) 
		{
			if ((bajt & 0xF0) == LCD)
			{
				temp = bajt & 0x0F;
				czekam = 1;
			}
		}
		else
		{
			odebrana_komenda = temp;
			odebrana_dana = bajt;
			odebrano_nowy_pakiet = 1;
			czekam = 0;
		}
	}
		
}

void refresh(unsigned char komenda, unsigned char val)
{
	char bufor[16];
	Lcd_Clear();
	
	switch(komenda)
	{
		case POKAZ_MENU:
			Lcd_Cursor(1,1);
			Lcd_Print("1.Turns");
			Lcd_Cursor(1,10);
			Lcd_Print("2.Power");
			Lcd_Cursor(2,1);
			Lcd_Print("3.Directory");
			Lcd_Cursor(3,1);
			Lcd_Print("4.Lamp1");
			Lcd_Cursor(3,9);
			Lcd_Print("5.Lamp2");
			Lcd_Cursor(4,1);
			Lcd_Print("6.Start");
		
		break;
		
		case OBROTY:
			Lcd_Cursor(1,1);
			Lcd_Print("Set Turn Count");
			Lcd_Cursor(2,1);
			sprintf(bufor, "Turns: %d    ", (int)val);
			Lcd_Print(bufor);
			Lcd_Cursor(4,1);
			Lcd_Print("* - SAVE");
		break;
		
		case MOC:
			Lcd_Cursor(1,1);
			Lcd_Print("Set power");
			Lcd_Cursor(2,1);
			sprintf(bufor, "Power: %d    ", (int)val);
			Lcd_Print(bufor);
			Lcd_Cursor(3,1);
			Lcd_Print("# - 100%");
			Lcd_Cursor(4,1);
			Lcd_Print("* - SAVE");
		break;
		
		case KIERUNEK:
			Lcd_Cursor(1,1);
			Lcd_Print("Set Directory");
			Lcd_Cursor(2,1);
			Lcd_Print("1 - LEFT");
			Lcd_Cursor(3,1);
			Lcd_Print("2 - RIGHT");
		break;
		
		case GRZANIE_L1:
			Lcd_Cursor(1,1);
			Lcd_Print("Set Lamp 1 Power");
			Lcd_Cursor(2,1);
			sprintf(bufor, "Power: %d    ", (int)val);
			Lcd_Print(bufor);
			Lcd_Cursor(3,1);
			Lcd_Print("# - 100%");
			Lcd_Cursor(4,1);
			Lcd_Print("* - SAVE");
		break;
		
		case GRZANIE_L2:
			Lcd_Cursor(1,1);
			Lcd_Print("Set Lamp 2 Power");
			Lcd_Cursor(2,1);
			sprintf(bufor, "Power: %d    ", (int)val);
			Lcd_Print(bufor);
			Lcd_Cursor(3,1);
			Lcd_Print("# - 100%");
			Lcd_Cursor(4,1);
			Lcd_Print("* - SAVE");
		break;
		
		case START:
			Lcd_Cursor(1,1);
			Lcd_Print("Motor running..");			
			Lcd_Cursor(2,1);
			Lcd_Print("Please wait..");
		break;
	}
}

void main(void)
{
	P3_4 = 0;
	LcdInit();
	SerialInit();
	
	refresh(POKAZ_MENU, 7);

	while(1) {
		if (odebrano_nowy_pakiet) 
		{
			refresh(odebrana_komenda, odebrana_dana);
			odebrano_nowy_pakiet = 0;
		}
	}
}
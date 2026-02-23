#include <REGX51.H>

unsigned char code kody[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
unsigned char xdata lewy_disp  _at_ 0xFE00;
unsigned char xdata prawy_disp _at_ 0xFD00;
volatile unsigned char j = 0;
volatile unsigned char i = 0;


volatile bit odebrano_nowy_pakiet = 0;
volatile unsigned char odebrana_komenda = 0;
volatile unsigned char odebrana_dana = 0;

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
			if ((bajt & 0xF0) == BCD)
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

void wyswietl_bcd() interrupt 1
{
		TH0 = 0xFF;
		TL0 = 0x8D;
    lewy_disp  = kody[(odebrana_dana / 10) % 10];
    prawy_disp = kody[odebrana_dana % 10];
}

void init
	(void)
{
	TMOD = 0x21; 
	TH1 = 0xFD;
	TL1 = 0xFD;
	SCON = 0x50;
	TR1 = 1;
	TH0 = 0xFF;
	TL0 = 0x8D;
	ET0 = 1;
	TR0 = 1;
	ES = 1; 
	EA = 1; 
}


void main(void){
	
	P3_4 = 0;
	P1 = 0x00;
	init();

	lewy_disp  = 0x00;
	prawy_disp = 0x00;

	while (1)
	{
		if (odebrano_nowy_pakiet)
		{
			odebrano_nowy_pakiet = 0;
		}
	}
	
}
	
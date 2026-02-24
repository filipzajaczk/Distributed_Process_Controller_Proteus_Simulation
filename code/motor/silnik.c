#include <REGX52.H>

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

volatile unsigned char obroty_licznik = 0;
volatile unsigned char licznik = 0;
volatile unsigned char lampa1 = 0;
volatile unsigned char lampa2 = 0;
volatile unsigned char moc_silnika = 50;

volatile bit odebrano_nowy_pakiet = 0;
volatile unsigned char odebrana_komenda = 0;
volatile unsigned char odebrana_dana = 0;

volatile bit aktualizuj_bcd = 0;
volatile bit koniec = 0;
bit lewo = 1;
bit prawo = 0;

void Send(unsigned char Value){
	ES = 0;
	P3_4 = 1;
	TI = 0;	
	SBUF = Value;
	while(TI==0){;}
	TI = 0;
	P3_4 = 0;
	ES = 1;
}

void wyslij_pakiet(unsigned char adresat, unsigned char komenda, unsigned int dana) {
	unsigned char pakiet1;
	pakiet1 = adresat | komenda;
	Send(pakiet1);
	Send(dana);
}
//zainicjowany jest licznik który tyka sobie w tle ale ograniczamy go do modulo 10. Moc okreslamy jako ulamek
//np. na 3 z 10 tykniec ma byc dostarczany prad do zarówy, w taki sposób dostarczamy 30% mocy zarowki
void zarowa() interrupt 1
{
	
	TH0 = 0xFF;
	TL0 = 0x00;
	
	licznik++;
	if(licznik >= 100) licznik = 0;
	
	if(licznik < lampa1) P2_2 =1;
	else P2_2 = 0;
	
	if(licznik < lampa2) P2_1 =1;
	else P2_1 = 0;
	
	if(prawo && (licznik < moc_silnika)) P2_5 =1;
	else P2_5 = 0;
	
	if(lewo && (licznik < moc_silnika)) P2_6 =1;
	else P2_6 = 0;
}

//przy kazdym impulsie na port P3.2 (INT0) wykonuje sie ta funkcja zliczania obrotów
void licz_obroty(void) interrupt 0 
{
	obroty_licznik --;
	if (obroty_licznik >= 0)
	{
		aktualizuj_bcd = 1;
	}
	
	if (obroty_licznik == 0)
	{
		P2_3 = 0;
		P2_4 = 0;
	}
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
			if ((bajt & 0xF0) == SILNIK)
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


void init()
{
    TMOD = 0x21;
    TH1 = 0xFD;
    TL1 = 0xFD;
    SCON = 0x50;
    TR1 = 1;
    TH0 = 0xFF;
    TL0 = 0x9C;
    ET0 = 1;
    TR0 = 1;

    IT0 = 1;
    EX0 = 1;
    
    ES = 1;
    EA = 1;
}

void main(void)
{
	init();
	
	P3_4 = 0;
	P2 = 0;

	P2_3 = 0;
	P2_4 = 0;
	
	
	while (1)
	{
		if (odebrano_nowy_pakiet)
		{
			switch(odebrana_komenda)
			{
				case OBROTY:
					obroty_licznik = odebrana_dana;
				break;
				case MOC:
					moc_silnika = odebrana_dana;
				break;
				case KIERUNEK:
					if(odebrana_dana == 1)
					{
						lewo = 1;
						prawo = 0;
					}
					else{
						lewo = 0;
						prawo = 1;
					}
				break;
				case GRZANIE_L1:
					lampa2 = odebrana_dana;
				break;				
				case GRZANIE_L2:
					lampa1 = odebrana_dana;
				break;				
				case START:
					P2_3 = 1;
					P2_4 = 1;
				break;	
			}
			odebrano_nowy_pakiet = 0;
		}
		
		if(aktualizuj_bcd) 
		{
			wyslij_pakiet(BCD, OBROTY, obroty_licznik);
			aktualizuj_bcd = 0;
		}
	}
	
}


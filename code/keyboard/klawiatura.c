#include <REGX52.H>

unsigned int jednosc = 0;
unsigned int dziesiatka = 0;
unsigned int liczba = 0;
unsigned char klawisz;
char cyfra;

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


void delay(unsigned int t) {
    unsigned int i, j;
    for(i=0; i<t; i++)
        for(j=0; j<100; j++);
}


void Send(unsigned char Value){
	P3_4 = 1;
	TI = 0;	
	SBUF = Value;
	while(TI==0){;}
	TI = 0;
	P3_4 = 0;
}

void wyslij_pakiet(unsigned char adresat, unsigned char komenda, unsigned int dana) {
	unsigned char pakiet1;
	pakiet1 = adresat | komenda;
	Send(pakiet1);
	Send(dana);
}



unsigned char code scan[] = {0xF7, 0xFD, 0xFB};

unsigned char pobierz_klawisz(void) {
	unsigned char wcisniety_kod = 0;
	unsigned char odczyt = 0;
	unsigned int i = 0;

	while(1) 
	{
		for(i = 0; i < 3; i++) 
		{
			P2 = scan[i];
			odczyt = P2; 

			if (odczyt != scan[i])
			{
				wcisniety_kod = odczyt;
				
				do {
					P2 = scan[i]; 
				} while (P2 != scan[i]);

				return wcisniety_kod;
			}
		}
	}
}

char zdekoduj_cyfre(unsigned char kod) {
	switch(kod) 
	{
		case 0xE7: return 1;
		case 0xD7: return 4;
		case 0xB7: return 7;
		case 0x77: return 10; // *
		case 0xEB: return 2;
		case 0xDB: return 5;
		case 0xBB: return 8;
		case 0x7B: return 0;
		case 0xED: return 3;
		case 0xDD: return 6;
		case 0xBD: return 9;
		case 0x7D: return 11; // #
		default:   return -1;
	}
}

void init()
{
	P2 = 1;
	P3_4 = 0;
	TMOD = 0x20;
	TH1 = 0xFD;
	TL1 = 0xFD;
	SCON = 0x50;
	TR1 = 1;
	EA = 1;
	ES = 1;
}

void main(void) {
	init();

	while(1) 
	{
		wyslij_pakiet(LCD, POKAZ_MENU, 7);
		klawisz = pobierz_klawisz(); 
		cyfra = zdekoduj_cyfre(klawisz);
		jednosc = 0;
		dziesiatka = 0;
		switch(cyfra)
		{
			case 1: //ustawienie ile obrotow ma zrobic silnik
				liczba = 1;
				wyslij_pakiet(LCD, OBROTY, liczba);
				while(1)
				{
					klawisz = pobierz_klawisz(); 
					cyfra = zdekoduj_cyfre(klawisz);
					if(cyfra == 10) //zakceptowanie
					{
						wyslij_pakiet(LCD, POKAZ_MENU, 7);							
						break;
					}
					else
					{
						dziesiatka = jednosc;
						jednosc = cyfra;
						liczba = dziesiatka * 10 + jednosc;
						wyslij_pakiet(LCD, OBROTY, liczba);
						delay(5);
						wyslij_pakiet(BCD, OBROTY, liczba);
						delay(5);
						wyslij_pakiet(SILNIK, OBROTY, liczba);
					}
				}
			break;
			case 2: //ustawienie mocy w % pracy silnika
				liczba = 1;
				wyslij_pakiet(LCD, MOC, liczba);
				//wyslij silnikowi
				while(1)
				{
					klawisz = pobierz_klawisz(); 
					cyfra = zdekoduj_cyfre(klawisz);
					if(cyfra == 10) //zakceptowanie
					{
						if(liczba == 0) 
						{
							wyslij_pakiet(SILNIK, MOC, 50);
						}
						wyslij_pakiet(LCD, POKAZ_MENU, 7);
						break;
					}
					else if (cyfra == 11) //ustawienie 100%
					{
						wyslij_pakiet(LCD, MOC, 100);
						wyslij_pakiet(SILNIK, MOC, 100);
					}
					else
					{
						dziesiatka = jednosc;
						jednosc = cyfra;
						liczba = dziesiatka * 10 + jednosc;
						wyslij_pakiet(LCD, MOC, liczba);
						delay(5);
						wyslij_pakiet(SILNIK, MOC, liczba);
					}
				}
			break;
			case 3: //w ktora strone silniczek ma sie krecic
				wyslij_pakiet(LCD, KIERUNEK, 1);
				while(1)
				{
					klawisz = pobierz_klawisz(); 
					cyfra = zdekoduj_cyfre(klawisz);
					if (cyfra == 1)
					{
						//wyslij_pakiet(LCD, KIERUNEK, 1);
						wyslij_pakiet(LCD, POKAZ_MENU, 7);
						delay(5);
						wyslij_pakiet(SILNIK, KIERUNEK, 1);
						break;
					}
					else if (cyfra == 2)
					{
						//wyslij_pakiet(LCD, KIERUNEK, 2);
						wyslij_pakiet(LCD, POKAZ_MENU, 7);
						delay(5);
						wyslij_pakiet(SILNIK, KIERUNEK, 2);
						break;
					}
				}				
			break;
			case 4: //ustawienie grzania pierwszej lampy
				liczba = 0;
				wyslij_pakiet(LCD, GRZANIE_L1, liczba);
				//wyslij lampie
				while(1)
				{
					klawisz = pobierz_klawisz(); 
					cyfra = zdekoduj_cyfre(klawisz);
					if(cyfra == 10) //zakceptowanie
					{
						wyslij_pakiet(LCD, POKAZ_MENU, 7);
						break;
					}
					else if (cyfra == 11) //ustawienie 100%
					{
						wyslij_pakiet(LCD, GRZANIE_L1, 100);
						wyslij_pakiet(SILNIK, GRZANIE_L1, 100);
					}
					else
					{
						dziesiatka = jednosc;
						jednosc = cyfra;
						liczba = dziesiatka * 10 + jednosc;
						wyslij_pakiet(LCD, GRZANIE_L1, liczba);
						delay(5);
						wyslij_pakiet(SILNIK, GRZANIE_L1, liczba);
					}
				}
			break;
			case 5: //ustawienie grzania drugiej lampy
				liczba = 0;
				wyslij_pakiet(LCD, GRZANIE_L2, liczba);
				//wyslij lampie
				while(1)
				{
					klawisz = pobierz_klawisz(); 
					cyfra = zdekoduj_cyfre(klawisz);
					if(cyfra == 10) //zakceptowanie
					{
						wyslij_pakiet(LCD, POKAZ_MENU, 7);
						break;
					}
					else if (cyfra == 11) //ustawienie 100%
					{
						wyslij_pakiet(LCD, GRZANIE_L2, 100);
						wyslij_pakiet(SILNIK, GRZANIE_L2, 100);
					}
					else
					{
						dziesiatka = jednosc;
						jednosc = cyfra;
						liczba = dziesiatka * 10 + jednosc;
						wyslij_pakiet(LCD, GRZANIE_L2, liczba);
						delay(5);
						wyslij_pakiet(SILNIK, GRZANIE_L2, liczba);
					}
				}
			break;
			case 6:
				wyslij_pakiet(LCD, START, 0);
				wyslij_pakiet(SILNIK, START, 0);
			break;
		}
	}
}
//************************************************************************//
// Złomek - kumpel Heńka, projekt pogladowy obsługi DDS AD9850, 
// wyświetlacza nokii 5110 i jakiegoś enkodera.
// Projekt otwarty http://sp-hm.pl
// SQ9MDD -  początkowy szkielet programu v 1.0.0
// S_____ - 
// S_____ - 
//
//************************************************************************//
/* CHANGELOG (nowe na górze)
 2014.10.12 - początek projektu wspólnego na sp-hm.pl
 wymiana biblioteki wyświetlacza lcd na LCDD5110 basic
 2014.05.22 - pierwsza wersja kodu warsztaty arduino w komorowie.  
 */
//************************************************************************//
//podłączamy bibliotekę syntezera
#include <AH_AD9850.h>

//podłączamy bibliotekę enkodera
#include <RotaryEncoder.h>;

//podłączamy bibliotekę do obsługi wyświetlacza
#include <LCD5110_Basic.h> 

//inicjalizujemy komunikację z syntezerem
//syntezer   - arduino
//CLK        - PIN 8
//FQUP       - PIN 9
//BitData    - PIN 10
//RESET      - PIN 11
AH_AD9850 AD9850(8, 9, 10, 11);

// inicjalizujemy wyświetlacz
// lcd    - arduino
// sclk   - PIN 7
// sdin   - PIN 6
// dc     - PIN 5
// reset  - PIN 3
// sce    - PIN 4
LCD5110 lcd(7,6,5,3,4); 
extern uint8_t SmallFont[]; //czcionka z biblioteki
extern uint8_t MediumNumbers[];//czcionka z biblioteki

//inicjalizujemy enkoder
//AO - w lewo
//A1 - w prawo
//nalezy pamiętać o kondensatorach (100nF) pomiędzy liniami encodera a masą
RotaryEncoder encoder(A0,A1,5,6,1000);

//kontrast wyświetlacza
const int kontrast = 70; 
const int pulses_for_groove = 2;

//zmienna pomocnicza do wyrzucania danych na lcd
char buffor[] = "              ";

//zmienna dla częstotliwości, wstawiamy tam częstotliwość od której startujemy
long czestotliwosc = 28500000;

//zmienna pomocnicza do liczenia impulsów z enkodera
int enc_sum = 0;

//funkcja do obsługi wyświetlania zmiany częstotliwości
void show_frequency(){
  lcd.setFont(SmallFont);                    //ustawiamy czcionkę
  sprintf(buffor,"%lu",czestotliwosc);       //konwersja danych do wyświetlenia (ładujemy longa do stringa
  lcd.print(buffor,CENTER,0);                //wyświetlamy dane na lcd 
}

// setup funkcja odpalana przy starcie
void setup(){
  //uruchamiam port szeregowy w celach diagnostycznych
  Serial.begin(9600);
  //odpalamy syntezer i ustawiamy częstotliwość startową
  AD9850.set_frequency(0,0,czestotliwosc);    //set power=UP, phase=0, 1MHz frequency
  delay(1000);                                //sekunda opóźnienia 
  lcd.InitLCD(kontrast);                      //odpalamy lcd ustawiamy kontrast
  show_frequency();                           //pokazmy cos na lcd
} 

void loop(){
  //czytamy wartość z encodera
  int enc = encoder.readEncoder(); 
  if(enc != 0) {                          //jeśli wartość jest inna niż zero sumujemy
    enc_sum = enc_sum + enc;              //jeden ząbek encodera to +2 lub -2 tutaj to zliczam
    Serial.println(enc);
  } 
  //jesli zaliczyliśmy ząbek dodajemy lub odejmujemy do częstotliwości wartość kroku (na razie na sztywno 100Hz)
  if(enc_sum >= 2){
    czestotliwosc = czestotliwosc + 100;  //docelowo czestotliwosc = czestotliwosc + krok

    AD9850.set_frequency(czestotliwosc);  //ustawiam syntezę na odpowiedniej częstotliwości
    show_frequency();                     //drukuję częstotliwość na wyświetlaczu za pomocą gotowej funkcji
    enc_sum = 0;                          //reset zmiennej zliczającej impulsy enkodera
  }
  if(enc_sum <= -2){
    czestotliwosc = czestotliwosc - 100;  //docelowo czestotliwosc = czestotliwosc - krok      
    AD9850.set_frequency(czestotliwosc);  //ustawiam syntezę na odpowiedniej częstotliwości
    show_frequency();                     //drukuję częstotliwość na wyświetlaczu za pomocą gotowej funkcji       
    enc_sum = 0;                          //reset zmiennej zliczającej impulsy enkodera
  }   
  delayMicroseconds(5);                    //małe opóźnienie dla prawidłowego działania enkodera
}


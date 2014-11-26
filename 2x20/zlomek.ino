//************************************************************************//
// Złomek - kumpel Heńka, DDS AD9850, 
// wyświetlacz LCD 2x20, enkoder.
// Projekt otwarty http://sp-hm.pl/thread-2164.html
// SQ9MDD -  początkowy szkielet programu v 1.0.0 

//************************************************************************//
/* CHANGELOG (nowe na górze)
 2014.11.26 - v.1.1.5 Krok syntezy 50Hz/1kHz/10kHz wyczyszczony kod prawidłowe wyświetlanie na LCD
 reklama na starcie syntezera
 2014.11.24 - v.1.1.4 Osobna gałąź przeznaczona dla klasycznego wyświetlacza 2x20
 2014.10.19 - v.1.0.4 wprowadzamy pośrednią kilka zmiennych do sekcji konfiguracji
 drobne czyszczenie kodu, poprawki w komentarzach
 2014.10.15 - v.1.0.3 limit czestotliwości górnej i dolnej
 przeniesienie wysyłania częstotliwości do DDS do osobnej funkcji
 2014.10.14 - v.1.0.2 zmiana kroku syntezy
 2014.10.12 - początek projektu wspólnego na sp-hm.pl
 wymiana biblioteki wyświetlacza lcd na LCDD5110 basic
 2014.05.22 - pierwsza wersja kodu warsztaty arduino w komorowie.  
 */
//************************************************************************//
#define software_version "1.1.5"

//podłączamy bibliotekę syntezera
#include <AH_AD9850.h>

//podłączamy bibliotekę enkodera
#include <RotaryEncoder.h>;

//podłączamy bibliotekę do obsługi wyświetlacza
#include <LiquidCrystal.h> 

//inicjalizujemy komunikację z syntezerem
//syntezer   - arduino
//CLK        - PIN 8
//FQUP       - PIN 9
//BitData    - PIN 10
//RESET      - PIN 11
AH_AD9850 AD9850(8, 9, 10, 11);

// inicjalizujemy wyświetlacz
// lcd      - arduino
// RS       - PIN 7
// ENABLE   - PIN 6
// D1       - PIN 5
// D2       - PIN 4
// D3       - PIN 3
// D4       - PIN 2
//LCD R/W do masy
LiquidCrystal lcd(7,6,5,4,3,2); 

//inicjalizujemy enkoder
//AO - w lewo
//A1 - w prawo
//nalezy pamiętać o kondensatorach (100nF) pomiędzy liniami encodera a masą
RotaryEncoder encoder(A0,A1,5,6,1000);  

//*****************************************************************************************************************************
//zmienne do modyfikacji każdy ustawia to co potrzebuje
const int pulses_for_groove = 2;                         //ilość impulsów na ząbek enkodera zmienić w zależności od posiadanego egzemplarza
const int step_input = A2;                               //wejście do podłączenia przełącznika zmiany kroku
const long low_frequency_limit = 3500000;                //dolny limit częstotliwości
const long high_frequency_limit = 3800000;               //górny limit częstotliwości
const long start_frequency = 3715000;                    //częstotliwość startowa
const long posrednia = -8000000;                         //częstotliwość pośredniej, każdy dobiera swoją w zależności od konstrukcji radia
//*****************************************************************************************************************************

//zmienne wewnętrzne, 
//jeśli nie trzeba proszę nie modyfikować
char buffor[] = "              ";                        //zmienna pomocnicza do wyrzucania danych na lcd
long frequency = start_frequency;                        //zmienna dla częstotliwości, wstawiamy tam częstotliwość od której startujemy
long step_value = 50;                                    //domyślny krok syntezy
int enc_sum = 0;                                         //zmienna pomocnicza do liczenia impulsów z enkodera

//funkcja do obsługi wyświetlania zmiany częstotliwości
void show_frequency(){
  long mhz = frequency / 1000000;                        //wyciągam czyste MHz
  long khz = frequency % 1000000;                        //wyciągam czyste kHz w dwóch krokach
  khz = khz / 1000;                                      //kHz obcinam z Hz
  long hz = frequency % 1000;                            //wyciągam Hz
  hz = hz / 10;                                          //obcinam by wyświetlić tylko dziesiątki Hz
  sprintf(buffor,"%1lu.%03lu.%02lu",mhz,khz,hz);         //konwersja danych do wyświetlenia (ładujemy częstotliwość, kropki itd do stringa i na ekran)
  lcd.setCursor(6,1);                                    //ustawianie kursora kolumna,wiersz
  lcd.print(buffor);                                     //wyświetlamy dane na lcd 
}

//funkcja do wyświetlania aktualnego kroku syntezera
void show_step(){
  switch(step_value){
     case 50:
       sprintf(buffor," 50Hz",step_value);         //bufor dla kroku 50Hz
     break; 
     case 1000:
       sprintf(buffor," 1kHz",step_value);         //bufor dla kroku 1kHz
     break;
     case 10000:
       sprintf(buffor,"10kHz",step_value);         //bufor dla kroku 10kHz
     break;
  }
  lcd.setCursor(15,0);                             //ustawiam kursor w 15-tym rzędzie i 1-szym wierszu
  lcd.print(buffor);                               //wyświetlam dane na lcd (8 oznacza drugi rząd) 
}

//funkcja zmieniajaca częstotliwość DDS-a
void set_frequency(int plus_or_minus){
  if(plus_or_minus == 1){                                                        //jeśli na plus to dodajemy
    frequency = frequency + step_value;                                          //częstotliwość = częstotliwość + krok    
  }  
  else if(plus_or_minus == -1){                                                  //jeśli na minus to odejmujemy
    frequency = frequency - step_value;                                          //częstotliwość = częstotliwość - krok    
  }
    frequency = constrain(frequency,low_frequency_limit,high_frequency_limit);   //limitowanie zmiennej 
    long frequency_to_dds = abs(posrednia + frequency);                          //a tutaj obliczam częstotliwość wynikową 
    AD9850.set_frequency(frequency_to_dds);                                      //ustawiam syntezę na odpowiedniej częstotliwości  
    Serial.println(frequency_to_dds);
}

// setup funkcja odpalana przy starcie
void setup(){  
  Serial.begin(9600);                         //uruchamiam port szeregowy w celach diagnostycznych       
  lcd.begin(20, 2);                           //inicjalizacja wyświetlacza 20 rzędów w 2 wierszach
  pinMode(step_input,INPUT_PULLUP);           //inicjalizujemy wejście zmiany kroku i podciągamy je do plusa
  set_frequency(0);                           //odpalamy syntezer i ustawiamy częstotliwość startową
  lcd.setCursor(0,0);                         //ustawiam kursor pierwszys rząd pierwszy wiersz
  lcd.print("Zlomek DDS");                    //reklama produktu
  lcd.setCursor(0,1);                         //ustawiam kursoe pierwszy rząd drugi wiersz
  lcd.print("v.");                            //v jak wersja
  lcd.print(software_version);                //i wyświetlam wersję
  
  delay(3000);                                //opóźnienie 
  lcd.clear();                                //czyścimy wyświetlacz
  show_frequency();                           //pokazujemy częstotliwość na lcd
  show_step();                                //pokazujemy krok syntezy
} 

void loop(){  
  int enc = encoder.readEncoder();        //czytamy wartość z encodera
  if(enc != 0) {                          //jeśli wartość jest inna niż zero sumujemy
    enc_sum = enc_sum + enc;              //jeden ząbek encodera to +2 lub -2 tutaj to zliczam
    Serial.println(enc);                  //wyrzucamy na port rs-232 wartość licznika enkodera (debugowanie)
  } 
  
  //obsługa klawisza zmiany kroku
  if(digitalRead(step_input) == LOW){     //sprawdzanie czy przycisk jest wcisnięty
   delay(100);                            //odczekajmy 100msec
    if(digitalRead(step_input) == LOW){   //jeśli klawisz nadal jest wcisnięty (czyli nie są to zakłócenia)
       switch(step_value){                //za pomocą instrukcji swich zmieniamy krok
          case 10000:                     //jeśli krok jest 10kHz ustaw 100kHz
            step_value = 1000;
          break;
          case 1000:                      //jeśli krok jest 1kHz ustaw 10kHz
            step_value = 50;
          break;
          case 50:                       //jeśli krok jest 100Hz ustaw 1kHz
            step_value = 10000;
          break;
       }
    }
    show_step();                          //pokazuję zmianę kroku na lcd
    delay(300);                           //zwłoka po zmianie kroku 300msec
  }
  
  //jesli zaliczyliśmy ząbek dodajemy lub odejmujemy do częstotliwości wartość kroku (na razie na sztywno 100Hz)
  if(enc_sum >= pulses_for_groove){
    set_frequency(1);                     //wywołuję funkcje zmiany częstotliwości z parametrem +
    show_frequency();                     //drukuję częstotliwość na wyświetlaczu za pomocą gotowej funkcji
    enc_sum = 0;                          //reset zmiennej zliczającej impulsy enkodera
  }
  if(enc_sum <= -(pulses_for_groove)){
    set_frequency(-1);                    //wywołuję funkcje zmiany częstotliwości z parametrem -
    show_frequency();                     //drukuję częstotliwość na wyświetlaczu za pomocą gotowej funkcji       
    enc_sum = 0;                          //reset zmiennej zliczającej impulsy enkodera
  }   
  delayMicroseconds(5);                    //małe opóźnienie dla prawidłowego działania enkodera
}

//testowanie ilości dostępnego RAMU 
/*
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
*/


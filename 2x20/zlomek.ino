//************************************************************************//
// Złomek - kumpel Heńka, projekt pogladowy obsługi DDS AD9850, 
// wyświetlacza nokii 5110 i jakiegoś enkodera.
// Projekt otwarty http://sp-hm.pl/thread-2164.html
// SQ9MDD -  początkowy szkielet programu v 1.0.0
// S_____ - 
// S_____ - 
//
//************************************************************************//
/* CHANGELOG (nowe na górze)
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
// lcd    - arduino
// sclk   - PIN 7
// sdin   - PIN 6
// dc     - PIN 5
// reset  - PIN 3
// sce    - PIN 4
LiquidCrystal lcd(7,6,5,3,4,2); 

//inicjalizujemy enkoder
//AO - w lewo
//A1 - w prawo
//nalezy pamiętać o kondensatorach (100nF) pomiędzy liniami encodera a masą
RotaryEncoder encoder(A0,A1,5,6,1000);  

//*****************************************************************************************************************************
//zmienne do modyfikacji każdy ustawia to co potrzebuje
const int pulses_for_groove = 2;             //ilość impulsów na ząbek enkodera zmienić w zależności od posiadanego egzemplarza
const int step_input = A2;                   //wejście do podłączenia przełącznika zmiany kroku
const long low_frequency_limit = 3500000;    //dolny limit częstotliwości
const long high_frequency_limit = 3800000;   //górny limit częstotliwości
const long start_frequency = 3715000;        //częstotliwość startowa
const long posrednia = -8000000;             //częstotliwość pośredniej, każdy dobiera swoją w zależności od konstrukcji radia
//*****************************************************************************************************************************

//zmienne wewnętrzne, 
//jeśli nie trzeba proszę nie modyfikować
char buffor[] = "              ";            //zmienna pomocnicza do wyrzucania danych na lcd
long frequency = start_frequency;            //zmienna dla częstotliwości, wstawiamy tam częstotliwość od której startujemy
long step_value = 100;                       //domyślny krok syntezy
int enc_sum = 0;                             //zmienna pomocnicza do liczenia impulsów z enkodera

//funkcja do obsługi wyświetlania zmiany częstotliwości
void show_frequency(){
  sprintf(buffor,"%08lu",frequency);         //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
  lcd.print(buffor);                //wyświetlamy dane na lcd 
}

//funkcja do wyświetlania aktualnego kroku syntezera
void show_step(){
  sprintf(buffor,"%08lu",step_value);         //konwersja danych do wyświetlenia (ładujemy krok syntezy do stringa i na ekran)
  lcd.print(buffor);                 //wyświetlamy dane na lcd (8 oznacza drugi rząd) 
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
  lcd.begin(20, 2);
  pinMode(step_input,INPUT_PULLUP);           //inicjalizujemy wejście zmiany kroku i podciągamy je do plusa
  set_frequency(0);                           //odpalamy syntezer i ustawiamy częstotliwość startową 
  delay(1000);                                //sekunda opóźnienia   
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
          case 100000:                    //jeśli krok jest 100kHz ustaw 100Hz
            step_value = 100;
          break;
          case 10000:                     //jeśli krok jest 10kHz ustaw 100kHz
            step_value = 100000;
          break;
          case 1000:                      //jeśli krok jest 1kHz ustaw 10kHz
            step_value = 10000;
          break;
          case 100:                       //jeśli krok jest 100Hz ustaw 1kHz
            step_value = 1000;
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
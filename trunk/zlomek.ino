//************************************************************************//
// Złomek - kumpel Heńka, projekt pogladowy obsługi DDS AD9850, 
// wyświetlacza nokii 5110 i jakiegoś enkodera.
// Projekt otwarty http://sp-hm.pl/thread-2164.html
// SQ9MDD - początkowy szkielet programu v 1.0.0 - 1.0.5
// SP6IFN - przejście na bibliotekę graficzną i parę zmian 1.0.5 s-metr
// S_____ - 
//
//************************************************************************//
/* CHANGELOG (nowe na górze)
 2014.10.2x - v.1.0.9 dodane opcje na lcd RIT, ATT, VFO/A/B SPLIT. Funkcjonalnie dzisiaj odpalam tylko RIT-a, reszta później
 2014.10.21 - v.1.0.8 dodałem możliwość pracy jako GEN lub SDR czyli dowolny mnożnik częstotliwości od 1 w zwyż, 
 patrz opcje konfiguracji, dodałem alternatywny sposób przeskalowania SP9MRN, zakomentowane można użyć zamiast MAP
 2014.10.20 - v.1.0.7 zmiana kierunku zmiany kroku syntezy, alternatywny sposób przeskalowania s-metra(sugestia SP9MRN)
 2014.10.20 - v.1.0.6 przepisany sposób wyświetlania danych (pozostał jeden znany bug do poprawki)
 wyczyszczone komentarze, dodanie s-metra według pomysłu Rysia SP6IFN
 2014.10.20 - v.1.0.5 wymiana biblioteki wyświetlacza LCD
 2014.10.19 - v.1.0.4 wprowadzamy pośrednią kilka zmiennych do sekcji konfiguracji
 drobne czyszczenie kodu, poprawki w komentarzach
 2014.10.15 - v.1.0.3 limit czestotliwości górnej i dolnej
 prprzeniesienie wysyłania częstotliwości do DDS do osobnej funkcji
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
#include <LCD5110_Graph.h> 

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
LCD5110 myGLCD(7,6,5,3,4); 
extern uint8_t TinyFont[];          //czcionka z biblioteki.....dodałem małe fonty (SP6IFN)
extern uint8_t SmallFont[];         //czcionka z biblioteki
extern uint8_t MediumNumbers[];     //czcionka z biblioteki

//inicjalizujemy enkoder
//AO - w lewo
//A1 - w prawo
//nalezy pamiętać o kondensatorach (100nF) pomiędzy liniami encodera a masą
RotaryEncoder encoder(A0,A1,5,6,1000);  

//*****************************************************************************************************************************
//zmienne do modyfikacji każdy ustawia to co potrzebuje
const int step_input = A2;                   //wejście do podłączenia przełącznika zmiany kroku
const int s_metr_port = A5;                  //wejście dla s-metra
const int rit_swich_input = 2;               //wejście do uruchamiania funkcji RIT
const int rit_analog_input_pin = A4;          //wejście do pomiaru wartości pokrętła RIT
const int ptt_input = 12;                    //wejście PTT procesor musi wiedzieć czy nadajemy czy odbieramy by zrealizować RIT-a
const int kontrast = 70;                     //kontrast wyświetlacza
const int pulses_for_groove = 2;             //ilość impulsów na ząbek enkodera zmienić w zależności od posiadanego egzemplarza
const long low_frequency_limit = 3500000;    //dolny limit częstotliwości
const long high_frequency_limit = 7200000;   //górny limit częstotliwości
const long start_frequency = 3715000;        //częstotliwość startowa
const long posrednia = -8000000;             //częstotliwość pośredniej, każdy dobiera swoją w zależności od konstrukcji radia
const int tryb_pracy = 0;                    //tryby pracy: 0-pośrednia, 1-generator, 2-lub wyżej, mnożnik razy 2 lub więcej
long step_value = 1000;                      //domyślny krok syntezy
const long s_metr_update_interval = 100;     //interwał odświeżania s-metra w msec
const long rit_range = 2000;                 //zakres pracy RIT +/- podana wartość, domyślnie 2000Hz
//*****************************************************************************************************************************

//zmienne wewnętrzne pomocnicze, 
//jeśli nie trzeba proszę nie modyfikować
char buffor[] = "              ";            //zmienna pomocnicza do wyrzucania danych na lcd
long frequency = start_frequency;            //zmienna dla częstotliwości, wstawiamy tam częstotliwość od której startujemy
int enc_sum = 0;                             //zmienna pomocnicza do liczenia impulsów z enkodera
long s_metr_update_time = 0;                 //zmienna pomocnicza do przechowywania czasu następnego uruchomienia s-metra
long frequency_to_dds = 0;                   //zmienna pomocnicza przechowuje częstotliwość którą wysyłam do DDS-a
boolean rit_state = false;                   //stan RIT-a przyjmuje wartość true lub false
boolean ptt_on = false;                      //stan przycisku PTT
boolean last_ptt_state = false;              //poprzedni stan PTT potrzebne do wykrywania zmianu stanu PTT
long rit_poprawka = 0;                       //domyślna wartość poprawki
int rit_last_input_value = 0;                //poprzednia wartość na przetworniku rita

//funkcja do obsługi wyświetlania zmiany częstotliwości
void show_frequency(){
  long f_prefix = frequency/1000;            //pierwsza część częstotliwości dużymi literkami
  long f_sufix = frequency%1000;             //obliczamy resztę z częstotliwości
  sprintf(buffor,"%05lu",f_prefix);          //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
  myGLCD.setFont(MediumNumbers);             //ustawiamy czcionkę dla dużych cyfr  
  myGLCD.print(buffor,1,13);                 //wyświetlamy duże cyfry na lcd 
  sprintf(buffor,".%03lu",f_sufix);          //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
  myGLCD.setFont(SmallFont);                 //ustawiamy małą czcionkę
  myGLCD.print(buffor,60,22);                //wyświetlamy małe cyfry na lcd 
  myGLCD.update();                           //wysyłamy dane do bufora wyświetlacza
}

//funkcja do wyświetlania aktualnego kroku syntezera za pomocą podkreślenie odpowiedniej cyfry
void show_step(){
       myGLCD.clrLine(0,31,83,31);          //czyszczę cała linię gdzie wyświetlam podkreślenie
       myGLCD.clrLine(0,32,83,32);          //czyszczę druga linię tak samo podkreśliniki są grube na dwa piksele
  switch(step_value){                       //przełącznik reaguje na bieżącą wartość kroku syntezy
     case 100:
        myGLCD.drawLine(69, 31, 82, 31);    //pokreślam 100Hz
        myGLCD.drawLine(69, 32, 82, 32);    //pokreślam 100Hz
     break;
    case 1000:
        myGLCD.drawLine(51, 31, 59, 31);    //pokreślam 1kHz
        myGLCD.drawLine(51, 32, 59, 32);    //pokreślam 1kHz       
    break; 
    case 10000:
        myGLCD.drawLine(39, 31, 47, 31);    //pokreślam 10kHz
        myGLCD.drawLine(39, 32, 47, 32);    //pokreślam 10kHz       
    break;
    case 100000:
        myGLCD.drawLine(27, 31, 35, 31);    //pokreślam 100kHz
        myGLCD.drawLine(27, 32, 35, 32);    //pokreślam 100kHz       
    break;    
  }
 myGLCD.update();  //jak już ustaliliśmy co rysujemy to wysyłamy to do LCD 
}

//funkcja ustawiająca częstotliwość DDS-a
void set_frequency(int plus_or_minus){
  if(plus_or_minus == 1){                                                        //jeśli na plus to dodajemy
    frequency = frequency + step_value;                                          //częstotliwość = częstotliwość + krok    
  }  
  else if(plus_or_minus == -1){                                                  //jeśli na minus to odejmujemy
    frequency = frequency - step_value;                                          //częstotliwość = częstotliwość - krok    
  }
  frequency = constrain(frequency,low_frequency_limit,high_frequency_limit);     //limitowanie zmiennej częstotliwości tej na wyświetlaczu 
  if(tryb_pracy == 0){                                                           //zmiana trybu pracy syntezy 0 - pośrednia
    frequency_to_dds = abs(posrednia + frequency);                               //a tutaj obliczam częstotliwość wynikową dla pracy w trybie pośredniej 
  }else{                                                                         //tryby pracy 1 - mnożnik * 1 generator lub 2 i więcej mnożnik
    frequency_to_dds = frequency * tryb_pracy;                                   //mnożymy częstotliwość przez tryb pracy
  }
  if(rit_state == true && ptt_on == false){
    frequency_to_dds = frequency_to_dds + rit_poprawka;
  }
  AD9850.set_frequency(frequency_to_dds);                                        //ustawiam syntezę na odpowiedniej częstotliwości  
  //Serial.println(frequency_to_dds);                                            //wypluwanie na rs-232 częstotliwości generatora (debugowanie)
}

//wskaźnik s-metra by nie przeszkadzał w pracy enkodera zrobiony jest na pseudo współdzieleniu czasu.
//każdorazowe wykonanie tej funkcji przestawia czas następnego jej wykonania o czas podany w konfiguracji domyślnie 100msec
void show_smetr(){                                
  if(millis() >= s_metr_update_time){                              //sprawdzam czy już jest czas na wykonanie funkcji
     myGLCD.clrLine(1, 45, 83, 45);                                //czyścimy stare wskazanie s-metra linia pierwsza
     myGLCD.clrLine(1, 46, 83, 46);                                //czyścimy stare wskazanie s-metra linia druga
     int s_value = analogRead(A5);                                 //czytamy wartość z wejścia gdzie jest podpięty sygnał s-metra
     int s_position = map(s_value,0,1023,1,83);                    //przeskalowuję zakres z wejścia analogowego na szerokość wyświetlacza
     //int s_position = (s_value*10)>>7;                           //alternatywny sposób przeskalowania SP9MRN
     myGLCD.drawLine(1, 45, s_position, 45);                       //rysuję nową linię wskazania s metra
     myGLCD.drawLine(1, 46, s_position, 46);                       //rysuję nową linię wskazania s metra
     myGLCD.update();                                              //wysyłam dane do bufora wyświetlacza
     s_metr_update_time = millis() + s_metr_update_interval;       //ustawiam czas kolejnego wykonania tej funkcji
  }
}

//funkcja która obsługuje klawisz wyłącznik RIT-a
void rit_swich(){
 if(rit_state == true){
  myGLCD.clrLine(72,0,83,0);
  myGLCD.clrLine(72,8,83,8);
  myGLCD.print("         ",CENTER,2); 
  rit_state = false; 
  //Serial.println("false");  
 }else{
  myGLCD.drawLine(72,0,83,0);
  myGLCD.drawLine(72,8,83,8); 
  sprintf(buffor,"%05lu",rit_poprawka);
  myGLCD.print(buffor,CENTER,2);
  rit_state = true; 
  //Serial.println("true");  
 } 
}

void ptt_switch(){
  myGLCD.setFont(TinyFont);
  if(last_ptt_state != ptt_on){
    if(ptt_on == true){
       myGLCD.print("TX", 0,2);       
    }else{
       myGLCD.print("RX", 0,2);       
    }
    myGLCD.update(); 
    set_frequency(0);
    last_ptt_state = ptt_on;
  }
}

void rit_knob(){   
   if(rit_state == true){
      int rit_input_value = analogRead(rit_analog_input_pin);
      if(rit_input_value > (rit_last_input_value + 3) || rit_input_value < (rit_last_input_value - 3)){        
        rit_poprawka = map(rit_input_value,0,1023,-(rit_range/10),(rit_range/10));
        rit_poprawka = rit_poprawka*10;
        Serial.println(rit_poprawka);
  sprintf(buffor,"%05lu",rit_poprawka);
  myGLCD.print(buffor,CENTER,2);        
        rit_last_input_value = rit_input_value; 
      }      
   }  
}

//tutaj rysujemy domyślne elementy ekranu będziemy to robić raz,
//tak by nie przerysowywać całego ekranu podczas pracy syntezy
void show_template(){
  myGLCD.setFont(TinyFont);                           //najmniejsza czcionka

  myGLCD.print("RX", 0,2);
  //myGLCD.drawLine(0,0,11,0);
  //myGLCD.drawLine(0,8,11,8);

  myGLCD.print("RIT", 72,2);
  //myGLCD.drawLine(72,0,83,0);
  //myGLCD.drawLine(72,8,83,8);

  //myGLCD.print("00000",CENTER,2);

  myGLCD.print("S1.3.5.7.9.+20.40.60.", CENTER, 38);  //opis dla s-metra
  myGLCD.drawRect(0, 44, 83, 47);                     //rysujemy prostokąt dla s-metra podając koordynaty narożników
  myGLCD.update();                                    //i wypluwamy to na lcd
}

// setup funkcja odpalana przy starcie
void setup(){  
  pinMode(s_metr_port,INPUT);             //
  pinMode(ptt_input,INPUT_PULLUP);        //
  pinMode(rit_swich_input,INPUT_PULLUP);  //
  Serial.begin(9600);                     //uruchamiam port szeregowy w celach diagnostycznych       
  myGLCD.InitLCD(kontrast);               //odpalamy lcd ustawiamy kontrast
  myGLCD.clrScr();
  pinMode(step_input,INPUT_PULLUP);       //inicjalizujemy wejście zmiany kroku i podciągamy je do plusa
  set_frequency(0);                       //odpalamy syntezer i ustawiamy częstotliwość startową 
  delay(1000);                            //sekunda opóźnienia   
  show_frequency();                       //pokazujemy częstotliwość na lcd
  show_step();                            //pokazujemy krok syntezy
  show_template();                        //pokazujemy domyślne stałe elementy LCD
} 

void loop(){
//obsługa PTT to musi być szybkie
  if(digitalRead(ptt_input) == LOW){       //odczytuję wejście PTT jesli jest aktywne 
    ptt_on = true;                         //ustawiam zmienna pomocniczą na prawda (flaga)
  }else{                                   //jeśli wejscie nie jest aktywne to
    ptt_on = false;                        //ustawiam zmienną pomocniczą na fałsz (zdejmuję flagę)
  }

  //obsługa enkodera
  int enc = encoder.readEncoder();        //czytamy wartość z encodera
  if(enc != 0) {                          //jeśli wartość jest inna niż zero sumujemy
    enc_sum = enc_sum + enc;              //jeden ząbek encodera to +2 lub -2 tutaj to zliczam
    //Serial.println(enc);                //wyrzucamy na port rs-232 wartość licznika enkodera (debugowanie)
    //przesuwam w czasie kolejne wykonanie updejtu s-metra 
    //bardzo topomaga przy szybkim kręceniu enkoderem, nie gubi wtedy kroków
    s_metr_update_time = millis() + s_metr_update_interval;       
  } 

  //obsługa klawisza zmiany kroku
  if(digitalRead(step_input) == LOW){     //sprawdzanie czy przycisk jest wcisnięty
    delay(100);                           //odczekajmy 100msec
    if(digitalRead(step_input) == LOW){   //jeśli klawisz nadal jest wcisnięty (czyli nie są to zakłócenia)
      switch(step_value){                 //za pomocą instrukcji swich zmieniamy krok
      case 100000:                        //jeśli krok jest 100kHz ustaw 10kHz
        step_value = 10000;
        break;
      case 10000:                         //jeśli krok jest 10kHz ustaw 1kHz
        step_value = 1000;
        break;
      case 1000:                          //jeśli krok jest 1kHz ustaw 100Hz
        step_value = 100;
        break;
      case 100:                           //jeśli krok jest 100Hz ustaw 100kHz
        step_value = 100000;
        break;
      }
    }
    show_step();                          //pokazuję zmianę kroku na lcd
    delay(200);                           //zwłoka po zmianie kroku 200msec
  }
  
  //obsługa klawisza włączenia funkcji RIT
  if(digitalRead(rit_swich_input) == LOW){
   delay(50);
    if(digitalRead(rit_swich_input) == LOW){
       rit_swich();
    delay(200);  
    } 
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
  delayMicroseconds(5);                   //małe opóźnienie dla prawidłowego działania enkodera

  //wywołuję funkcję do obsługi s-metra
  show_smetr();

  //wywołuję funkcję do obsługi PTT
  ptt_switch();  

  //wywołuję funkcję do obsługi pokrętła RIT
  rit_knob();
}

//testowanie ilości dostępnego RAMU 
/*
int freeRam () {
 extern int __heap_start, *__brkval;
 int v;
 return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
 }
*/


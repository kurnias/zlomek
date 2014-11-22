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
 2014.10.20 - v.1.0.7 zmiana kierunku zmiany kroku syntezy, alternatywny sposób przeskalowania s-metra(sugestia SP9MRN)
 2014.10.20 - v.1.0.6 przepisany sposób wyświetlania danych (pozostał jeden znany bug do poprawki)
 wyczyszczone komentarze, dodanie s-metra według pomysłu Rysia SP6IFN
 2014.10.20 - v.1.0.5 wymiana biblioteki wyświetlacza LCD
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
const int kontrast = 70;                     //kontrast wyświetlacza
const int pulses_for_groove = 2;             //ilość impulsów na ząbek enkodera zmienić w zależności od posiadanego egzemplarza
const int step_input = A2;                   //wejście do podłączenia przełącznika zmiany kroku
const long low_frequency_limit = 3500000;    //dolny limit częstotliwości
const long high_frequency_limit = 3800000;   //górny limit częstotliwości
const long start_frequency = 3715000;        //częstotliwość startowa
const long posrednia = -8000000;             //częstotliwość pośredniej, każdy dobiera swoją w zależności od konstrukcji radia
long step_value = 100;                       //domyślny krok syntezy
int s_metr_port = A5;                        //wejście dla s-metra
const long s_metr_update_interval = 100;     //interwał odświeżania s-metra w msec
//*****************************************************************************************************************************

//zmienne wewnętrzne, 
//jeśli nie trzeba proszę nie modyfikować
char buffor[] = "              ";            //zmienna pomocnicza do wyrzucania danych na lcd
long frequency = start_frequency;            //zmienna dla częstotliwości, wstawiamy tam częstotliwość od której startujemy
int enc_sum = 0;                             //zmienna pomocnicza do liczenia impulsów z enkodera
long s_metr_update_time = 0;                 //zmienna pomocnicza do przechowywania czasu następnego uruchomienia s-metra

//funkcja do obsługi wyświetlania zmiany częstotliwości
void show_frequency(){
  long f_prefix = frequency/1000;            //pierwsza część częstotliwości dużymi literkami
  long f_sufix = frequency%1000;             //obliczamy resztę z częstotliwości
  sprintf(buffor,"%lu",f_prefix);            //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
  myGLCD.setFont(MediumNumbers);             //ustawiamy czcionkę dla dużych cyfr  
  myGLCD.print(buffor,13,10);                //wyświetlamy duże cyfry na lcd 
  sprintf(buffor,".%03lu",f_sufix);          //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
  myGLCD.setFont(SmallFont);                 //ustawiamy małą czcionkę
  myGLCD.print(buffor,60,19);                //wyświetlamy małe cyfry na lcd 
  myGLCD.update();                           //wysyłamy dane do bufora wyświetlacza
}

//funkcja do wyświetlania aktualnego kroku syntezera za pomocą podkreślenie odpowiedniej cyfry
void show_step(){
       myGLCD.clrLine(0,28,83,28);          //czyszczę cała linię gdzie wyświetlam podkreślenie
       myGLCD.clrLine(0,29,83,29);          //czyszczę druga linię tak samo podkreśliniki są grube na dwa piksele
  switch(step_value){                       //przełącznik reaguje na bieżącą wartość kroku syntezy
     case 100:
        myGLCD.drawLine(69, 28, 82, 28);    //pokreślam 100Hz
        myGLCD.drawLine(69, 29, 82, 29);    //pokreślam 100Hz
     break;
    case 1000:
        myGLCD.drawLine(51, 28, 59, 28);    //pokreślam 1kHz
        myGLCD.drawLine(51, 29, 59, 29);    //pokreślam 1kHz       
    break; 
    case 10000:
        myGLCD.drawLine(39, 28, 47, 28);    //pokreślam 10kHz
        myGLCD.drawLine(39, 29, 47, 29);    //pokreślam 10kHz       
    break;
    case 100000:
        myGLCD.drawLine(27, 28, 35, 28);    //pokreślam 100kHz
        myGLCD.drawLine(27, 29, 35, 29);    //pokreślam 100kHz       
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
  frequency = constrain(frequency,low_frequency_limit,high_frequency_limit);     //limitowanie zmiennej 
  long frequency_to_dds = abs(posrednia + frequency);                            //a tutaj obliczam częstotliwość wynikową 
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

//tutaj rysujemy domyślne elementy ekranu będziemy to robić raz,
//tak by nie przerysowywać całego ekranu podczas pracy syntezy
void show_template(){
  myGLCD.setFont(TinyFont);                           //najmniejsza czcionka
  myGLCD.print("S1.3.5.7.9.+20.40.60.", CENTER, 38);  //opis dla s-metra
  myGLCD.drawRect(0, 44, 83, 47);                     //rysujemy prostokąt dla s-metra podając koordynaty narożników
  myGLCD.update();                                    //i wypluwamy to na lcd
}

// setup funkcja odpalana przy starcie
void setup(){  
  pinMode(s_metr_port,INPUT);
  Serial.begin(9600);                     //uruchamiam port szeregowy w celach diagnostycznych       
  myGLCD.InitLCD(kontrast);               //odpalamy lcd ustawiamy kontrast
  pinMode(step_input,INPUT_PULLUP);       //inicjalizujemy wejście zmiany kroku i podciągamy je do plusa
  set_frequency(0);                       //odpalamy syntezer i ustawiamy częstotliwość startową 
  delay(1000);                            //sekunda opóźnienia   
  show_frequency();                       //pokazujemy częstotliwość na lcd
  show_step();                            //pokazujemy krok syntezy
  show_template();                        //pokazujemy domyślne stałe elementy LCD
} 

void loop(){  
  int enc = encoder.readEncoder();        //czytamy wartość z encodera
  if(enc != 0) {                          //jeśli wartość jest inna niż zero sumujemy
    enc_sum = enc_sum + enc;              //jeden ząbek encodera to +2 lub -2 tutaj to zliczam
    //Serial.println(enc);                //wyrzucamy na port rs-232 wartość licznika enkodera (debugowanie)
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
  delayMicroseconds(5);                   //małe opóźnienie dla prawidłowego działania enkodera
  show_smetr();
  
}

//testowanie ilości dostępnego RAMU 
/*
int freeRam () {
 extern int __heap_start, *__brkval;
 int v;
 return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
 }
*/


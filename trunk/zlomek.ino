//************************************************************************//
// Złomek - kumpel Heńka, projekt pogladowy obsługi DDS AD9850, 
// wyświetlacza nokii 5110 i jakiegoś enkodera.
// Projekt otwarty http://sp-hm.pl/thread-2164.html
// SQ9MDD - początkowy szkielet programu v 1.0.0 - 1.0.5
// SP6IFN - przejście na bibliotekę graficzną i parę zmian 1.0.5 s-metr
// !!!Miejsce na twój znak !!!
//************************************************************************//
/* CHANGELOG
 2014.10.24 - v.1.0.11 czyszczenie kodu, zmiana czcionki RIT, poprawki w komentarzach najmniejszy krok syntezy 50Hz, 
 drobne prace nad optymalizacją kodu
 2014.10.23 - v.1.0.10 dodana obsługa PTT i RIT --uff to wcale nie było proste ;) mam nadzieję że nie zagmatwałem kodu
 2014.10.23 - v.1.0.9 wersja nieudana poszła w kosz (RIT na potencjometrze)
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
extern uint8_t TinyFont[];       //czcionka z biblioteki.....dodałem małe fonty (SP6IFN)
extern uint8_t SmallFont[];      //czcionka z biblioteki
extern uint8_t MediumNumbers[];  //czcionka z biblioteki

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
const int ptt_input = 12;                    //wejście PTT procesor musi wiedzieć czy nadajemy czy odbieramy by zrealizować RIT-a
const int contrast = 70;                     //kontrast wyświetlacza
const int pulses_for_groove = 2;             //ilość impulsów na ząbek enkodera zmienić w zależności od posiadanego egzemplarza
const long low_frequency_limit = 3500000;    //dolny limit częstotliwości
const long high_frequency_limit = 7200000;   //górny limit częstotliwości
const long start_frequency = 3715000;        //częstotliwość startowa syntezy
const long if_frequency = -8000000;          //częstotliwość pośredniej, każdy dobiera swoją w zależności od konstrukcji radia
const int tryb_pracy = 1;                    //tryby pracy: 0-pośrednia, 1-generator, 2-lub wyżej, mnożnik razy 2 lub więcej
long step_value = 1000;                      //domyślny krok syntezy
const long s_metr_update_interval = 100;     //interwał odświeżania s-metra w msec
const long rit_range = 2000;                 //zakres pracy RIT +/- podana wartość, domyślnie 2000Hz max 9999Hz jeśli dasz wiecej posypie się wyświetlanie
const long rit_step = 50;                    //krok działania RIT-a domyślnie 50Hz

//zmienne wewnętrzne pomocnicze, 
//jeśli nie trzeba proszę nie modyfikować
char buffor[] = "              ";            //zmienna pomocnicza do wyrzucania danych na lcd
long frequency = start_frequency;            //zmienna dla częstotliwości, wstawiamy tam częstotliwość od której startujemy
int enc_sum = 0;                             //zmienna pomocnicza do liczenia impulsów z enkodera
long s_metr_update_time = 0;                 //zmienna pomocnicza do przechowywania czasu następnego uruchomienia s-metra
long frequency_to_dds = 0;                   //zmienna pomocnicza przechowuje częstotliwość którą wysyłam do DDS-a
int rit_state = 0;                           //stan RIT-a 0-rit off, 1-rit on, 2-rit on enkoder odchyłkę rit 
boolean ptt_on = false;                      //stan przycisku PTT
boolean last_ptt_state = false;              //poprzedni stan PTT potrzebne do wykrywania zmianu stanu PTT
long rit_poprawka = 0;                       //domyślna wartość poprawki
//*****************************************************************************************************************************

//FUNKCJE
//funkcja do obsługi wyświetlania zmiany częstotliwości
void show_frequency(){
  if(rit_state != 1){                              //Jeśli Enkoder pracuje jako VFO zaoszczędzimy trochę czasu procesora jesli enkoder pracuje jako RIT
    long f_prefix = frequency/1000;                //pierwsza część częstotliwości dużymi literkami
    long f_sufix = frequency%1000;                 //obliczamy resztę z częstotliwości
    sprintf(buffor,"%05lu",f_prefix);              //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
    myGLCD.setFont(MediumNumbers);                 //ustawiamy czcionkę dla dużych cyfr  
    myGLCD.print(buffor,1,13);                     //wyświetlamy duże cyfry na lcd 
    sprintf(buffor,".%03lu",f_sufix);              //konwersja danych do wyświetlenia (ładujemy częstotliwość do stringa i na ekran)
    myGLCD.setFont(SmallFont);                     //ustawiamy małą czcionkę
    myGLCD.print(buffor,60,22);                    //wyświetlamy małe cyfry na lcd 
  }
  if(rit_state == 1){                              //jeśli RIT jest włączony i enkoder pracuje jako RIT wyświetlamy zmiany częstotliwości RIT-a
    myGLCD.setFont(TinyFont);                      //ustawiamy małą czcionkę
    sprintf(buffor,"%05lu",abs(rit_poprawka));     //przygotowujemy bufor z zawartością aktualnej wartości RIT
    myGLCD.print(buffor,CENTER,2);                 //drukowanie na lcd
    if(rit_poprawka < 0){                          //obsługa znaku poprawki RIT jeśli mniejsza niż 0
      myGLCD.print("-",28,2);                      //drukujemy minus
    }else if(rit_poprawka > 0){                    //jeśli większa niż zero to
      myGLCD.print("+",28,2);                      //drukujemy plus
    }                                        
    else{
      myGLCD.print("0",28,2);                      //jeśli poprawka zerowa wrzucam zero zamiast plusa czy minusa
    }
  }
  myGLCD.update();                                 //wysyłamy dane do bufora wyświetlacza
}

//funkcja do wyświetlania aktualnego kroku syntezera za pomocą podkreślenie odpowiedniej cyfry
void show_step(){       
       myGLCD.clrLine(0,31,83,31);          //czyszczę cała linię gdzie wyświetlam podkreślenie
       myGLCD.clrLine(0,32,83,32);          //czyszczę druga linię tak samo podkreśliniki są grube na dwa piksele
  switch(step_value){                       //przełącznik reaguje na bieżącą wartość kroku syntezy
     case 50:
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
  if(rit_state == 2 || rit_state == 0){                                            //jeśli nie obsługuję RIT-a to manipuluje częstotliwością 
    if(plus_or_minus == 1){                                                        //jeśli na plus to dodajemy
      frequency = frequency + step_value;                                          //częstotliwość = częstotliwość + krok    
    }  
    else if(plus_or_minus == -1){                                                  //jeśli na minus to odejmujemy
      frequency = frequency - step_value;                                          //częstotliwość = częstotliwość - krok    
    }
  }
  if(rit_state == 1){                                                              //jesli obsługuję rita
    if(plus_or_minus == 1){                                                        //jeśli na plus to dodajemy
      rit_poprawka = rit_poprawka + rit_step;                                      //częstotliwość poprawki zwiększam o krok poprawki
    }  
    else if(plus_or_minus == -1){                                                  //jeśli na minus to odejmujemy
      rit_poprawka = rit_poprawka - rit_step;                                      //częstotliwość poprawki zmniejszam o krok poprawki    
    }
  rit_poprawka = constrain(rit_poprawka,-rit_range,rit_range);                     //limitujemy poprawkę RIT do wartości z konfiguracji
}
  int poprawka = 0;                                                                //lokalna zmienna pomocnicza
  if(rit_state != 0 && ptt_on == false){                                           //jeśli jesteśmy w trybie włączonego RIT-a
    poprawka = rit_poprawka;                                                       //lokalna zmienna pomocnicza przyjmuje wartość RIT by można to było dodać do czestotliwości 
  }
  frequency = constrain(frequency,low_frequency_limit,high_frequency_limit);       //limitowanie zmiennej częstotliwości tej na wyświetlaczu 
  if(tryb_pracy == 0){                                                             //zmiana trybu pracy syntezy 0 - pośrednia
    frequency_to_dds = abs(if_frequency + frequency + poprawka);                      //a tutaj obliczam częstotliwość wynikową dla pracy w trybie pośredniej + ew.poprawka z RIT
  }else{                                                                           //tryby pracy 1 - mnożnik * 1 generator lub 2 i więcej mnożnik
    frequency_to_dds = (frequency + poprawka) * tryb_pracy;                        //mnożymy częstotliwość przez tryb pracy no i pamiętamy o poprawce
  }
  AD9850.set_frequency(frequency_to_dds);                                          //ustawiam syntezę na odpowiedniej częstotliwości  
  Serial.println(frequency_to_dds);                                              //debugowanie
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

//funkcja która obsługuje klawisz RIT-a
void rit_swich(){
 myGLCD.setFont(TinyFont); 
 switch(rit_state){
  case 1:  //rit aktywny, enkoder steruje wartoscią RIT-a
     myGLCD.drawLine(28, 0, 51, 0);             //pokreślam wartość RIT
     myGLCD.drawLine(28, 8, 51, 8);             //pokreślam wartość RIT 
     myGLCD.drawLine(72,0,83,0);                //podkreślam oznaczenie rit
     myGLCD.drawLine(72,8,83,8);                //podkreślam oznaczenie rit 
     show_frequency(); 
  break;
  case 2:  //rit aktywny, enkoder steruje częstotliwością
     myGLCD.clrLine(28, 0, 51, 0);              //anauluję podkreślenie wartości RIT
     myGLCD.clrLine(28, 8, 51, 8);              //anauluję podkreślenie wartości RIT 
     myGLCD.drawLine(72,0,83,0);                //podkreślam oznaczenie rit
     myGLCD.drawLine(72,8,83,8);                //podkreślam oznaczenie rit 
  break;
  case 0:  //rit nie jest aktywny
     myGLCD.clrLine(28, 0, 51, 0);              //anauluję podkreślenie wartości RIT
     myGLCD.clrLine(28, 8, 51, 8);              //anauluję podkreślenie wartości RIT
     myGLCD.clrLine(72,0,83,0);                 //anuluję oznaczenie rit
     myGLCD.clrLine(72,8,83,8);                 //anuluję oznaczenie rit
     sprintf(buffor,"        ",rit_poprawka);   //czyszczę miejsce po wartości RIT gdy pracujemy bez niego
     myGLCD.print(buffor,CENTER,2);             //przygotowuję dane do wysyłki na LCD
  break; 
 }
  myGLCD.update();                              //i wypluwamy to na lcd
  set_frequency(0);                             //ustawiam częstotliwość pracy syntezy na wypadek gdy wartość rit jest różna od zera
}

//sygnalizacja PTT (sygnalizacja to skutek uboczny dla RIT-a musimy wiedzieć czy odbieramy czy nadajemy)
void ptt_switch(){
  myGLCD.setFont(TinyFont);                //ustawiam małą czcionkę
  if(last_ptt_state != ptt_on){            //sprawdzam czy zmienił się stan PTT jeśli tak to robię update na LCD
    if(ptt_on == true){                    //jeśli TX
       myGLCD.print("T", 0,2);             //to zapalamy T do TX
    }else{                                 //jesli RX
       myGLCD.print("R", 0,2);             //zapalamy R do RX
    }
    myGLCD.update();                       //wyrzucam do bufora wyświetlacza
    set_frequency(0);                      //ustawiam częstotliwość (bo może być różna dla TX i RX)
    last_ptt_state = ptt_on;               //uaktualniam poprzedni stan PTT
  }
}

//tutaj rysujemy domyślne elementy ekranu będziemy to robić raz,
//tak by nie przerysowywać całego ekranu podczas pracy syntezy
void show_template(){
  myGLCD.setFont(TinyFont);                           //najmniejsza czcionka
  myGLCD.print("RX", 0,2);                            //Sygnalizacja TX RX będzie tutaj
  myGLCD.print("RIT", 72,2);                          //Sygnalizacja pracy RIT-u tutaj
  myGLCD.print("S1.3.5.7.9.+20.40.60.", CENTER, 38);  //opis dla s-metra
  myGLCD.drawRect(0, 44, 83, 47);                     //rysujemy prostokąt dla s-metra podając koordynaty narożników
  myGLCD.update();                                    //i wypluwamy to na lcd
}

//*****************************************************************************************************************************
//FUNKCJE OBOWIĄZKOWE
//setup funkcja odpalana przy starcie
void setup(){  
  pinMode(s_metr_port,INPUT);             //ustawiam tryb pracy wejścia s-metra
  pinMode(ptt_input,INPUT_PULLUP);        //ustawiam tryb pracy wejścia PTT
  pinMode(rit_swich_input,INPUT_PULLUP);  //ustawiam tryb pracy wejścia przełącznika RIT
  Serial.begin(9600);                     //uruchamiam port szeregowy w celach diagnostycznych       
  myGLCD.InitLCD(contrast);               //odpalamy lcd ustawiamy kontrast
  myGLCD.clrScr();                        //czyścimy ekran z ewentualnych śmieci
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
  ptt_switch();                            //wywołuję funkcję do obsługi PTT  

  //obsługa enkodera
  int enc = encoder.readEncoder();        //czytamy wartość z encodera
  if(enc != 0) {                          //jeśli wartość jest inna niż zero sumujemy
    enc_sum = enc_sum + enc;              //jeden ząbek encodera to +2 lub -2 tutaj to zliczam
    //Serial.println(enc);                //wyrzucamy na port rs-232 wartość licznika enkodera (debugowanie)
    //przesuwam w czasie kolejne wykonanie updejtu s-metra 
    //bardzo to pomaga przy szybkim kręceniu enkoderem, nie gubi wtedy kroków
    s_metr_update_time = millis() + s_metr_update_interval;       
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
  
  //obsługa klawisza zmiany kroku
  if(digitalRead(step_input) == LOW){     //sprawdzanie czy przycisk jest wcisnięty
    delay(50);                            //zwłoka by wyeliminować drgania styków
    if(digitalRead(step_input) == LOW){   //jeśli klawisz nadal jest wcisnięty (czyli nie są to zakłócenia)
      switch(step_value){                 //za pomocą instrukcji swich zmieniamy krok
      case 100000:                        //jeśli krok jest 100kHz ustaw 10kHz
        step_value = 10000;
        break;
      case 10000:                         //jeśli krok jest 10kHz ustaw 1kHz
        step_value = 1000;
        break;
      case 1000:                          //jeśli krok jest 1kHz ustaw 50Hz
        step_value = 50;
        break;
      case 50:
        step_value = 100000;
      break;
      }
     show_step();                          //pokazuję zmianę kroku na lcd
     delay(200);                           //zwłoka po zmianie kroku 200msec     
    }
  }
 
  //obsługa klawisza włączenia funkcji RIT
  if(digitalRead(rit_swich_input) == LOW){    //jeśli klawisz wciśnięty
   delay(50);                                 //zwłoka by wyeliminować drgania styków
    if(digitalRead(rit_swich_input) == LOW){  //jeśli nadal wciśnięty (eliminuję drgania styku)
       switch(rit_state){                     //przełącznik trybu pracy z RIT
        case 1:                               //jesli tryb jest 1 (enkoder pracuje jako rit wartość RIT dodaję do częstotliwości)  
         rit_state = 2;                       //ustaw tryb 2
        break;
       case 2:                                //jeśli tryb jest 2 (enkoder pracuje jako enkoder wartość RIT dodaję do częstotliwości)
        rit_state = 0;                        //ustaw tryb 0
       break;
       case 0:                                //jeśli tryb jest 0 (enkoder pracuje jako enkoder wartość RIT wyłączony)  
        rit_state = 1;                        //ustaw tryb 2
       break; 
       }
       rit_swich();                           //odpalam funkcję do obsługi trybu pracy  
    delay(200);                               //zwłoka dla prawidłowego działania
    } 
  }
 
  show_smetr();                               //wywołuję funkcję do obsługi s-metra
  //Serial.println(freeRam());                //testowanie ilości dostępnego RAM-u aby zadziałało należy odkomentować funkcję poniżej
}

//KONIEC PROGRAMU
//*****************************************************************************************************************************

//testowanie ilości dostępnego RAMU 
//int freeRam () {
// extern int __heap_start, *__brkval;
// int v;
// return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
// }



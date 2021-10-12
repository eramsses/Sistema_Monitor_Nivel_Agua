//Notas musicales
#include "notas.h"

//Librerias
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

//DEFINICION DE LOS BOTONES
#define BTN_MODO 0 //3
#define BTN_OK 1 //4
#define BTN_DWN 2 //5
#define BTN_UP 3 //6

uint8_t boton[4] = {3, 4, 5, 6};

uint8_t estado_boton[4]; //Guarda el estado de los botones

uint8_t valorBtnOK;
uint8_t valorBtnModo_flanco;

//CARACTERES ESPECIALES
byte check[] = {B00000, B00001, B00011, B10110, B11100, B01000, B00000, B00000};

byte punto[] = {B00011, B00011, B00000, B00000, B00000, B00000, B00000, B00000};

byte baja1[] = {B00100, B10101, B01110, B00100, B00000, B00000, B00000, B00000};

byte baja2[] = {B00000, B00100, B00100, B10101, B01110, B00100, B00000, B00000};

byte baja3[] = { B00000, B00000, B00000, B00100, B00100, B10101, B01110, B00100};

byte sube1[] = {B00000, B00000, B00000, B00000, B00100, B01110, B10101, B00100};

byte sube2[] = {B00000, B00000, B00100, B01110, B10101, B00100, B00100, B00000};

byte sube3[] = {B00100, B01110, B10101, B00100, B00100, B00000, B00000, B00000};

byte carga1[] = {B00100, B00100, B00100, B00100, B00100, B00100, B00100, B00100};

byte carga2[] = {B00000, B00001, B00011, B00110, B01100, B11000, B10000, B00000};

byte carga3[] = {B00000, B00000, B00000, B11111, B11111, B00000, B00000, B00000};

byte carga4[] = {B00000, B10000, B11000, B01100, B00110, B00011, B00001, B00000};

//DEFINICIONES PARA SENSOR ULTRASONICO
#define GND_SENSOR 10
#define VCC_SENSOR 11
#define TRIG 12 //Trigger en pin A7
#define ECHO 13 //Echo en pin 10
#define MUESTRA 200

//DEFINICIONES PARA LEDs
#define LED_AMARILLO 7
#define LED_VERDE 8

//DEFINICION BUZZER
#define BUZZER 9

//ESTADOS DEL MENU
#define M_MONITOR 0
#define M_LLENADO 1
#define M_CONFIGURACION_LARGO 2
#define M_CONFIGURACION_ANCHO 3
#define M_CONFIGURACION_ALTO_SENSOR 4
#define M_CONFIGURACION_ALTO_AGUA 5
#define M_CONFIGURACION_FACTOR_CORRECCION 6

//DEFINICIONES DE LAS DIRECCIONES DE MEMORIA EEPROM PARA LAS MEDIDAS DEL TANQUE
#define EE_LARGO 0
#define EE_ANCHO 2
#define EE_ALTURA 4
#define EE_ALTURA_AGUA 6
#define EE_FACTOR_CORRECCION 8

#define ALTURA_MAXIMA 400;

volatile int modo = M_MONITOR; //Modo actual

//VARIABLES
int dist = 0;
unsigned int altura = 100, largo = 100, ancho = 100, altura_max_agua = 0, factor_correccion = 0;
int altura_agua_cm_inicial, altura_agua_cm_actual;

unsigned long tiempoInicial, tiempoActual;

//CONSTANTES DE CONVERSION PARA MONITOREO Y LLENADO
#define BARRIL_x_METRO_3 6.29
#define GLS_x_BARRIL 42
#define MIN_CAMBIO_CM 3
#define EST_LLENANDO 0
#define EST_CALCULANDO 1
#define EST_VACIANDO 3


void setup() {

  //Inicialización de pines del sensor
  pinMode(GND_SENSOR, OUTPUT);//Pin de tierra para el sensor 0v
  pinMode(VCC_SENSOR, OUTPUT);//Pin de voltaje para el sensor +5v
  digitalWrite(GND_SENSOR, LOW); // 0v activo
  digitalWrite(VCC_SENSOR, HIGH); //+5v activo
  pinMode(TRIG, OUTPUT);  // trigger como salida
  pinMode(ECHO, INPUT_PULLUP);    // echo como entrada
  delay(100);
  //Inicialización de pines de los LEDs
  pinMode(LED_AMARILLO, OUTPUT);   // LED para bomba
  pinMode(LED_VERDE, OUTPUT);   // LED para marcar que esta entrando agua desde el proveedor
  //digitalWrite(LED_AMARILLO, HIGH);
  //digitalWrite(LED_VERDE, HIGH);

  //Botones del menú en modo PULLUP
  pinMode(boton[BTN_MODO], INPUT_PULLUP);
  pinMode(boton[BTN_OK], INPUT_PULLUP);
  pinMode(boton[BTN_DWN], INPUT_PULLUP);
  pinMode(boton[BTN_UP], INPUT_PULLUP);

  lcd.createChar(0, check);
  lcd.createChar(1, punto);

  //Inicializar botones en ALTO de array de estados para verificar que hay flaco de subida
  estado_boton[0] = HIGH;
  estado_boton[1] = HIGH;
  estado_boton[2] = HIGH;
  estado_boton[3] = HIGH;

  //Iniciar LCD
  lcd.init(); //Iniciar
  lcd.backlight(); //Activar Retroiluminación
  lcd.clear(); //Limpiar cualquier caracter extraño

  //Iniciar las medidas del tanque leyendo desde la EEPROM en sus respectivas variables
  EEPROM.get(EE_LARGO, largo);
  EEPROM.get(EE_ANCHO, ancho);
  EEPROM.get(EE_ALTURA, altura);
  EEPROM.get(EE_ALTURA_AGUA, altura_max_agua);
  EEPROM.get(EE_FACTOR_CORRECCION, factor_correccion);

  //Llamar al efecto de inicio
  efectoInicial();

  //Boton para manejar la interrupción
  attachInterrupt(digitalPinToInterrupt(boton[BTN_MODO]), cambioModo, RISING);
}

void loop() {
  //Cambio entre los diferentes estados
  switch (modo) {
    case M_MONITOR://Inicio monitor
      lcd.clear();
      modoMonitor();
      break;//Fin monitor
    case M_LLENADO://Modo Llenado
      lcd.clear();
      modoLlenando();
      break;
    case M_CONFIGURACION_LARGO:
      lcd.clear();
      modoConfiguracionL();
      break;
    case M_CONFIGURACION_ANCHO:
      lcd.clear();
      modoConfiguracionA();
      break;
    case M_CONFIGURACION_ALTO_SENSOR:
      lcd.clear();
      modoConfiguracionAltoSensor();
      break;
    case M_CONFIGURACION_ALTO_AGUA:
      lcd.clear();
      modoConfiguracionAltoAgua();
      break;
    case M_CONFIGURACION_FACTOR_CORRECCION:
      lcd.clear();
      modoConfiguracionFactorCorreccion();
      break;
  }
}

void cambioModo() {

  valorBtnOK = digitalRead(boton[BTN_OK]);//recupera el valor del estado del btn
  valorBtnModo_flanco = flancoSubida(BTN_MODO);
  switch (modo) {
    case M_MONITOR://Inicio monitor

      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_CONFIGURACION_LARGO;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_LLENADO;
      }
      break;//Fin monitor
    case M_LLENADO:
      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_CONFIGURACION_LARGO;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_MONITOR;
      }
      break;
    case M_CONFIGURACION_LARGO:
      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_MONITOR;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_CONFIGURACION_ANCHO;

      }
      break;
    case M_CONFIGURACION_ANCHO:
      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_MONITOR;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_CONFIGURACION_ALTO_SENSOR;

      }
      break;
    case M_CONFIGURACION_ALTO_SENSOR:
      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_MONITOR;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_CONFIGURACION_ALTO_AGUA;

      }
      break;
    case M_CONFIGURACION_ALTO_AGUA:
      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_MONITOR;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_CONFIGURACION_FACTOR_CORRECCION;

      }
      break;
    case M_CONFIGURACION_FACTOR_CORRECCION:
      if (valorBtnModo_flanco && valorBtnOK == LOW) {
        modo = M_MONITOR;
      }
      if (valorBtnModo_flanco && valorBtnOK == HIGH) {
        modo = M_CONFIGURACION_LARGO;

      }
      break;
  }


  delay(500);

}

void modoMonitor() {


  lcd.setCursor(3, 0);
  lcd.print("* MONITOREO *");

  int d = 0;
  int d_a = 0;
  int altura_agua_cm = 0;
  float altura_agua_m =  0.00;
  float largo_m = 0.00;
  float ancho_m = 0.00;
  float altura_maxima_agua_m = 0.00;
  float porcentaje_agua = 0.00;
  int p_a = 0;

  notaAtras();

  boolean p = true;

  while (modo == M_MONITOR) {

    //mostrarDistancia();
    d = obtenerDistancia();
    altura_agua_cm = altura - d;
    altura_agua_m = (float)altura_agua_cm / 100;
    largo_m = (float)largo / 100;
    ancho_m = (float)ancho / 100;
    altura_maxima_agua_m = (float)altura_max_agua / 100;
    porcentaje_agua = (float)altura_agua_cm / altura_max_agua;
    p_a  = porcentaje_agua * 100;


    float vol_agua_m = (float)largo_m * ancho_m * altura_agua_m;

    float vol_agua_barriles = (float)vol_agua_m * BARRIL_x_METRO_3;
    float vol_agua_galones = (float)vol_agua_barriles * GLS_x_BARRIL;

    if (p) {
      lcd.setCursor(19, 0);
      lcd.write((byte)1);
    } else {
      lcd.setCursor(19, 0);
      lcd.print(" ");
    }
    p = !p;

    if (d_a != d) {
      d_a = d;
      lcd.setCursor(0, 1);
      lcd.print("                    ");
      lcd.setCursor(0, 1); lcd.print(altura_agua_m); lcd.print(" m de agua");

      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 2); lcd.print(vol_agua_galones); lcd.print(" gal. "); lcd.print(p_a); lcd.print("%");

      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      lcd.print("Hay "); lcd.print(vol_agua_barriles); lcd.print(" barriles");
    }
    delay(250);
  }
}

void modoLlenando() {

  lcd.setCursor(4, 0);
  lcd.print("* LLENANDO *");

  lcd.setCursor(0, 3);
  lcd.print("     CALCULANDO     ");


  int cm_llenados = 0;
  int estado_llenado = 0;
  int d = 0;
  int d_a = 0;
  int altura_agua_cm = 0;
  float altura_agua_m =  0.00;
  float largo_m = 0.00;
  float ancho_m = 0.00;
  float altura_maxima_agua_m = 0.00;
  float porcentaje_agua = 0.00;
  int p_a = 0;
  unsigned long tt = 0;

  notaAdelante();

  boolean p = true;

  d = obtenerDistancia();
  altura_agua_cm_inicial = altura - d;
  int altura_agua_cm_inicial_i = altura_agua_cm_inicial;

  tiempoInicial = millis();


  while (modo == M_LLENADO) {

    if (p) {
      lcd.setCursor(19, 0);
      lcd.write((byte)1);
    } else {
      lcd.setCursor(19, 0);
      lcd.print(" ");
    }
    p = !p;

    //Calculo de tiempo de llenado
    tiempoActual = millis();
    d = obtenerDistancia();
    altura_agua_cm = altura - d;
    altura_agua_m = (float)altura_agua_cm / 100;
    largo_m = (float)largo / 100;
    ancho_m = (float)ancho / 100;
    altura_maxima_agua_m = (float)altura_max_agua / 100;
    porcentaje_agua = (float)altura_agua_cm / altura_max_agua;
    p_a  = porcentaje_agua * 100;

    float vol_agua_m = (float)largo_m * ancho_m * altura_agua_m;

    float vol_agua_barriles = (float)vol_agua_m * BARRIL_x_METRO_3;
    float vol_agua_galones = (float)vol_agua_barriles * GLS_x_BARRIL;

    altura_agua_cm_actual = altura_agua_cm;

    if ((altura_agua_cm_actual - altura_agua_cm_inicial_i) > 0) { //Llenando
      altura_agua_cm_inicial_i = altura_agua_cm_actual;
      estado_llenado = EST_LLENANDO;

      if ((altura_agua_cm_actual - altura_agua_cm_inicial) >= MIN_CAMBIO_CM) { //Calcular tiempo de llenado

        cm_llenados = altura_agua_cm_actual - altura_agua_cm_inicial;
        tt = abs(tiempoActual - tiempoInicial);
        altura_agua_cm_inicial = altura_agua_cm_actual;
        tiempoInicial = tiempoActual;

        calcularTiempoLLenado(altura_agua_cm_inicial, cm_llenados, tt);

        altura_agua_cm_inicial = altura_agua_cm_actual;
        estado_llenado = EST_LLENANDO;

      }

    } else if ((altura_agua_cm_actual - altura_agua_cm_inicial_i) < 0) {//Vaciando

      if (altura_agua_cm_actual - altura_agua_cm_inicial_i > 10) {
        altura_agua_cm_inicial = altura_agua_cm_actual;
      }
      altura_agua_cm_inicial_i = altura_agua_cm_actual;
      estado_llenado = EST_VACIANDO;
    }

    if (d_a != d) {
      d_a = d;
      lcd.setCursor(0, 1);
      lcd.print("                    ");
      lcd.setCursor(0, 1); lcd.print(altura_agua_m); lcd.print(" m de agua");

      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 2); lcd.print(vol_agua_galones); lcd.print(" gal. "); lcd.print(p_a); lcd.print("%");
    }

    switch (estado_llenado) {
      case EST_LLENANDO:
        mostrarSubiendo();
        break;//Fin monitor
      case EST_CALCULANDO:
        mostrarCalculando();
        lcd.setCursor(0, 3);
        lcd.print("     CALCULANDO     ");
        break;
      case EST_VACIANDO:
        //lcd.setCursor(0, 3);
        //lcd.print("");
        mostrarBajando();
        break;
    }

    delay(250);
  }
}

void mostrarDistancia() {
  dist = obtenerDistancia();
  lcd.setCursor(0, 1);
  lcd.print("Distancia: ");
  lcd.setCursor(11, 1);
  lcd.print("        ");
  lcd.setCursor(11, 1);
  lcd.print(dist);
  //lcd.setCursor(15,1);
  lcd.print(" cm");
}

void calcularTiempoLLenado(int altura_inicial_agua, int cm_llenados, unsigned long tiempo_millis) {

  int altura_agua_cm = altura - obtenerDistancia();

  int faltan = altura_max_agua - altura_inicial_agua - cm_llenados;
  float partes = (faltan / cm_llenados);

  unsigned long tiempo_millis_restante = abs(partes * tiempo_millis);

  char tr[12];
  int Hr = 0, Mr = 0, Sr = 0;

  Sr = tiempo_millis_restante / 1000;

  Mr += Sr / 60;
  Sr = Sr % 60;
  Hr += Mr / 60;
  Mr = Mr % 60;

  sprintf(tr, "Falta: %02d:%02d:%02d", Hr, Mr, Sr);

  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(tr);
}

/*
int obtenerDistancia() {
  int duracion;
  int distancia;

  digitalWrite(TRIG, HIGH);     // generacion del pulso a enviar
  delay(1);       // al pin conectado al trigger
  digitalWrite(TRIG, LOW);    // del sensor

  duracion = pulseIn(ECHO, HIGH);  // con funcion pulseIn se espera un pulso
  // alto en Echo
  distancia = (duracion / 29.0) + factor_correccion; //58.0;    // distancia medida en centimetros 58.2

  if (distancia < 0) {
    distancia = 0;
  }
  delay(100);       // demora entre datos
  return distancia;
}*/

int obtenerDistancia(){
  int sumaDistancias = 0;
  int distanciaPromedio = 0;

  for(int i = 0; i < MUESTRA; i++){
    sumaDistancias += obtenerDistanciaSimple();
  }
  distanciaPromedio = sumaDistancias / MUESTRA;

  return distanciaPromedio;
}

int obtenerDistanciaSimple(){
  int duracion;
  int distancia;
  
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(20);

  digitalWrite(TRIG, LOW);

  duracion = pulseIn(ECHO, HIGH, 260000);

  distancia = (duracion / 29.0) - (factor_correccion); //58.0;    // distancia medida en centimetros 58.2

  delay(50);       // demora entre datos
  return distancia;
}

void modoConfiguracionL() {
  //lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("* CONFIGURACION *");
  lcd.setCursor(0, 1);
  lcd.print("Largo: ");

  lcd.setCursor(0, 3);
  lcd.print("  M    ");
  lcd.setCursor(7, 3);
  lcd.write((byte)0);
  lcd.setCursor(12, 3);
  lcd.print("-    +");

  int miLargo = 0;
  char largoTxt[8];

  EEPROM.get(EE_LARGO, miLargo);
  if (miLargo == -1 || miLargo < 40) {
    miLargo = 100;
  }

  sprintf(largoTxt, "%02d cm", miLargo);
  lcd.setCursor(0, 2);
  lcd.print(largoTxt);
  delay(800);
  while (modo == M_CONFIGURACION_LARGO) {
    if (flancoSubida(BTN_UP)) {
      if (miLargo < 3000) {
        miLargo++;

        notaMas();

        sprintf(largoTxt, "%02d cm", miLargo);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(largoTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miLargo > 0) {
        miLargo--;

        notaMenos();

        sprintf(largoTxt, "%02d cm", miLargo);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(largoTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(EE_LARGO, miLargo);
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      notaAdelante();
      delay(500);
      largo = miLargo;
      modo = M_CONFIGURACION_ANCHO;
    }

  }
}

void modoConfiguracionA() {
  //lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("* CONFIGURACION *");
  lcd.setCursor(0, 1);
  lcd.print("Ancho: ");

  lcd.setCursor(0, 3);
  lcd.print("  M    ");
  lcd.setCursor(7, 3);
  lcd.write((byte)0);
  lcd.setCursor(12, 3);
  lcd.print("-    +");

  int miAncho = 0;
  char anchoTxt[8];

  EEPROM.get(EE_ANCHO, miAncho);
  if (miAncho == -1 || miAncho < 40) {
    miAncho = 100;
  }

  sprintf(anchoTxt, "%02d cm", miAncho);
  lcd.setCursor(0, 2);
  lcd.print(anchoTxt);
  delay(500);
  while (modo == M_CONFIGURACION_ANCHO) {
    if (flancoSubida(BTN_UP)) {
      if (miAncho < 3000) {
        miAncho++;

        notaMas();

        sprintf(anchoTxt, "%02d cm", miAncho);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(anchoTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miAncho > 0) {
        miAncho--;

        notaMenos();

        sprintf(anchoTxt, "%02d cm", miAncho);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(anchoTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(EE_ANCHO, miAncho);
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      notaAdelante();
      delay(500);
      ancho = miAncho;
      modo = M_CONFIGURACION_ALTO_SENSOR;
    }

  }
}

void modoConfiguracionAltoSensor() {
  lcd.setCursor(2, 0);
  lcd.print("* CONFIGURACION *");
  lcd.setCursor(0, 1);
  lcd.print("Altura del Sensor: ");

  lcd.setCursor(0, 3);
  lcd.print("  M    ");
  lcd.setCursor(7, 3);
  lcd.write((byte)0);
  lcd.setCursor(12, 3);
  lcd.print("-    +");

  int miAltura = 0;
  char alturaTxt[8];

  EEPROM.get(EE_ALTURA, miAltura);
  if (miAltura == -1 || miAltura < 40) {
    miAltura = 100;
  }

  sprintf(alturaTxt, "%02d cm", miAltura);
  lcd.setCursor(0, 2);
  lcd.print(alturaTxt);
  delay(500);
  while (modo == M_CONFIGURACION_ALTO_SENSOR) {
    if (flancoSubida(BTN_UP)) {
      if (miAltura < 600) {
        miAltura++;

        notaMas();

        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miAltura > 40) {
        miAltura--;

        notaMenos();

        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(EE_ALTURA, miAltura);
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      notaAdelante();
      delay(500);
      altura = miAltura;
      modo = M_CONFIGURACION_ALTO_AGUA;
    }

  }
}

void modoConfiguracionAltoAgua() {
  lcd.setCursor(2, 0);
  lcd.print("* CONFIGURACION *");
  lcd.setCursor(0, 1);
  lcd.print("Altura Maxima Agua: ");

  lcd.setCursor(0, 3);
  lcd.print("  M    ");
  lcd.setCursor(7, 3);
  lcd.write((byte)0);
  lcd.setCursor(12, 3);
  lcd.print("-    +");

  int miAltura = 0;
  char alturaTxt[8];

  EEPROM.get(EE_ALTURA_AGUA, miAltura);
  if (miAltura == -1 || miAltura < 40) {
    miAltura = 100;
  }

  sprintf(alturaTxt, "%02d cm", miAltura);
  lcd.setCursor(0, 2);
  lcd.print(alturaTxt);
  delay(500);
  while (modo == M_CONFIGURACION_ALTO_AGUA) {
    if (flancoSubida(BTN_UP)) {
      if (miAltura < altura) {
        miAltura++;

        notaMas();

        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miAltura > 0) {
        miAltura--;

        notaMenos();

        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(EE_ALTURA_AGUA, miAltura);
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      notaAdelante();
      delay(500);

      altura_max_agua = miAltura;
      delay(100);
      modo = M_CONFIGURACION_FACTOR_CORRECCION;
    }

  }
}


void modoConfiguracionFactorCorreccion() {
  lcd.setCursor(2, 0);
  lcd.print("* CONFIGURACION *");
  lcd.setCursor(0, 1);
  lcd.print("Factor de Correccion: ");

  lcd.setCursor(0, 3);
  lcd.print("  M    ");
  lcd.setCursor(7, 3);
  lcd.write((byte)0);
  lcd.setCursor(12, 3);
  lcd.print("-    +");

  int miFactor = 0;
  char factorTxt[8];

  EEPROM.get(EE_FACTOR_CORRECCION, miFactor);
  if (miFactor < -20 || miFactor > 20) {
    miFactor = 0;
  }

  sprintf(factorTxt, "%02d cm", miFactor);
  lcd.setCursor(0, 2);
  lcd.print(factorTxt);
  delay(500);
  while (modo == M_CONFIGURACION_FACTOR_CORRECCION) {
    if (flancoSubida(BTN_UP)) {
      if (miFactor < 20) {
        miFactor++;

        notaMas();

        sprintf(factorTxt, "%02d cm", miFactor);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(factorTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miFactor > -20) {
        miFactor--;

        notaMenos();

        sprintf(factorTxt, "%02d cm", miFactor);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(factorTxt);
      } else {
        notaError();
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(EE_FACTOR_CORRECCION, miFactor);
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      notaAdelante();
      delay(500);
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(5, 3);
      lcd.print("Saliendo!");
      notaFinal();

      factor_correccion = miFactor;
      delay(100);
      modo = M_MONITOR;
    }

  }
}

uint8_t flancoSubida(int btn) {
  uint8_t valorNuevo = digitalRead(boton[btn]);//recupera el valor del estado del btn
  uint8_t resultado = estado_boton[btn] != valorNuevo && valorNuevo == 1;
  estado_boton[btn] = valorNuevo;

  return resultado;
}

void efectoInicial() {
  //EFECTO DE INICIANDO
  lcd.clear();
  float l = (float)largo / 100, a = (float)ancho / 100, hs = (float)altura / 100, ha = (float)altura_max_agua / 100;

  for (int j = 0; j < 3; j++) {
    if (j == 0) {
      lcd.setCursor(5, 0); lcd.print("INICIANDO");
    } else if (j == 1) {
      lcd.setCursor(0, 0);
      lcd.print("                    ");
      lcd.setCursor(5, 0); lcd.print("LEER DATOS");
      lcd.setCursor(0, 2); lcd.print("l ="); lcd.print(l); lcd.print("m  a ="); lcd.print(a); lcd.print("m");
      lcd.setCursor(0, 3); lcd.print("hs="); lcd.print(hs); lcd.print("m  ha="); lcd.print(ha); lcd.print("m");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("                    ");
      lcd.setCursor(4, 0); lcd.print("CARGAR DATOS");
      lcd.setCursor(0, 2); lcd.print("l ="); lcd.print(l); lcd.print("m  a ="); lcd.print(a); lcd.print("m");
      lcd.setCursor(0, 3); lcd.print("hs="); lcd.print(hs); lcd.print("m  ha="); lcd.print(ha); lcd.print("m");
    }
    for (int k = 0; k < 16; k++) {

      lcd.setCursor(k + 2, 1); lcd.print("*");
      delay(30);
    }

    delay(200);

    lcd.setCursor(0, 1);
    lcd.print("                    ");

  }

  notaFinal();

  lcd.clear();
  //FIN DEL EFECTO INICIANDO
}

void mostrarCalculando() {
  lcd.createChar(2, carga1);
  lcd.createChar(3, carga2);
  lcd.createChar(4, carga3);
  lcd.createChar(5, carga4);

  for (int i = 2; i <= 5; i++) {
    lcd.setCursor(19, 2);
    lcd.write((byte)i);
    delay(120);
  }
  lcd.setCursor(19, 2);
  lcd.print(" ");
}

void mostrarSubiendo() {
  lcd.createChar(2, sube1);
  lcd.createChar(3, sube2);
  lcd.createChar(4, sube3);

  for (int i = 2; i <= 4; i++) {
    lcd.setCursor(19, 2);
    lcd.write((byte)i);
    delay(120);
  }
  lcd.setCursor(19, 2);
  lcd.print(" ");
}

void mostrarBajando() {
  lcd.createChar(2, baja1);
  lcd.createChar(3, baja2);
  lcd.createChar(4, baja3);

  for (int i = 2; i <= 4; i++) {
    lcd.setCursor(19, 2);
    lcd.write((byte)i);
    delay(120);
  }
  lcd.setCursor(19, 2);
  lcd.print(" ");
}

void notaError() {
  int melodia[] = {
    NOTE_B2
  };

  int duracionDeNotas[] = {
    10
  };
  for (int estaNota = 0; estaNota < 5; estaNota++) {
    int duracionNota = 1000 / duracionDeNotas[estaNota] / 2.2;
    tone(BUZZER, melodia[estaNota], duracionNota);
    int pausaEntreNotas = duracionNota * 1.30;
    delay(pausaEntreNotas);
    noTone(BUZZER);
  }
}

void notaMas() {
  int melodia[] = {
    NOTE_A5
  };

  int duracionDeNotas[] = {
    10
  };
  for (int estaNota = 0; estaNota < 2; estaNota++) {
    int duracionNota = 1000 / duracionDeNotas[estaNota] / 2.2;
    tone(BUZZER, melodia[estaNota], duracionNota);
    int pausaEntreNotas = duracionNota * 1.30;
    delay(pausaEntreNotas);
    noTone(BUZZER);
  }
}

void notaMenos() {
  int melodia[] = {
    NOTE_F4
  };

  int duracionDeNotas[] = {
    10
  };
  for (int estaNota = 0; estaNota < 2; estaNota++) {
    int duracionNota = 1000 / duracionDeNotas[estaNota] / 2.2;
    tone(BUZZER, melodia[estaNota], duracionNota);
    int pausaEntreNotas = duracionNota * 1.30;
    delay(pausaEntreNotas);
    noTone(BUZZER);
  }
}

void notaAtras() {
  int melodia[] = {
    NOTE_E5, NOTE_G4, NOTE_F4
  };

  int duracionDeNotas[] = {
    10, 10, 3
  };
  for (int estaNota = 0; estaNota < 3; estaNota++) {
    int duracionNota = 1000 / duracionDeNotas[estaNota] / 2.2;
    tone(BUZZER, melodia[estaNota], duracionNota);
    int pausaEntreNotas = duracionNota * 1.30;
    delay(pausaEntreNotas);
    noTone(BUZZER);
  }
}

void notaAdelante() {
  int melodia[] = {
    NOTE_F4, NOTE_G4, NOTE_E5
  };

  int duracionDeNotas[] = {
    10, 10, 3
  };

  for (int estaNota = 0; estaNota < 3; estaNota++) {
    int duracionNota = 1000 / duracionDeNotas[estaNota] / 2.2;
    tone(BUZZER, melodia[estaNota], duracionNota);
    int pausaEntreNotas = duracionNota * 1.30;
    delay(pausaEntreNotas);
    noTone(BUZZER);
  }
}

void notaFinal() {
  int melodia[] = {
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
  };

  int duracionDeNotas[] = {
    4, 8, 8, 4, 4, 4, 4, 4
  };

  for (int estaNota = 0; estaNota < 8; estaNota++) {
    int duracionNota = 1000 / duracionDeNotas[estaNota] / 2.4;
    tone(BUZZER, melodia[estaNota], duracionNota);
    int pausaEntreNotas = duracionNota * 1.30;
    delay(pausaEntreNotas);
    noTone(BUZZER);
  }
}

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



//DEFINICIONES PARA SENSOR ULTRASONICO
#define VCC_SENSOR A6
#define TRIG A7 //Trigger en pin A7
#define ECHO 10 //Echo en pin 10

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

#define ALTURA_MAXIMA 400;

volatile int modo = M_MONITOR; //Modo actual

//VARIABLES
int dist = 0;
unsigned int altura = 100, largo = 100, ancho = 100, altura_max_agua = 0;





void setup() {

  //Inicialización de pines del sensor
  pinMode(VCC_SENSOR, OUTPUT);//Pin de voltaje para el sensor +5v
  digitalWrite(VCC_SENSORC, HIGH); //+5v activo
  pinMode(TRIG, OUTPUT);  // trigger como salida
  pinMode(ECHO, INPUT);    // echo como entrada

  //Inicialización de pines de los LEDs
  pinMode(LED_AMARILLO, OUTPUT);   // LED para bomba
  pinMode(LED_VERDE, OUTPUT);   // LED para marcar que esta entrando agua desde el proveedor

  //Botones del menú en modo PULLUP
  pinMode(boton[BTN_MODO], INPUT_PULLUP);
  pinMode(boton[BTN_OK], INPUT_PULLUP);
  pinMode(boton[BTN_DWN], INPUT_PULLUP);
  pinMode(boton[BTN_UP], INPUT_PULLUP);



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
  EEPROM.get(0, largo);
  EEPROM.get(8, ancho);
  EEPROM.get(16, altura);
  EEPROM.get(24, altura_max_agua);

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
  int altura_agua_cm = 0;
  float altura_agua_m =  0.00;
  float largo_m = 0.00;
  float ancho_m = 0.00;
  float altura_maxima_agua_m = 0.00;
  float porcentaje_agua = 0.00;
  int p_a = 0;

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

    float vol_agua_barriles = (float)vol_agua_m * 6.29;
    float vol_agua_galones = (float)vol_agua_barriles * 42;

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


    //lcd.setCursor(0, 3);
    //lcd.print("  M    G    -    +");
    delay(500);
  }
}

void modoLlenando() {

  lcd.setCursor(4, 0);
  lcd.print("* LLENANDO *");

  //unsigned long t = millis();
  H = 0;
  M = 0;
  S = 0;

  while (modo == M_LLENADO) {
    //mostrarDistancia();
    tiempoPre = millis();
    mostrarTiempo();
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

void mostrarTiempo() {
  //manejo de tiempo
  //tiempoPre = millis();
  if (tiempoPre - tiempoPas > 999) {
    S++;
    tiempoPas = tiempoPre;
  }

  M += S / 60;
  S = S % 60;
  H += M / 60;
  M = M % 60;
  H = H % 24;

  sprintf(hora, "%02d:%02d:%02d", H, M, S);

  //lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print("TIEMPO =");

  lcd.setCursor(9, 2);
  lcd.print(hora);

}

int obtenerDistancia() {
  int duracion;
  int distancia;

  digitalWrite(TRIG, HIGH);     // generacion del pulso a enviar
  delay(1);       // al pin conectado al trigger
  digitalWrite(TRIG, LOW);    // del sensor

  duracion = pulseIn(ECHO, HIGH);  // con funcion pulseIn se espera un pulso
  // alto en Echo
  distancia = duracion / 58.0;    // distancia medida en centimetros 58.2
  delay(300);       // demora entre datos
  return distancia;
}

void modoConfiguracionL() {
  //lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("* CONFIGURACION *");
  lcd.setCursor(0, 1);
  lcd.print("Largo: ");

  lcd.setCursor(0, 3);
  lcd.print("  M    G    -    +");

  int miLargo = 0;
  char largoTxt[8];
  EEPROM.get(0, miLargo);
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
        sprintf(largoTxt, "%02d cm", miLargo);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(largoTxt);
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miLargo > 0) {
        miLargo--;
        sprintf(largoTxt, "%02d cm", miLargo);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(largoTxt);
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(0, miLargo);
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      delay(1000);
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
  lcd.print("  M    G    -    +");

  int miAncho = 0;
  char anchoTxt[8];
  EEPROM.get(8, miAncho);
  if (miAncho == -1 || miAncho < 40) {
    miAncho = 100;
  }

  sprintf(anchoTxt, "%02d cm", miAncho);
  lcd.setCursor(0, 2);
  lcd.print(anchoTxt);
  delay(800);
  while (modo == M_CONFIGURACION_ANCHO) {
    if (flancoSubida(BTN_UP)) {
      if (miAncho < 3000) {
        miAncho++;
        sprintf(anchoTxt, "%02d cm", miAncho);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(anchoTxt);
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miAncho > 0) {
        miAncho--;
        sprintf(anchoTxt, "%02d cm", miAncho);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(anchoTxt);
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(8, miAncho);
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      delay(1000);
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
  lcd.print("  M    G    -    +");

  int miAltura = 0;
  char alturaTxt[8];
  EEPROM.get(16, miAltura);
  if (miAltura == -1 || miAltura < 40) {
    miAltura = 100;
  }

  sprintf(alturaTxt, "%02d cm", miAltura);
  lcd.setCursor(0, 2);
  lcd.print(alturaTxt);
  delay(800);
  while (modo == M_CONFIGURACION_ALTO_SENSOR) {
    if (flancoSubida(BTN_UP)) {
      if (miAltura < 600) {
        miAltura++;
        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miAltura > 40) {
        miAltura--;
        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(16, miAltura);
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      delay(1000);
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
  lcd.print("  M    G    -    +");

  int miAltura = 0;
  char alturaTxt[8];
  EEPROM.get(24, miAltura);
  if (miAltura == -1 || miAltura < 40) {
    miAltura = 100;
  }

  sprintf(alturaTxt, "%02d cm", miAltura);
  lcd.setCursor(0, 2);
  lcd.print(alturaTxt);
  delay(800);
  while (modo == M_CONFIGURACION_ALTO_AGUA) {
    if (flancoSubida(BTN_UP)) {
      if (miAltura < altura) {
        miAltura++;
        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      }
    }

    if (flancoSubida(BTN_DWN)) {
      if (miAltura > 0) {
        miAltura--;
        sprintf(alturaTxt, "%02d cm", miAltura);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print(alturaTxt);
      }
    }

    if (flancoSubida(BTN_OK)) {
      EEPROM.put(24, miAltura);
      lcd.setCursor(5, 3);
      lcd.print("Guardado!");
      delay(1000);
      altura_max_agua = miAltura;
      modo = M_MONITOR;
    }

  }
}

uint8_t flancoSubida(int btn) {
  uint8_t valorNuevo = digitalRead(boton[btn]);//recupera el valor del estado del btn
  //Serial.println(valorNuevo);
  uint8_t resultado = estado_boton[btn] != valorNuevo && valorNuevo == 1;
  //Serial.println(resultado);
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
      delay(50);
    }

    delay(300);

    lcd.setCursor(0, 1);
    lcd.print("                    ");
    //lcd.clear();
  }
  delay(1000);

  /* for (int k = 0; k < 4; k++){
     lcd.noDisplay(); delay(300); lcd.display(); delay(300);
    }*/

  lcd.clear();
  //FIN DEL EFECTO INICIANDO
}

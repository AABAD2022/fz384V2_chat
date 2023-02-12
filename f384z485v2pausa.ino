

// MODIFICACION ARDUINO MAXI
#include <Adafruit_NeoPixel.h>
#include <Controllino.h>
#include <AccelStepper.h>
#include <SPI.h>
#include <Ethernet.h>
#include <millisDelay.h>


byte MAC_ADDRESS[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress IP(192, 168, 0, 100);
IPAddress DNS(192, 168, 0, 1);
IPAddress GW(192, 168, 0, 1);
IPAddress SNET(255, 255, 255, 0);
EthernetServer SERVER(3000);
EthernetClient CLIENT;


int LOGIC = 1;   /* SIRVE PARA INVERTIR LA LOGICA DE LOS SENSORES */
int SEN_VCC = 0;
int SEN_SECURE = 1;
int SEN_CONVEYOR = 0;
int SEN_DOR_RIGHT = 0;
int SEN_DOR_LEFT = 0;
int SEN_DRAWER = 0;
int SEN_TRAY = 1;
int SEN_TROLLEY_1 = 0;
int SEN_TROLLEY_2 = 0;
int SEN_TROLLEY_3 = 0;
int SEN_TROLLEY_4 = 0;
int SEN_BLISTER_EXIT = 0;

byte SEN_STATE[3];
// Puertos Sensores
int VCC_SEN_PIN = CONTROLLINO_A0;           // sensor maquina encendida o apagada
int SECURE_SEN_PIN = CONTROLLINO_A1;        // sensor seta emergencia
int CONVEYOR_SEN_PIN = CONTROLLINO_A2;      // SENSOR CORREA a sub_board
int DOR_RIGHT_SEN_PIN = CONTROLLINO_A3;     // sensor puerta derecha   // z80 viejas int DOR_RIGHT_SEN_PIN    = CONTROLLINO_A8;
int DOR_LEFT_SEN_PIN = CONTROLLINO_A4;      // sensor puerta Izquierda // z80 viejas int DOR_LEFT_SEN_PIN     = CONTROLLINO_A7;
int DRAWER_SEN_PIN = CONTROLLINO_A5;        // sensor cajon intermedio
int TRAY_SEN_PIN = CONTROLLINO_A6;          // sensor bandeja en posición
int TROLLEY_1_SEN_PIN = CONTROLLINO_A7;     // sensor carro_1
int TROLLEY_2_SEN_PIN = CONTROLLINO_A8;     // sensor carro_2
int TROLLEY_3_SEN_PIN = CONTROLLINO_A9;     // sensor carro_3
int TROLLEY_4_SEN_PIN = CONTROLLINO_IN0;    // sensor carro_4
int BLISTER_EXIT_SEN_PIN = CONTROLLINO_IN1; // sensor SALIDA BLISTER

// Puertos salida reles
int LOCK_TROLLEY_1_PIN = CONTROLLINO_R0;     // cerradura carro_1
int LOCK_TROLLEY_2_PIN = CONTROLLINO_R1;     // cerradura carro_2
int LOCK_TROLLEY_3_PIN = CONTROLLINO_R2;     // cerradura carro_3
int LOCK_TROLLEY_4_PIN = CONTROLLINO_R3;     // cerradura carro_4
int LOCK_DRAWER_PIN = CONTROLLINO_R4;        // cerradura cajon intermedio
int LOCK_TRAY_PIN = CONTROLLINO_R5;          // cerradura bandeja
int POWERBTN_SELLADORA_5V = CONTROLLINO_R6;  // encendido sub board selladora 5V
int POWERBTN_SELLADORA_24V = CONTROLLINO_R7; // encendido sub board selladora 24v
int CONVEYOR_PIN = CONTROLLINO_R8;           // CINTA TRASPORTADORA conveyor
int EMG_DRIVERS_PIN = CONTROLLINO_R9;        // Paro emergencia drivers cartesianos

// Puertos salidas digitales
int LED_TROLLEY = 42; // leds semaforo puertas CASA 12
int R;
int G;
int B;
int SECUENCIA_ON;//variable encendido
int CARRO_ABIERTO = 1;//variable carro cerrado

const int COUNT = 32; // Total numero de leds  
const int LEDCARRO = 8; //numero leds por tira
const int TIEMPOENTRELED = 3;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(COUNT, LED_TROLLEY, NEO_GRB + NEO_KHZ800); //define las propiedades de los leds
// Variables
byte COUNT_DPS = 0;


// int MODEL = 0; // 0 dosculumnas , 1 solo columna interior , 2 solo columna exterior

int TIMEOUT = 300;
long LAST_READ = 0;
byte BUFF_READ[100];
int BUFF_N = 0;
//int BUFF_L = 0;
int BUFF_LEN = 6;
int MODE_R = 0;
const char STX = 0x02;
const char ETX = 0x03;
const char CMD_OK = 0x05;
const char CMD_KO = 0x06;
int CARRO_NX = 0;
int TIEMPO_PLUS_CORREA = 2000; // tiempo extra cinta trasportadora
unsigned long previousMillis = 0;
const long interval = 5000;

millisDelay delayCORREA;
void setup()
{
  CONFIG();   
  Ethernet.begin(MAC_ADDRESS, IP, DNS, GW, SNET);
  SERVER.begin();
  Serial.begin(57600);
  Serial3.begin(57600);
  Controllino_RS485Init();        //Initialize CONTROLLINO RS485 direction control DE/RE pins and Serial3
  Controllino_RS485TxEnable();
  
  digitalWrite(LOCK_TROLLEY_1_PIN, 0);
  digitalWrite(LOCK_TROLLEY_2_PIN, 0);
  digitalWrite(LOCK_TROLLEY_3_PIN, 0);
  digitalWrite(LOCK_TROLLEY_4_PIN, 0);
  digitalWrite(LOCK_DRAWER_PIN, 0);
  digitalWrite(LOCK_TRAY_PIN, 0);
  digitalWrite(POWERBTN_SELLADORA_5V, 0);
  digitalWrite(POWERBTN_SELLADORA_24V, 0);
  digitalWrite(EMG_DRIVERS_PIN, 1);
  strip.begin(); // iniciar la tira
  strip.show(); // poner la tira en negro (porque aún no le hemos dicho que haga nada)
  
  ARRANCA_SELLADORA();
  //doShow();
    Serial3.print("Conectado a RS485  ");
  //Serial.print("Server started at ");
  //Serial.println(Ethernet.localIP());
  //Serial.println(STX);
    Serial3.flush();
}
void CLEAR_DATA(long TIME__){
   if((long)(LAST_READ+ TIMEOUT) < TIME__ && BUFF_N>0){  
      memset(BUFF_READ, 0, sizeof BUFF_READ);
      BUFF_N= 0;   
   }
}

void loop() {
 unsigned long TIME_ =  micros();
 CLIENT = SERVER.available();
 Controllino_RS485RxEnable();
 SEN_VCC = READ_PIN(VCC_SEN_PIN);
 if (SEN_VCC == 0){
   SECUENCIA_ON = 1;
   ARRANCA_SELLADORA();
  

 }
   else
   {
    if(SECUENCIA_ON == 1){
        ARRANCA_SELLADORA();

       // Serial.println(SECUENCIA_ON);
    }
    }

 
 if (BUFF_N > BUFF_LEN) {
        Controllino_RS485TxEnable();
      

     if (BUFF_READ[0] == STX) {      
       if (BUFF_READ[5] == ETX && BUFF_READ[6] == Checksums(BUFF_READ, BUFF_LEN + 1, 1)) {
        /*
        if (BUFF_READ[1] == 0x30) {                              //02 30 00 00 00 03 Chk 33
          RESET_COLUMS();
          //Serial.print(MODE_R);
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 }; 
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);        
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 3){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }

       */
        /*
        if (BUFF_READ[1] == 0x01 ) {
          byte by = (byte)(MAX_ENC_EXT >> 16 & 0xFF);
          byte by1 = (byte)(MAX_ENC_EXT >> 8 & 0xFF);
          byte by2 = (byte)(MAX_ENC_EXT >> 0 & 0xFF);
          byte DATA_OUT[] =  { STX,CMD_OK,BUFF_READ[1],by,by1,by2,ETX,0 };
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        if (BUFF_READ[1] == 0x02) {
          byte by = (byte)(MAX_ENC_IN >> 16 & 0xFF);
          byte by1 = (byte)(MAX_ENC_IN >> 8 & 0xFF);
          byte by2 = (byte)(MAX_ENC_IN >> 0 & 0xFF);
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],by,by1,by2,ETX,0 };
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*        if (BUFF_READ[1] == 0x36 ) {  //02 36 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 36 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          DTA_REFIL();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x37 ) {  //02 37 00 00 00 03 3AChk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 37 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          //LEVA_RESET();
          //ARRANCA_SELLADORA();
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 3){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x38 ) {  //02 38 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 38 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          DTA_CLOSE();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}

        }*/
        /*
        if (BUFF_READ[1] == 0x39 ) {  //02 39 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],BUFF_READ[2],0x00,ETX,0 };    //02 05 39 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          TRAY_OPEN();
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 3){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x21 ) {  //02 21 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 21 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          SATEL();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        if (BUFF_READ[1] == 0x22) {  //02 22 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 22 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          SATEL();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
          if (BUFF_READ[1] == 0x3D ) {  //02 3D 00 00 00 03 Chk
          SENSOR_READ();
          GET_SENSORS();          
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],SEN_STATE[0],SEN_STATE[1],SEN_STATE[2],SEN_STATE[3],ETX,0 };  
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 1){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        /*
        if (BUFF_READ[1] == 0x60 ) {  //02 60 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],BUFF_READ[2],BUFF_READ[3],BUFF_READ[3],ETX,0 };    //02 05 60 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          //byte data_Stepp[] = { BUFF_READ[4],BUFF_READ[3],BUFF_READ[2] };
          //long STEP  = (long)toInt(data_Stepp); // (long)((data[3] & 0xFF) << 0) + (long)((data[2] & 0xFF) << 8 ) + (long)((data[1] & 0xFF) << 16);
          //MOTOR_EXT.runToNewPosition((long)STEP);
          //ENC_EXT = STEP;
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 3){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x61 ) {  //02 61 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],BUFF_READ[2],BUFF_READ[3],ETX,0 };    //02 05 61 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          byte data_Stepp[] = { BUFF_READ[4],BUFF_READ[3],BUFF_READ[2] };
          long STEP  = (long)toInt(data_Stepp); //(long)((data[3] & 0xFF) << 0) + (long)((data[2] & 0xFF) << 8 ) + (long)((data[1] & 0xFF) << 16);
          MOTOR_IN.runToNewPosition((long)STEP);
          ENC_IN = STEP;
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x20 ) {  //02 20 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 20 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          TRAY_RESET();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x4F) {  //02 4F 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 4F 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          MOVE_DTA((int)((int)( BUFF_READ[2] * 0xFFFF ) +(int)( BUFF_READ[3] * 0xFF ) + (int)BUFF_READ[4])); //ubico_X_celda();
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 3){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        /*
        if (BUFF_READ[1] == 0x6F) {  //02 6F 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 6F 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          LEVA_RESET();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        if (BUFF_READ[1] == 0x70) {  //02 70 01 00 03 74
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],BUFF_READ[2],BUFF_READ[3],ETX,0 };    //02 05 70 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          MOVE_LEVA((int)((int)( BUFF_READ[2] * 0xFFFF ) +(int)( BUFF_READ[3] * 0xFF ) + (int)BUFF_READ[4])); // ubico_Y_celda();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }

        if (BUFF_READ[1] == 0x11 ) {  //02 11 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 11 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          MAGNET_ON();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        if (BUFF_READ[1] == 0x12 ) {  //02 12 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],BUFF_READ[2],0x00,ETX,0 };    //02 05 12 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          CARRO_NX = (BUFF_READ[2]);
          //MAGNET_OFF();
          //LED_CARROS();
          ABRIR_CARROS();
          if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 1){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));
          }
        }
        /*
        if (BUFF_READ[1] == 0x13 ) {  //02 13 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 13 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          SOLENOID_ON();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        if (BUFF_READ[1] == 0x14 ) {  //02 14 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 05 14 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          SOLENOID_OFF();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        if (BUFF_READ[1] == 0x71 ) {  //02 71 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 71 00 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          PISTON_ON();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
         if (BUFF_READ[1] == 0x72 ) {  //02 72 00 00 00 03 Chk
          byte DATA_OUT[] = { STX,CMD_OK,BUFF_READ[1],0x00,ETX,0 };    //02 72 00 00 00 03 Chk
          DATA_OUT[sizeof(DATA_OUT)-1] = Checksums(DATA_OUT, sizeof(DATA_OUT),1);
          PISTON_OFF();
          if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
          if (MODE_R == 2){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        }
        */
        memset(BUFF_READ, 0, sizeof BUFF_READ);
        BUFF_N= 0;
        
      }
       else {// Zona No Checksum Okey
        byte DATA_OUT[] = { STX,CMD_KO,Checksums(BUFF_READ, BUFF_LEN, 1),0x00,ETX,0 };
        DATA_OUT[5] = Checksums(DATA_OUT, BUFF_LEN - 1,1);
        if (MODE_R == 1){ Serial.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        if (MODE_R == 1){ SERVER.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        if (MODE_R == 1){ Serial3.write((uint8_t*)DATA_OUT, sizeof(DATA_OUT));}
        CLEAR_DATA(TIME_);   
       }
           Serial3.flush();


     }

  }
  if (Serial3.available()>0){ 
    BUFF_READ[BUFF_N] = Serial3.read();          
    LAST_READ= TIME_;   
    BUFF_N++;
    MODE_R=1; //SERIE
  }
 if (CLIENT) {  
    if (CLIENT.available() > 0) {
      BUFF_READ[BUFF_N] = CLIENT.read();  
      LAST_READ= TIME_;   
      BUFF_N++;        
      MODE_R=2; //ETH
    }
  }
    
  if (Serial.available()>0){ 
    BUFF_READ[BUFF_N] = Serial.read();          
    LAST_READ= TIME_;   
    BUFF_N++;
    MODE_R=1; //SERIE
  }
  CLEAR_DATA(TIME_);
  //IS_POWER_OFF();
  // DTA_Button();
  CORREA_salida();     // llamada sub rutina mover correa salida
  //ARRANCA_SELLADORA(); // SENSOR ENCENDIDO
  //LOKEY_DISCONECT();
  CARROS_OPEN_SEN();
  Paro_Emg_cartesianos(); //Sub rutina paro emergencia blister

 
  
  
}

void SENSOR_READ()
{
  SEN_VCC = READ_PIN(VCC_SEN_PIN);
  SEN_SECURE = READ_PIN(SECURE_SEN_PIN);
  SEN_CONVEYOR = READ_PIN(CONVEYOR_SEN_PIN);
  SEN_DOR_RIGHT = READ_PIN(DOR_RIGHT_SEN_PIN);
  SEN_DOR_LEFT = READ_PIN(DOR_LEFT_SEN_PIN);
  SEN_DRAWER = READ_PIN(DRAWER_SEN_PIN);
  SEN_TRAY = READ_PIN(TRAY_SEN_PIN);
  SEN_TROLLEY_1 = READ_PIN(TROLLEY_1_SEN_PIN);
  SEN_TROLLEY_2 = READ_PIN(TROLLEY_2_SEN_PIN);
  SEN_TROLLEY_3 = READ_PIN(TROLLEY_3_SEN_PIN);
  SEN_TROLLEY_4 = READ_PIN(TROLLEY_4_SEN_PIN);
  SEN_BLISTER_EXIT = READ_PIN(BLISTER_EXIT_SEN_PIN);
  
}

void CONFIG()
{
  pinMode(VCC_SEN_PIN, INPUT);
  pinMode(SECURE_SEN_PIN, INPUT);
  pinMode(CONVEYOR_SEN_PIN, INPUT);
  pinMode(DOR_RIGHT_SEN_PIN, INPUT);
  pinMode(DOR_LEFT_SEN_PIN, INPUT);
  pinMode(DRAWER_SEN_PIN, INPUT);
  pinMode(TRAY_SEN_PIN, INPUT);
  pinMode(TROLLEY_1_SEN_PIN, INPUT);
  pinMode(TROLLEY_2_SEN_PIN, INPUT);
  pinMode(TROLLEY_3_SEN_PIN, INPUT);
  pinMode(TROLLEY_4_SEN_PIN, INPUT);
  pinMode(BLISTER_EXIT_SEN_PIN, INPUT);

  pinMode(LOCK_TROLLEY_1_PIN, OUTPUT);
  pinMode(LOCK_TROLLEY_2_PIN, OUTPUT);
  pinMode(LOCK_TROLLEY_3_PIN, OUTPUT);
  pinMode(LOCK_TROLLEY_4_PIN, OUTPUT);
  pinMode(LOCK_DRAWER_PIN, OUTPUT);
  pinMode(LOCK_TRAY_PIN, OUTPUT);
  pinMode(POWERBTN_SELLADORA_5V, OUTPUT);
  pinMode(POWERBTN_SELLADORA_24V, OUTPUT);
  pinMode(CONVEYOR_PIN, OUTPUT);
  pinMode(EMG_DRIVERS_PIN, OUTPUT);
  DDRD = DDRD | B01110000;
}

int READ_PIN(int PIN)
{
  int E_PIN = digitalRead(PIN);
  if (LOGIC == 0)
  {
    if (E_PIN == 1)
    {
      E_PIN = 0;
    }
    else
    {
      E_PIN = 1;
    }
  }
  return E_PIN;
}

void DTA_Button()
{
  /*BTN_DTA = READ_PIN(DTA_BTN_PIN);
  if (BTN_DTA == 0) {
     digitalWrite(DTA_LED_PIN, 1);
    int CLOSE = READ_PIN(DTA_SEN_CLOSE);
    if (CLOSE == 0) {  //clapes en 1
       DTA_OPEN();
     }else {
       DTA_CLOSE();
    }
     digitalWrite(DTA_LED_PIN, 0);
  }
 */
}




void CORREA_salida()
{
SEN_CONVEYOR = READ_PIN(CONVEYOR_SEN_PIN);

if (SEN_CONVEYOR == 1) {
digitalWrite(CONVEYOR_PIN, 1);
previousMillis = millis();
} else {
unsigned long currentMillis = millis();
if (currentMillis - previousMillis < interval) {
digitalWrite(CONVEYOR_PIN, 1);
} else {
digitalWrite(CONVEYOR_PIN, 0);
}
}
}






void Paro_Emg_cartesianos()//subrutina paro emergencia Cartesianos
{ 
    SEN_SECURE = READ_PIN(SECURE_SEN_PIN);
  // SEN_VCC = READ_PIN(VCC_SEN);
  if (SEN_SECURE == 0)
  {
   digitalWrite(EMG_DRIVERS_PIN, 0);
    // Serial.println("sale a 0");
  }
  else
  {
    //digitalWrite(CONVEYOR_PIN, 1);
    // Serial.println("sale a 1");
    //delay(TIEMPO_PLUS_CORREA);
    digitalWrite(EMG_DRIVERS_PIN, 1);
    //if (millis() - timing > 6000){ // En lugar de 1000, ajusta el valor de pausa que deses
    //timing = millis(); 
      

 }
 }
void ARRANCA_SELLADORA()
{ // subrutina arrancar selladora
  SEN_VCC = READ_PIN(VCC_SEN_PIN);

  if (SEN_VCC == 1)
  {
    SECUENCIA_ON = 0;
    doShow();

    digitalWrite(POWERBTN_SELLADORA_24V, 1);
    delay(1000);
    /*if (millis() - timing > 1000){ // En lugar de 1000, ajusta el valor de pausa que deses
    timing = millis(); 
    }*/
    digitalWrite(POWERBTN_SELLADORA_5V, 1);
    //delay(3000);
    //digitalWrite(POWERBTN_SELLADORA_5V, 0);
    //delay(2000);
    //digitalWrite(POWERBTN_SELLADORA_5V, 1);

  }
  else
  {
    digitalWrite(POWERBTN_SELLADORA_5V, 0);
    delay(500);
    digitalWrite(POWERBTN_SELLADORA_24V, 0);
    delay(500);
  }
}

void TRAY_OPEN()
{
  
  int CLOSED = READ_PIN(TRAY_SEN_PIN);
  digitalWrite(LOCK_TRAY_PIN, 1);
  while (CLOSED == 1) {
  CLOSED = READ_PIN(TRAY_SEN_PIN); 
    }
  digitalWrite(LOCK_TRAY_PIN, 0);
  /*
int OPEN = READ_PIN(DTA_SEN_OPEN);
 digitalWrite(DTA_WIRE_RED_GND, 0);
 digitalWrite(DTA_WIRE_RED_24V, 1);
 digitalWrite(DTA_WIRE_BLACK_GND, 1);
 digitalWrite(DTA_WIRE_BLACK_24V, 0);
 while (OPEN == 1) {  //clapes en 0
 OPEN = READ_PIN(DTA_SEN_OPEN);
 }
 digitalWrite(DTA_WIRE_RED_GND, 0);
 digitalWrite(DTA_WIRE_RED_24V, 0);
 digitalWrite(DTA_WIRE_BLACK_GND, 0);
 digitalWrite(DTA_WIRE_BLACK_24V, 0);
*/
}

void DTA_CLOSE()
{
  /*
  int CLOSE = READ_PIN(DTA_SEN_CLOSE);
  digitalWrite(DTA_WIRE_RED_GND, 1);
  digitalWrite(DTA_WIRE_RED_24V, 0);
  digitalWrite(DTA_WIRE_BLACK_GND, 0);
  digitalWrite(DTA_WIRE_BLACK_24V, 1);
  while (CLOSE == 1) { //clapes en 0
    CLOSE = READ_PIN(DTA_SEN_CLOSE);
  }
  digitalWrite(DTA_WIRE_RED_GND, 0);
  digitalWrite(DTA_WIRE_RED_24V, 0);
  digitalWrite(DTA_WIRE_BLACK_GND, 0);
  digitalWrite(DTA_WIRE_BLACK_24V, 0);
  */
}

void DTA_REFIL()
{
  /*
 int STEP = 7500;
  int SPEED = 200;
  int SEN = 600;
  digitalWrite(DTA_DRIVER_DIR,1);
  digitalWrite(DTA_DRIVER_ENA,0);
  SEN_DTA_REFIL = READ_PIN(DTA_SEN_REFIL) ;
  digitalWrite(DTA_DRIVER_DIR,0);
  while (SEN_DTA_REFIL == 1) { //CLAPES 0
    SEN_DTA_REFIL = READ_PIN(DTA_SEN_REFIL) ;
    MOVE_MOTOR(DTA_DRIVER_PUL,SPEED);
  }
  digitalWrite(DTA_DRIVER_DIR,1);
  for(int t =0;t<STEP;t++){
     MOVE_MOTOR(DTA_DRIVER_PUL,SPEED);
  }
  digitalWrite(DTA_DRIVER_DIR,0);
   for(int R =0;R<SEN;R++){
     MOVE_MOTOR(DTA_DRIVER_PUL,SPEED);
  }
  digitalWrite(DTA_DRIVER_DIR,1);
   for(int R =0;R<SEN;R++){
     MOVE_MOTOR(DTA_DRIVER_PUL,SPEED);
  }
    digitalWrite(DTA_DRIVER_DIR,0);
  SEN_DTA_REFIL = READ_PIN(DTA_SEN_REFIL) ;
  while (SEN_DTA_REFIL == 1) {
    SEN_DTA_REFIL = READ_PIN(DTA_SEN_REFIL) ;
    MOVE_MOTOR(DTA_DRIVER_PUL,SPEED);
  }
  for(int R =0;R<SEN;R++){
     MOVE_MOTOR(DTA_DRIVER_PUL,SPEED);
  }

 digitalWrite(DTA_DRIVER_DIR,0);
 digitalWrite(DTA_DRIVER_ENA, 1);
*/
}

void TRAY_RESET()
{
  /*
  int SPEED = 800;
  digitalWrite(TRAY_DRIVER_DIR,0);
  digitalWrite(TRAY_DRIVER_ENA,0);
  SEN_TRAY_RESET = READ_PIN(TRAY_SEN) ;
  while (SEN_TRAY_RESET == 1) {
    SEN_TRAY_RESET = READ_PIN(TRAY_SEN) ;
    MOVE_MOTOR(TRAY_DRIVER_PUL,SPEED);
  }
 digitalWrite(TRAY_DRIVER_DIR,1);
 digitalWrite(TRAY_DRIVER_ENA, 0);
 ENC_DTA = 0;
*/
}

void LEVA_RESET()
{
  /*
  int SPEED = 800;
  digitalWrite(LEVA_DRIVER_DIR,1);
  digitalWrite(LEVA_DRIVER_ENA,0);
  //SEN_LEVA_RESET = READ_PIN(DTA_SEN_LEVA) ;
  while (SEN_LEVA_RESET == 1) {
 //   SEN_LEVA_RESET = READ_PIN(DTA_SEN_LEVA) ;
    MOVE_MOTOR(LEVA_DRIVER_PUL,SPEED);
  }
 digitalWrite(LEVA_DRIVER_DIR,1);
 digitalWrite(LEVA_DRIVER_ENA, 0);
 ENC_LEVA = 0;
*/
}

void COL_EXT()
{
  /*
  int ACCE_STEP = 1000;
  int ACCE_INT = 2500;
  int ACCE_END = 600;
  int MAX_ERROR_STEP = 30000;
  int TARA_INT = 0;//12300;
  int INV=0;
  MAX_ENC_EXT= COL_RESET(EX_DRIVER_PUL,EX_DRIVER_DIR,EX_DRIVER_ENA,SEN_COL_EXT,COL_EXT_SEN_PIN,TARA_INT, ACCE_STEP, ACCE_INT, ACCE_END, MAX_ERROR_STEP,INV);
  ENC_EXT = 0;
  */
}

void COL_IN()
{
  /*
  int ACCE_STEP = 350;
  int ACCE_INT = 1800;
  int ACCE_END = 600;
  int MAX_ERROR_STEP = 20000;
  int TARA_INT = 0;//5480;
  int INV=1;
  MAX_ENC_IN= COL_RESET(IN_DRIVER_PUL,IN_DRIVER_DIR,IN_DRIVER_ENA,SEN_COL_IN,COL_IN_SEN_PIN,TARA_INT, ACCE_STEP, ACCE_INT, ACCE_END, MAX_ERROR_STEP,INV);
  ENC_IN = 0;


}

int COL_RESET(int PUL,int DIR,int ENA,int SEN,int PIN,int TARA_INT,int ACCE_STEP,int ACCE_INT,int ACCE_END,int MAX_ERROR_STEP,int INV){
  int MAX_STEP = 0;
  float SPEED = 0;
  float ACCE_LINE = (float)(ACCE_INT - ACCE_END) / ACCE_STEP ;
  digitalWrite(DIR,0);
  if (INV==1) {
    if (digitalRead(DIR)== 1) {
      digitalWrite(DIR,0);
    }
    else{
      digitalWrite(DIR,1);
    }
  }
  digitalWrite(ENA,0);
  SEN = READ_PIN(PIN) ;
   for(int i =0;i<MAX_ERROR_STEP;i++){
    SEN = READ_PIN(PIN) ;
    if (SEN == 0) {
      for(int q =TARA_INT;q>=0;q--){
        if (i <= ACCE_STEP ) {   //acelerar
          SPEED =(float)ACCE_INT - (ACCE_LINE * i);
          i++;
        }
        MAX_STEP +=1;
        MOVE_MOTOR(PUL,SPEED);
      }

      for(int e =ACCE_STEP;e>=0;e--){
        MAX_STEP +=1;
        MOVE_MOTOR(PUL,SPEED);
      }

      SEN = READ_PIN(PIN) ;
      while (SEN == 1) {
        SEN = READ_PIN(PIN) ;
        MAX_STEP +=1;
        MOVE_MOTOR(PUL,SPEED);
      }

      for(int e =ACCE_STEP;e>=0;e--){
        SPEED =(float)ACCE_INT - (ACCE_LINE * e);
        MOVE_MOTOR(PUL,SPEED);
      }
      break;
    }
    if (i <= ACCE_STEP ) {   //acelerar
     SPEED =(float)ACCE_INT - (ACCE_LINE * i);
    }
    if ((MAX_ERROR_STEP -i) <= ACCE_STEP ) {   //decelerar
     SPEED =(float)ACCE_INT - (ACCE_LINE * (MAX_ERROR_STEP -i));
    }
    MOVE_MOTOR(PUL,SPEED);
  }

  digitalWrite(DIR,0);
  digitalWrite(ENA, 0);
  return MAX_STEP;
*/
}

void SATEL()
{
  /*
int Tara1 = 20;
int Tara2 = 50;
   if (MOTOR_SATEL.currentPosition()==0){
      MOTOR_SATEL.runToNewPosition(Tara2);
      MOTOR_SATEL.setCurrentPosition(1);
      delay(100);
      MOTOR_SATEL.runToNewPosition(Tara1*-1);
      exit;
   }
         if (MOTOR_SATEL.currentPosition()<=0){
          MOTOR_SATEL.runToNewPosition(Tara1);
         }else{
          MOTOR_SATEL.runToNewPosition(Tara1*-1);
         }

    COUNT_DPS = (byte)(COUNT_DPS + 1);
    COUNT_DPS = (byte)(COUNT_DPS & 0xff);
    delay(50);
*/
}

void SOLENOID_ON()
{
/*
 digitalWrite(SOLENOID_PIN, 1);
*/}

void SOLENOID_OFF()
{
  /*
 digitalWrite(SOLENOID_PIN, 0);
*/
}

void MAGNET_ON()
{
  /*
 digitalWrite(MAGNET_PIN,1);

*/
}

void MAGNET_OFF()
{
  /*
  digitalWrite(MAGNET_PIN,0);
  */
}

void PISTON_ON()
{
  /*
 digitalWrite(PISTON_PIN,1);
*/
}

void PISTON_OFF()
{
  /*
  digitalWrite(PISTON_PIN,0);
  */
}

void MOVE_MOTOR(int PUL, int SPEED)
{
  /*
    digitalWrite(PUL,1);
    delayMicroseconds(SPEED);
    digitalWrite(PUL,0);
    delayMicroseconds(SPEED);
   */
}
void GET_SENSORS()
{
  SEN_STATE[0] = 0;
  SEN_STATE[1] = 0;
  SEN_STATE[2] = 0;
  SEN_STATE[3] = 0;
  
  if (SEN_VCC == 1) { SEN_STATE[0] += 1; }  //sensor maquina encendida
  if (0 == 1) { SEN_STATE[0] += 2; }  // no utilizado
  if (SEN_SECURE == 0) { SEN_STATE[0] += 4; } //seta emergencia
  if (SEN_CONVEYOR == 1) { SEN_STATE[0] += 8; } // conveyor encendido
  if (SEN_BLISTER_EXIT == 0) { SEN_STATE[0] += 16; } //
  if (SEN_DOR_LEFT == 1) { SEN_STATE[0] += 32; } // Sensor puerta derecha
  if (SEN_DRAWER == 1) { SEN_STATE[0] += 64; } // Sensor cajon 
  if (SEN_TRAY == 0){ SEN_STATE[0] += 128; } // Sensor bandeja fuera

  if (SEN_TROLLEY_1 == 1) { SEN_STATE[1] += 1; } // Sensor Carro 1
  if (SEN_TROLLEY_2 == 1) { SEN_STATE[1] += 2; }  // Sensor Carro 2
  if (SEN_TROLLEY_3 == 1) { SEN_STATE[1] += 4; }  // Sensor Carro 3
  if (SEN_TROLLEY_4 == 1) { SEN_STATE[1] += 8; }  // sensor Carro 4
  if (SEN_BLISTER_EXIT == 1) { SEN_STATE[1] += 16; }
  if (SEN_DOR_RIGHT == 1) { SEN_STATE[1] += 32; } // Sensor puerta Derecha
  if (0 == 1) { SEN_STATE[1] += 64; }
  if (0 == 1) { SEN_STATE[1] += 128; }

  SEN_STATE[2] = 1; // VERSION CONTROLLINO
  SEN_STATE[3] = 1;//COUNT_DPS;

}

void MOVE_POS_MOTOR(int PUL, int DIR, int ENA, int STEP, int ACCE_STEP, int ENC, int ACCE_INT, int ACCE_END, int MAX_STEP, int CYCLE_STEP, int INV)
{
  /*
  float SPEED = 0;
  float ACCE_LINE = (float)(ACCE_INT - ACCE_END) / ACCE_STEP ;
  int DIR_RIGHT = (int)ENC - STEP  ;
  int DIR_LEFT  = (int)ENC + STEP  ;
  int MOVE_STEP = 0;
  if (abs(STEP-ENC)<=(MAX_STEP/2) ) {
    MOVE_STEP =abs(STEP-ENC)*CYCLE_STEP;
  }else{
     MOVE_STEP =(MAX_STEP-abs((STEP-ENC)))*CYCLE_STEP;
  }
  digitalWrite(ENA,0);
  if (abs(STEP-ENC) <= (MAX_STEP/2)) {
     if (STEP-ENC<0) {
        digitalWrite(DIR,1);
     }else{
       digitalWrite(DIR,0);
     }
  }else{
    if (STEP-ENC<0) {
        digitalWrite(DIR,0);
     }else{
        digitalWrite(DIR,1);
     }
  }

  if (INV==1) {
    if (digitalRead(DIR)== 1) {
      digitalWrite(DIR,0);
    }
    else{
      digitalWrite(DIR,1);
    }
  }

  for(int i =0;i<MOVE_STEP;i++){
    if (i <= ACCE_STEP and  i <=(MOVE_STEP/3) ) {   //acelerar 1000
     SPEED =(float)ACCE_INT - (ACCE_LINE * i);
    }
    if ((MOVE_STEP -i) <= ACCE_STEP and  (MOVE_STEP -i) <=(MOVE_STEP/3)) {   //decelerar
     SPEED =(float)ACCE_INT - (ACCE_LINE * (MOVE_STEP -i));
    }
   // Serial.println( SPEED);
    MOVE_MOTOR(PUL,SPEED);
   }
  //digitalWrite(DIR,0);
  digitalWrite(ENA,0);
*/
}

void MOVE_DTA(int STEP)
{
  /*
    float SPEED = 800;
    int MOVE = STEP - ENC_DTA;
    digitalWrite(TRAY_DRIVER_DIR,1);
    digitalWrite(TRAY_DRIVER_ENA,0);
    if (MOVE <= 0 ) {
      MOVE*=-1 ;
      digitalWrite(TRAY_DRIVER_DIR,0);
    }
    for(int i =0;i<MOVE;i++){
      MOVE_MOTOR(TRAY_DRIVER_PUL,SPEED);
    }
    digitalWrite(TRAY_DRIVER_DIR,1);
    digitalWrite(TRAY_DRIVER_ENA,0);
    ENC_DTA = STEP;
*/
}

void MOVE_LEVA(int STEP)
{
  /*
    float SPEED = 800;
    int MOVE = STEP - ENC_LEVA;
    digitalWrite(LEVA_DRIVER_DIR,1);
    digitalWrite(LEVA_DRIVER_ENA,0);
    if (MOVE <= 0 ) {
      MOVE*=-1 ;
      digitalWrite(LEVA_DRIVER_DIR,0);
    }
    for(int i =0;i<MOVE;i++){
      MOVE_MOTOR(LEVA_DRIVER_PUL,SPEED);
    }
    digitalWrite(LEVA_DRIVER_DIR,1);
    digitalWrite(LEVA_DRIVER_ENA,0);
    ENC_LEVA = STEP;
*/
}

void RESET_2COLUM()
{
  /*
  MOTOR_EXT.setMaxSpeed(2000);
  MOTOR_EXT.setAcceleration(2000);
  MOTOR_EXT.setSpeed(10);
  MOTOR_IN.setMaxSpeed(2000);
  MOTOR_IN.setAcceleration(2000);
  MOTOR_IN.setSpeed(10);
  REF_EXT= digitalRead(COL_EXT_SEN_PIN);
  REF_IN= digitalRead(COL_IN_SEN_PIN);
  MOTOR_EXT.setCurrentPosition(1);
  MOTOR_IN.setCurrentPosition(1);
  INIT_EXT=0;
  INIT_IN=0;
  MOTOR_EXT.move(100000);
  MOTOR_IN.move(100000);
  while( INIT_IN == 0 or INIT_EXT == 0 ){
    if (REF_EXT == LOW and INIT_EXT == 0 ) {
      REF_EXT = digitalRead(COL_EXT_SEN_PIN);
    }else{
      if (INIT_EXT == 0 ) {
        MOTOR_EXT.stop();
        INIT_EXT = 1;
      }
    }
    if (REF_IN == LOW and INIT_IN == 0) {
      REF_IN = digitalRead(COL_IN_SEN_PIN);
    }else{
      if (INIT_IN == 0 ) {
        MOTOR_IN.stop();
        INIT_IN = 1;
      }
    }
  MOTOR_EXT.run();
  MOTOR_IN.run();
}
while (MOTOR_EXT.distanceToGo() > 0 or MOTOR_IN.distanceToGo() > 0){
MOTOR_EXT.run();
MOTOR_IN.run();
}
delay(100);
REF_EXT= digitalRead(COL_EXT_SEN_PIN);
REF_IN= digitalRead(COL_IN_SEN_PIN);
MOTOR_EXT.setCurrentPosition(1);
MOTOR_IN.setCurrentPosition(1);
INIT_EXT=0;
INIT_IN=0;
MOTOR_EXT.move(100000);
MOTOR_IN.move(100000);

while( INIT_IN == 0 or INIT_EXT == 0 ){
if (REF_EXT == LOW and INIT_EXT == 0 ) {
  REF_EXT = digitalRead(COL_EXT_SEN_PIN);
}else{
  if (INIT_EXT == 0 ) {
    MOTOR_EXT.stop();
    INIT_EXT = 1;
  }
}
if (REF_IN == LOW and INIT_IN == 0) {
  REF_IN = digitalRead(COL_IN_SEN_PIN);
  }else{
    if (INIT_IN == 0 ) {
    MOTOR_IN.stop();
    INIT_IN = 1;
  }
}
 MOTOR_EXT.run();
 MOTOR_IN.run();
}
while (MOTOR_EXT.distanceToGo() > 0 or MOTOR_IN.distanceToGo() > 0){
MOTOR_EXT.run();
MOTOR_IN.run();
}
delay(100);
MOTOR_EXT.setMaxSpeed(MAX_SPEED_EXT);
MOTOR_EXT.setAcceleration(ACCELERATION_EXT);
MOTOR_EXT.setSpeed(SPEED_EXT);
MOTOR_IN.setMaxSpeed(MAX_SPEED_IN);
MOTOR_IN.setAcceleration(ACCELERATION_IN);
MOTOR_IN.setSpeed(SPEED_IN);
delay(100);
MOTOR_EXT.setCurrentPosition(0);
MOTOR_IN.setCurrentPosition(0);
delay(1000);
MOTOR_EXT.runToNewPosition(TARA_EXT);
MOTOR_IN.runToNewPosition(TARA_IN);
delay(100);
MOTOR_EXT.setCurrentPosition(0);
MOTOR_IN.setCurrentPosition(0);
MAX_ENC_EXT=3600*14;
MAX_ENC_IN=2680*8;
*/
}
void RESET_IN()
{
  /*
  MOTOR_IN.setMaxSpeed(2000);
  MOTOR_IN.setAcceleration(2000);
  MOTOR_IN.setSpeed(10);
  REF_IN= digitalRead(COL_IN_SEN_PIN);
  MOTOR_IN.setCurrentPosition(1);
  INIT_IN=0;
  MOTOR_IN.move(100000);
   while( INIT_IN == 0  ){
      if (REF_IN == LOW and INIT_IN == 0) {
        REF_IN = digitalRead(COL_IN_SEN_PIN);
      }else{
        if (INIT_IN == 0 ) {
          MOTOR_IN.stop();
          INIT_IN = 1;
        }
      }
      MOTOR_IN.run();
    }
    while ( MOTOR_IN.distanceToGo() > 0){
      MOTOR_IN.run();
    }
    delay(1000);
    REF_IN= digitalRead(COL_IN_SEN_PIN);
    MOTOR_IN.setCurrentPosition(1);
    INIT_IN=0;
    MOTOR_IN.move(100000);
    while( INIT_IN == 0  ){
      if (REF_IN == LOW and INIT_IN == 0) {
        REF_IN = digitalRead(COL_IN_SEN_PIN);
      }else{
        if (INIT_IN == 0 ) {
          MOTOR_IN.stop();
          INIT_IN = 1;
        }
      }
      MOTOR_IN.run();
    }
    while ( MOTOR_IN.distanceToGo() > 0){
      MOTOR_IN.run();
    }
    delay(1000);
    MOTOR_IN.setMaxSpeed(MAX_SPEED_IN);
    MOTOR_IN.setAcceleration(ACCELERATION_IN);
    MOTOR_IN.setSpeed(SPEED_IN);
    delay(100);
    MOTOR_IN.setCurrentPosition(0);
    delay(1000);
    MOTOR_IN.runToNewPosition(TARA_IN);
    delay(100);
    MOTOR_IN.setCurrentPosition(0);
    MAX_ENC_IN=2680*8;
*/
}
void RESET_COLUM_EXT()
{
  /*
  MOTOR_EXT.setMaxSpeed(1000);
  MOTOR_EXT.setAcceleration(1000);
  MOTOR_EXT.setSpeed(10);
  REF_EXT= digitalRead(COL_EXT_SEN_PIN);
  MOTOR_EXT.setCurrentPosition(1);
  INIT_EXT=0;
  MOTOR_EXT.move(100000);
   while( INIT_EXT == 0  ){
      if (REF_EXT == LOW and INIT_EXT == 0) {
        REF_EXT = digitalRead(COL_EXT_SEN_PIN);
      }else{
        if (INIT_EXT == 0 ) {
          MOTOR_EXT.stop();
          INIT_EXT = 1;
        }
      }
      MOTOR_EXT.run();
    }
    while ( MOTOR_EXT.distanceToGo() > 0){
      MOTOR_EXT.run();
    }
    delay(1000);
    REF_EXT= digitalRead(COL_EXT_SEN_PIN);
    MOTOR_EXT.setCurrentPosition(1);
    INIT_EXT=0;
    MOTOR_EXT.move(100000);
    while( INIT_EXT == 0  ){
      if (REF_EXT == LOW and INIT_EXT == 0) {
        REF_EXT = digitalRead(COL_EXT_SEN_PIN);
      }else{
        if (INIT_EXT == 0 ) {
          MOTOR_EXT.stop();
          INIT_EXT = 1;
        }
      }
      MOTOR_EXT.run();
    }
    while ( MOTOR_EXT.distanceToGo() > 0){
      MOTOR_EXT.run();
    }

    MOTOR_EXT.setMaxSpeed(MAX_SPEED_EXT);
    MOTOR_EXT.setAcceleration(ACCELERATION_EXT);
    MOTOR_EXT.setSpeed(SPEED_EXT);
    delay(100);
    MOTOR_EXT.setCurrentPosition(0);
    delay(1000);
    MOTOR_EXT.runToNewPosition(TARA_EXT);
    delay(100);
    MOTOR_EXT.setCurrentPosition(0);
    MAX_ENC_EXT=1680*13;
*/
}

void RESET_COLUMS()
{
  /*
  if (MODEL==0){
    RESET_2COLUM();
  }
  if (MODEL==1){
    RESET_IN();
  }
  if (MODEL==2){
    RESET_COLUM_EXT();
  }
*/
}

void ABRIR_CARROS()
{
  SENSOR_READ();
  GET_SENSORS();
  int CN = 0;
  int CL = SEN_STATE[1];
  int SENSORES_CARROS = (bitRead(CL,0)) + (bitRead(CL,1)) + (bitRead(CL,2)) + (bitRead(CL,3));

  //int SENSORES_CARROS = ((READ_PIN(SEN_TROLLEY_1)) + (READ_PIN(SEN_TROLLEY_2)) + (READ_PIN(SEN_TROLLEY_3)) + (READ_PIN(SEN_TROLLEY_4)));
  CN = CARRO_NX;
  
 // int SUMAME = (READ_PIN (LOCK_TROLLEY_1_PIN) + READ_PIN (LOCK_TROLLEY_2_PIN));
  //Serial.println(SUMAME);
  //Serial.println("");
  //Serial.println("");
  //Serial.println(SENSORES_CARROS);
if (SENSORES_CARROS < 4){ 
  CN = 0x0F;
  CARRO_NX = 0;
  }

  digitalWrite(LOCK_TROLLEY_1_PIN, !bitRead(CN,0));
  digitalWrite(LOCK_TROLLEY_2_PIN, !bitRead(CN,1));
  digitalWrite(LOCK_TROLLEY_3_PIN, !bitRead(CN,2));
  digitalWrite(LOCK_TROLLEY_4_PIN, !bitRead(CN,3));
  LED_CARROS(); 

  /*
  if (MODEL==0){
    RESET_2COLUM();
  }
  if (MODEL==1){
    RESET_IN();
  }
  if (MODEL==2){
    RESET_COLUM_EXT();
  }
*/
}
void CARROS_OPEN_SEN(){ // se encarga de desconectar los cierres cuando abrimos el carro.
  SENSOR_READ(); 
  GET_SENSORS();
  
  int CL = SEN_STATE[1];
  int SENSORES_CARROS = (bitRead(CL,0)) + (bitRead(CL,1)) + (bitRead(CL,2)) + (bitRead(CL,3));
  //Serial.print(digitalRead(LOCK_TROLLEY_1_PIN));
  //Serial.print(bitRead(CL,0));
  if (bitRead(CL,0) == 0 && digitalRead(LOCK_TROLLEY_1_PIN) == 1){
    digitalWrite(LOCK_TROLLEY_1_PIN, 0);
    CARRO_ABIERTO = 1;
  }
  if (bitRead(CL,1) == 0 && digitalRead(LOCK_TROLLEY_2_PIN) == 1){
    digitalWrite(LOCK_TROLLEY_2_PIN, 0);
    CARRO_ABIERTO = 1;    
  }
  if (bitRead(CL,2) == 0 && digitalRead(LOCK_TROLLEY_3_PIN) == 1){
    digitalWrite(LOCK_TROLLEY_3_PIN, 0);
    CARRO_ABIERTO = 1;
  }
  if (bitRead(CL,3) == 0 && digitalRead(LOCK_TROLLEY_4_PIN) == 1){
    digitalWrite(LOCK_TROLLEY_4_PIN, 0);
    CARRO_ABIERTO = 1;
    delay(100);
  //Serial.print(bitRead(CL,0));
  }  
  else{
    if (CARRO_ABIERTO == 1 && SENSORES_CARROS > 3 ){
          //Serial.print(SENSORES_CARROS);

    
    CARRO_NX = 0;
    LED_CARROS();
       
  }
  }
}


/// Color TIRAS LED PUERTA
void setColor(int Pixel, int R2, int G2, int B2) {
  uint32_t color = strip.Color(R2, G2, B2); // Fabrica el color
  strip.setPixelColor(Pixel, color); //Crea el color de cada pixel
  strip.show(); //Actualiza el color
}
void doClear() {
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 0, 0); //Apaga todos los led
  }
}
void efectoBlanco(){
  
      for (int i = COUNT; i >= 0; i--) {
      setColor(i, 255, 255, 255); //EFECTO REVERSO BLANCO
      delay(TIEMPOENTRELED);
      }
     
}
//sELECCION LED CARRO
void LED_CARROS(){
  SENSOR_READ();
  GET_SENSORS();
  int CL = 0;
  CARRO_ABIERTO = 0;
  if (CARRO_NX == 0 ){   
    CL = SEN_STATE[1];
    }  
else{
  CL = CARRO_NX;
}

//int C = SEN_STATE[1];
int SENCARRO = (bitRead(CL,0)) + (bitRead(CL,1)) + (bitRead(CL,2)) + (bitRead(CL,3));
  //Serial.println(SENCARRO);
  //Serial.println(bitRead(CL,3));
  //Serial.println(bitRead(CL,2));
  //Serial.println(bitRead(CL,1));
  //Serial.println(bitRead(CL,0));
    if (bitRead(CL,0) == 0) 
    {  
        
    if (SENCARRO == 3) {
      todorojo();
    for (int i = 0; i < LEDCARRO; i++) {
      //setColor(i, R, G, B);
      setColor(i, 0, 255, 0);
      delay(TIEMPOENTRELED);
      }
    //Serial.println("Cajon 1 abierto");
    }
    else
    {
    todoamarillo();
    //outp = 0;
    //Serial.println("Alguna Puerta Abierta");
    }
  }
  else if (bitRead(CL,1) == 0) {
    if (SENCARRO == 3) {
    todorojo();
    for (int i = 8; i < LEDCARRO + 8; i++) {
      //setColor(i, R, G, B);
      
      setColor(i, 0, 255, 0);
      delay(TIEMPOENTRELED);
      }
    //Serial.println("Cajon 2 abierto");
    }
    else
    {
    todoamarillo();
    //outp = 0;
    //Serial.println("Alguna Puerta Abierta");
    }
  }
  else if (bitRead(CL,2) == 0) {
    if (SENCARRO == 3) {
    todorojo();
    for (int i = 16; i < LEDCARRO + 16; i++) {
      //setColor(i, R, G, B);
      setColor(i, 0, 255, 0);
      delay(TIEMPOENTRELED);
      //Serial.println(i);

      }
    //Serial.println("Cajon 3 abierto");
    }
    else
    {
    todoamarillo();
    //outp = 0;
    //Serial.println("Alguna Puerta Abierta");
    }
  }
  
   else if (bitRead(CL,3) == 0) {
    if (SENCARRO == 3) {
    todorojo();
    for (int i = 24; i < LEDCARRO + 24; i++) {
      //setColor(i, R, G, B);
      setColor(i, 0, 255, 0);
      delay(TIEMPOENTRELED);
      }
    //Serial.println("Cajon 4 abierto");
    }
    else
    {
    todoamarillo();
    //outp = 0;
    //Serial.println("Alguna Puerta Abierta");
    }
  }
  
 else {
    todoazul();
  }
}
void todoazul() {
      efectoBlanco();
      for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 0, 255);

    delay(TIEMPOENTRELED);
  }
  //Serial.println("Orden todo funcionando OK");
}
void todorojo() {
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 255, 0, 0);
  delay(TIEMPOENTRELED);
  }
  //Serial.println("Cajones cerrados");
}
void todoamarillo() {
      efectoBlanco();
    
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 255, 247, 0);
    delay(TIEMPOENTRELED);
  }
  //Serial.println("Esperando orden PC abrir Cajon");
}
void doShow() {
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 0, 0);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 255, 0, 0); //turn all to red
    delay(5);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 255, 0); //turn all to blue
    delay(5);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 0, 255); //turn all to green
    delay(5);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 255, 247, 0); //turn all to yellow
    delay(5);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 255, 100, 0); //turn all to orange
    delay(25);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 255, 0, 205); //turn all to purple
    delay(5);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 213, 255); //turn all to turquoise
    delay(5);
  }
  for (int i = 0; i < COUNT; i++) {
    setColor(i, 0, 0, 0); //turn all to OFF
    delay(5);
  }
LED_CARROS();
}


uint16_t Checksums(const byte *data, int dataLength, int dataint) {
  uint8_t result = 0;
  uint16_t sum = 0;
  for (uint8_t i = dataint; i < (dataLength - 1); i++) {
    sum += data[i];
  }
  result = sum & 0xFF;
  return result;
}

long toInt(const byte *bRefArr) {
  long iOutcome = 0;
  byte bLoop;          
  for ( int i =0; i<2 ; i++) {
      bLoop = bRefArr[i];
      iOutcome= (long)iOutcome +((long)(bLoop & 0xFF) << (8 * i));         
  }           
  return iOutcome;
}

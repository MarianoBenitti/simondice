/*! \mainpage Ejercicio Titulo
 * \date 01/01/2023
 * \author Nombre
 * \section genDesc Descripcion general
 * [Complete aqui con su descripcion]
 *
 * \section desarrollos Observaciones generales
 * [Complete aqui con sus observaciones]
 *
 * \section changelog Registro de cambios
 *
 * |   Fecha    | Descripcion                                    |
 * |:----------:|:-----------------------------------------------|
 * | 29/08/2023 | Creacion del documento                         |
 * | 30/08/2023 | Prueba de botones                              |
 * | 4/09/2023  | Correccion de errores y mejora de codigo       |  
 * | 7/09/2023  | comienzo de comunicacion con la placa          |
 * | 8/09/2023  | Comienzo armado de protocolo de comunicacion   |    
 */



/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "stdlib.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/
typedef enum{UP,FALLING,RISSING,DOWN}e_estadoB;
//_________________________________________________________________________________________________________________________________________
typedef enum{SECUENCIAINICIAL,MOSTRARCANTNIVEL,SELNIVEL,GENSEC,MNIVEL,ENCLEDS,CUENTAATRAS,MSECUENCIA,PREPARACION,JUGANDO,FINAL}e_simonDice;
typedef union{
    struct {
       uint8_t b0:1;
       uint8_t b1:1;
       uint8_t b2:1;
       uint8_t b3:1;//se utiliza para saber si se gano o se perdio
       uint8_t b4:1;//se usa para saber si mostrar el nivel
       uint8_t b5:1;
       uint8_t b6:1;
       uint8_t b7:1;
    }bit;
    uint8_t byte;
}band;
//______________________________________________________________________________
typedef struct __attribute__((packed,aligned(1))){
e_estadoB e_estadoBoton;
uint8_t presion;//indica si el boton se presiono
uint32_t Tpresion;//indica el tiempo que se presiono
}boton;
//______________________________________________________________________________
typedef struct __attribute__((packed,aligned(1))){
uint8_t iLL;//indice de lectura
uint8_t iEL;//indice de escritura
uint8_t estHeader;//estado del header
uint8_t nBytes;//numero de bytes de datos
uint8_t iDatos;//inicio de los datos en el buffer
uint8_t *bufL;//puntero al buffer con los datos leidos
uint8_t checksum;//checksum del comando
uint8_t tamBuffer;//tamaño que limita el buffer debe ser de 2 a la n para que funcione correctamente la compuerta and de reinicio
}s_LDatos;//estructura de lectura

typedef struct{
uint8_t iEE;//indice de escritura en el buffer
uint8_t iLE;//indice de lectura para transmitir los datos
uint8_t checksum;//checksum del comando
uint8_t *bufE;//puntero al buffer de escritura
uint8_t tamBuffer;//tamaño del buffer de escritura, debe ser de 2 a la n
}s_EDatos;//estructura de escritura
/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/
#define ISNEWBYTE banderas.bit.b1
#define WINORLOST banderas.bit.b3
/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);
BusIn BOTONES(PA_7,PA_6,PA_5,PA_4);
BusOut LEDS(PB_6,PB_7,PB_14,PB_15);
RawSerial PC(PA_9,PA_10);

/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/
/** \fn ComprobarBotones
 * \brief 
 *  Esta funcion comprueba el estado de los 4 botones que estan dentro del bus de entrada
 *  una vez comprobado el estado dentro de la estructura global de los botones establece el estado del boton,
 *  si fue presionado y cuanto tiempo estubo presionado.
 * */
void ComprobarBotones();
/** \fn OnRxByte
 * \brief 
 *  Verifica si hay algo para leer en el buffer de entrada, si hay algo para leer lo recibe y 
 * lo pasa al buffer de memoria para poder analizarlo.
 * */
void OnRxByte();
/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
const uint8_t SecIni[7]={0b1000,0b1100,0b0100,0b0110,0b0010,0b0011,0b0001};
Timer timerGen;//timer general del programa
int tAnt=0;//tiempo anterior tomado
int tAntBot=0;//tiempo anterior tomado de los botones
int tAntJuego=0;//tiempo anterior usado en el juego
band banderas;//conjunto de banderas para el juego
boton botones[4];//tiene los datos de cada boton en particular
e_simonDice e_estadoJuego;//estado actual del juego
uint8_t secuencia[12];
int aux=0;//esta variable auxiliar se utiliza para pasar la secuencia leida a los leds
uint8_t lvl=4;
uint8_t lvlAct=0;
uint8_t i=0;
uint8_t j=0;

//generamos una variable volatile para la comunicacion
volatile uint8_t dataByte;
//generamos el buffer de Lectura
uint8_t bufferL[64];
s_LDatos datosLec;

//generamos el buffer de Escritura
uint8_t bufferE[64];
s_EDatos datosEsc;

/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/

void ComprobarBotones(){
    
    if(timerGen.read_ms()-tAntBot>=40){
            tAntBot=timerGen.read_ms();
            for(i=0;i<4;i++){
                switch(botones[i].e_estadoBoton){
                    case UP:
                            if(BOTONES & (1<<i)){
                                botones[i].e_estadoBoton=FALLING;
                            }
                        break;
                    case FALLING:
                            if(BOTONES & (1<<i)){
                                botones[i].e_estadoBoton=DOWN;
                                botones[i].presion=1;
                                botones[i].Tpresion=timerGen.read_ms();
                            }
                        break;
                    case DOWN:
                                if((BOTONES & (1<<i))==0){
                                     botones[i].e_estadoBoton=RISSING;
                                 }
                                
                        break;    
                    case RISSING:if((BOTONES & (1<<i))==0){
                                    botones[i].e_estadoBoton=UP;
                                    botones[i].Tpresion=timerGen.read_ms()-botones[i].Tpresion;//calculamos el tiempo presionado
                                 }
                        break;
                    default:botones[i].e_estadoBoton=UP;
                        break;
                }

            }
        }
}

void OnRxByte(){
    while (PC.readable()){
        datosLec.bufL[datosLec.iEL] = PC.getc();
        datosLec.iEL++;
        if (datosLec.iEL>=datosLec.tamBuffer){
            datosLec.iEL=0;
        }
        //(datosLec.iEL)= (datosLec.tamBuffer) & (datosLec.iEL);//se reinicia cuando llega a 64
    }
}
/* END Function prototypes user code ------------------------------------------*/

int main()
{
/* Local variables -----------------------------------------------------------*/
for(i=0;i<4;i++){
botones[i].e_estadoBoton=UP;
botones[i].Tpresion=0;
botones[i].presion=0;
}
/* END Local variables -------------------------------------------------------*/
 

/* User code -----------------------------------------------------------------*/
    timerGen.start();
    //ENLAZAMOS EL PUERTO SERIE
    PC.baud(115200);//ASIGNAMOS LA VELOCIDAD DE COMUNICACION
    PC.attach(&OnRxByte,SerialBase::IrqType::RxIrq);//ENLAZAMOS LA FUNCION CON EL METODO DE LECTURA
    //INICIALIZAMOS BUFFER DE ENTRADA
    datosLec.iEL=0;
    datosLec.iLL=0;
    datosLec.nBytes=0;
    datosLec.iDatos=0;
    datosLec.checksum=0;
    datosLec.estHeader=0;
    datosLec.tamBuffer=64;
    datosLec.bufL=bufferL;
    //INICIALIZAOS EL BUFFER DE SALIDA
    datosEsc.iEE=0;
    datosEsc.iLE=0;
    datosEsc.checksum=0;
    datosEsc.bufE=bufferE;
    datosEsc.tamBuffer=64;
    while(1){
    	if(timerGen.read_ms()-tAnt>=300){
            tAnt=timerGen.read_ms();
            LED = !LED;
        }

        ComprobarBotones();
        
        switch(e_estadoJuego){
            case SECUENCIAINICIAL:
                    if(timerGen.read_ms()-tAntJuego>=200){
                        tAntJuego=timerGen.read_ms();
                       if(j>=7){
                        j=0;
                       }
                        LEDS = ~SecIni[j];
                        j++;
                    }
                    if(botones[1].presion){
                        tAntJuego=timerGen.read_ms();
                        botones[1].presion=0;
                        e_estadoJuego=MOSTRARCANTNIVEL;
                        j=0;
                    }
                    if(botones[0].Tpresion>=1000 && botones[0].Tpresion<=3000){
                        botones[0].Tpresion=0;
                        e_estadoJuego=GENSEC;
                        j=0;
                    }
                break;
            case MOSTRARCANTNIVEL:
                    LEDS=~lvl;
                    if(timerGen.read_ms()-tAntJuego>=2000){
                        e_estadoJuego=SECUENCIAINICIAL;
                    }
                     if(botones[1].presion){
                        tAntJuego=timerGen.read_ms();
                        botones[1].presion=0;
                        e_estadoJuego=SELNIVEL;
                     }
                break;
            case SELNIVEL:
                    lvl++;
                    if(lvl>12){
                        lvl=4;
                    }
                    e_estadoJuego=MOSTRARCANTNIVEL;
                break;
            case GENSEC:srand(timerGen.read_ms());
                        for(i=0;i<lvl;i++){
                            secuencia[i]=rand() % 4;
                            while (i!=0 && secuencia[i]==secuencia[i-1]){//evitamos que los colores se repitan
                                secuencia[i]=rand() % 4;
                            }
                        }
                        e_estadoJuego=MNIVEL;
                        tAntJuego=timerGen.read_ms();
                        lvlAct=1;
                break;
            case MNIVEL://se muestra el nivel actual
                        LEDS=~lvlAct;
                        if(timerGen.read_ms()-tAntJuego>=2000){
                            e_estadoJuego=ENCLEDS;
                            tAntJuego=timerGen.read_ms();
                        }
                break;
            case ENCLEDS:
                    LEDS=0b0000;//encendemos todos los leds
                    if(timerGen.read_ms()-tAntJuego>=1000){
                        j=0;
                        e_estadoJuego=CUENTAATRAS;
                        tAntJuego=timerGen.read_ms();
                    }
                    break;
            case CUENTAATRAS:
                 if(timerGen.read_ms()-tAntJuego>=1000){
                        LEDS=LEDS | (1<<j);//hacemos la cuenta regresiva(ponemos en 1 cada led)
                        j++;
                        if(j>4){
                            j=0;
                            aux=1000;//esperamos 1000 ms antes de mostrar la secuencia
                            e_estadoJuego=MSECUENCIA;
                        }
                        tAntJuego=timerGen.read_ms();
                    }
                break;
            case MSECUENCIA:
                    //MUESTREO DE LA SECUENCIA
                    if (timerGen.read_ms()-tAntJuego>=aux){
                        LEDS=0b1111;//apagamos todos los leds
                        if(j==lvlAct){//si ya se mostro toda la secuencia se pasa a preparacion
                            e_estadoJuego=PREPARACION; 
                        }else{
                            srand(timerGen.read_ms());
                            aux=rand()%(500+1)+100;//se utiliza la auxiliar para guardar el tiempo aleatorio
                            LEDS=~(LEDS & 1<<secuencia[j]);
                            j++;
                        }
                        tAntJuego=timerGen.read_ms();
                    }
                break;
            case PREPARACION:
                for(i=0;i<4;i++){
                    botones[i].presion=0;//borramos la basura de los botones
                    j=0;//inicializamos j que usaremos para saber la posicion de la secuencia
                }
                e_estadoJuego=JUGANDO;
                tAntJuego=timerGen.read_ms();
                break;
            case JUGANDO:
                //SE VERIFICA QUE NO PASARON LOS 3 SEGUNDOS LIMITE
                if(timerGen.read_ms()-tAntJuego>=3000){
                    WINORLOST=0;
                    e_estadoJuego=FINAL;
                    LEDS=0b0000;//encendemos todos los leds para la secuencia final
                }
                //SE VERIFICA QUE BOTON SE PRESIONO Y SE LO COMPARA CON LA SECUENCIA
                for(i=0;i<4;i++){
                    if(botones[i].presion){
                        botones[i].presion=0;//anulo la presion del boton
                        LEDS=0b1111;
                        LEDS=LEDS & ~(1<<i);//encendemos el led presionado
                        
                        if(i!=secuencia[j]){
                            e_estadoJuego =FINAL;
                            LEDS=0b0000;//encendemos todos los leds para la secuencia final
                            WINORLOST=0;
                            j=0;
                        }else{
                            
                            j++;//pasamos a comparar la siguiente parte de la secuencia
                            if(j==(lvl)){//si se supero el ultimo nivel se termina el juego
                                e_estadoJuego=FINAL;
                                LEDS=0b0000;//encendemos todos los leds para la secuencia final
                                WINORLOST=1;
                                j=0;
                            }
                            if(j==lvlAct){//si se supero el nivel actual se vuelve a mostrar la secuencia
                                lvlAct++;
                                e_estadoJuego=MNIVEL;
                            }
                        }
                        tAntJuego=timerGen.read_ms();
                    }
                }
                
                break;
            case FINAL:if(WINORLOST){
                            //si se gano
                            if(timerGen.read_ms()-tAntJuego>=500){
                                tAntJuego=timerGen.read_ms();
                                j++;
                                LEDS=~LEDS;
                            }
                            if(j==4){
                            e_estadoJuego=SECUENCIAINICIAL;
                            WINORLOST=0;
                            }
                        }else{
                            //si se perdio
                            if(timerGen.read_ms()-tAntJuego>=100){
                                tAntJuego=timerGen.read_ms();
                                j++;
                                LEDS=~LEDS;
                            }
                            if(j==20){
                            e_estadoJuego=SECUENCIAINICIAL;
                            WINORLOST=0;
                            }
                        }
                        
                break;
            default:e_estadoJuego=SECUENCIAINICIAL;
                break;
        }
        
        //GENERAMOS LA COMUNICACION CON LA PC
        
        if(datosLec.iEL != datosLec.iLL){
            while(PC.writeable()){
                PC.putc(datosLec.bufL[datosLec.iLL]);
                datosLec.iLL++;
                if(datosLec.iLL>=datosLec.tamBuffer){
                    datosLec.iLL=0;
                }
            }
        }

    }

/* END User code -------------------------------------------------------------*/
}



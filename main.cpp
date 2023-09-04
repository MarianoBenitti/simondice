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
 *     
 */



/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "stdlib.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/
typedef enum{UP,FALLING,RISSING,DOWN}e_estadoB;
typedef enum{SECUENCIAINICIAL,GENSEC,MSECUENCIA,PREPARACION,JUGANDO,FINAL}e_simonDice;
typedef union{
    struct {
       uint8_t b0:1;//se utiliza para verificar si se esta cambiando el nivel
       uint8_t b1:1;//se utiliza para saber cuando mostrar la cuenta regresiva
       uint8_t b2:1;//se utiliza para saber cuando mostrar la secuencia
       uint8_t b3:1;//se utiliza para saber si se gano o se perdio
       uint8_t b4:1;//se usa para saber si mostrar el nivel
       uint8_t b5:1;
       uint8_t b6:1;
       uint8_t b7:1;
    }bit;
    uint8_t byte;
}band;
typedef struct {
e_estadoB e_estadoBoton;
uint8_t presion;//indica si el boton se presiono
uint32_t Tpresion;//indica el tiempo que se presiono
}boton;
/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/

/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);
BusIn BOTONES(PA_7,PA_6,PA_5,PA_4);
BusOut LEDS(PB_6,PB_7,PB_14,PB_15);
/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/

/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
const uint8_t SecIni[7]={0b1000,0b1100,0b0100,0b0110,0b0010,0b0011,0b0001};
Timer timerGen;//timer general del programa
uint32_t tAnt=0;//tiempo anterior tomado
uint32_t tAntBot=0;//tiempo anterior tomado de los botones
uint32_t tAntJuego=0;//tiempo anterior usado en el juego
band banderas;//conjunto de banderas para el juego
boton botones[4];//tiene los datos de cada boton en particular
e_simonDice e_estadoJuego;//estado actual del juego
uint8_t secuencia[12];
uint32_t aux=0;//esta variable auxiliar se utiliza para pasar la secuencia leida a los leds
uint8_t lvl=4;
uint8_t lvlAct=0;
uint8_t i=0;
uint8_t j=0;
/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/
//comprueba el estado de los botones,y lo carga en la enumeracion correspondiente al boton
void ComprobarBotones();
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
    
    while(1){
    	if(timerGen.read_ms()-tAnt>=300){
            tAnt=timerGen.read_ms();
            LED = !LED;
        }

        ComprobarBotones();
        
        switch(e_estadoJuego){
            case SECUENCIAINICIAL:
                    if(timerGen.read_ms()-tAntJuego>=200 && banderas.bit.b0==0){
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
                       //se que la bandera va a estar en 1 si no paso el tiempo y si ya entro una vez
                        lvl=lvl+banderas.bit.b0;
                        if(lvl>12){
                            lvl=4;
                        }
                        LEDS=~lvl;
                        banderas.bit.b0=1;
                    }
                    if(timerGen.read_ms()-tAntJuego>=2000 && banderas.bit.b0){//dejamos de cambiar nivel
                        banderas.bit.b0=0;
                        j=0;
                    }
                    if(botones[0].Tpresion>=1000 && botones[0].Tpresion<=2000){
                        botones[0].Tpresion=0;
                        e_estadoJuego=GENSEC;
                    }
                break;
            case GENSEC:srand(timerGen.read_ms());
                        for(i=0;i<lvl;i++){
                            secuencia[i]=rand() % 4;
                            while (i!=0 && secuencia[i]==secuencia[i-1]){//evitamos que los colores se repitan
                                secuencia[i]=rand() % 4;
                            }
                        }
                        e_estadoJuego=MSECUENCIA;
                        banderas.bit.b4=1;
                        lvlAct=1;
                break;
            case MSECUENCIA:
                    //MUESTREO DEL NIVEL
                    if(banderas.bit.b4){
                        LEDS=~lvlAct;
                        banderas.bit.b4=0;
                        tAntJuego=timerGen.read_ms();
                    }
                    //SE ENCIENDEN TODOS LOS LEDS
                    if(timerGen.read_ms()-tAntJuego>=1000 && banderas.bit.b1==0 && banderas.bit.b2==0){
                        banderas.bit.b1=1;
                        LEDS=0b0000;//encendemos todos los leds
                        j=0;
                        tAntJuego=timerGen.read_ms();
                    }
                    //CUENTA REGRESIVA
                    if(banderas.bit.b1 && timerGen.read_ms()-tAntJuego>=1000){
                        LEDS=LEDS | (1<<j);//hacemos la cuenta regresiva(ponemos en 1 cada led)
                        j++;
                        if(j>=4){
                            banderas.bit.b1=0;//se finaliza la cuenta regresiva
                            banderas.bit.b2=1;//para mostrar la secuencia
                            j=0;
                            aux=1000;//esperamos 1000 ms antes de mostrar la secuencia
                        }
                        tAntJuego=timerGen.read_ms();
                    }
                    //MUESTREO DE LA SECUENCIA
                    
                    if (banderas.bit.b2 && (timerGen.read_ms()-tAntJuego>=aux)){
                        LEDS=0b1111;//apagamos todos los leds
                        if(j==lvlAct){//si ya se mostro toda la secuencia se pasa a preparacion
                            e_estadoJuego=PREPARACION; 
                            banderas.bit.b2=0; 
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
                    banderas.bit.b3=0;
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
                            banderas.bit.b3=0;
                            j=0;
                            
                        }else{
                            
                            j++;//pasamos a comparar la siguiente parte de la secuencia
                            if(j==(lvl)){//si se supero el ultimo nivel se termina el juego
                                e_estadoJuego=FINAL;
                                LEDS=0b0000;//encendemos todos los leds para la secuencia final
                                banderas.bit.b3=1;
                                j=0;
                            }
                            if(j==lvlAct){//si se supero el nivel actual se vuelve a mostrar la secuencia
                                lvlAct++;
                                banderas.bit.b4=1;//para mostrar el nuevo nivel
                                e_estadoJuego=MSECUENCIA;
                            }
                        }
                        tAntJuego=timerGen.read_ms();
                    }
                }
                
                break;
            case FINAL:if(banderas.bit.b3){
                            //si se gano
                            if(timerGen.read_ms()-tAntJuego>=500){
                                tAntJuego=timerGen.read_ms();
                                j++;
                                LEDS=~LEDS;
                            }
                            if(j==4){
                            e_estadoJuego=SECUENCIAINICIAL;
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
                            }
                        }
                        
                break;
            default:e_estadoJuego=SECUENCIAINICIAL;
                break;
        }
    }

/* END User code -------------------------------------------------------------*/
}


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
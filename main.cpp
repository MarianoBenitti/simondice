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
 * | 01/01/2023 | Creacion del documento                         |
 *
 */



/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/

/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/
typedef enum{UP,FALLING,RISSING,DOWN}e_estadoB;

/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);
BusIn BOTONES(PA_7,PA_6,PA_5,PA_4);
BusOut LEDS(PB_6,PB_7,PB_14,PB_15);
/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/

/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
Timer timerGen;//timer general del programa
uint32_t tAnt=0;//tiempo anterior tomado
uint32_t tAntBot=0;//tiempo anterior tomado de los botones
e_estadoB e_estadoBotones[4];

uint16_t i=0;
/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/

/* END Function prototypes user code ------------------------------------------*/

int main()
{
/* Local variables -----------------------------------------------------------*/
e_estadoBotones[0]=UP;
e_estadoBotones[1]=UP;
e_estadoBotones[2]=UP;
e_estadoBotones[3]=UP;
/* END Local variables -------------------------------------------------------*/


/* User code -----------------------------------------------------------------*/
    timerGen.start();
    
    while(1){
    	if(timerGen.read_ms()-tAnt>=300){
            tAnt=timerGen.read_ms();
            LED = !LED;
        }
        
        
        if(timerGen.read_ms()-tAntBot>=40){
            tAntBot=timerGen.read_ms();
            for(i=0;i<4;i++){
                switch(e_estadoBotones[i]){
                    case UP:
                            if(BOTONES & (1<<i)){
                                e_estadoBotones[i]=FALLING;
                                
                            }
                        break;
                    case FALLING:
                            if(BOTONES & (1<<i)){
                                e_estadoBotones[i]=DOWN;
                                
                            }
                        break;
                    case DOWN:
                                if((BOTONES & (1<<i))==0){
                                     e_estadoBotones[i]=RISSING;
                                 }
                                LEDS=BOTONES | (1<<i);
                        break;    
                    case RISSING:if((BOTONES & (1<<i))==0){
                                     e_estadoBotones[i]=UP;
                                 }
                        break;
                    default:e_estadoBotones[i]=UP;
                        break;
                }

            }
        }
        
    }

/* END User code -------------------------------------------------------------*/
}



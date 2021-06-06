/*
 * cardata.h
 *
 *  Created on: Jun 5, 2021
 *      Author: denjo
 */

#ifndef INC_CARDATA_H_
#define INC_CARDATA_H_

/* Private includes ----------------------------------------------------------*/
#include "stddef.h"  // required for size_t

/* Exported types ------------------------------------------------------------*/

#define D_CAR_MAXPM 13
typedef struct {
	size_t N;      // the number of pulsemode
	float fs[D_CAR_MAXPM];
	int pm[D_CAR_MAXPM];
	float fc1[D_CAR_MAXPM];
	float fc2[D_CAR_MAXPM];
	float frand1[D_CAR_MAXPM];
	float frand2[D_CAR_MAXPM];
} PulseMode;

typedef struct {
	float lost;    // regen lost frequency [Hz]
	float acc0;    // starting acceleration [km/h/s]
	float brk0;    // usual max break acceleration [km/h/s]
	float eb0;     // emergency break acceleration [km/h/s]
	float gr;      // gear ratio
	float pp;      // number of pole pair
	float rw;      // radius of wheel [m]
} CarParam;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
int Cardata_getPulsemode(int, PulseMode*);
int Cardata_getCarparam(int, CarParam*);

/* Private defines -----------------------------------------------------------*/

#endif /* INC_CARDATA_H_ */

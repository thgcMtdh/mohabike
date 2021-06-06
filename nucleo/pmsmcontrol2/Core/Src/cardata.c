/*
 * cardata.c
 *
 *  Created on: Jun 5, 2021
 *      Author: denjo
 */

/**
  ******************************************************************************
  * @file           : cardata.c
  * @brief          : Store car data (pulse mode & parametes)
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include "cardata.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {  // Pulse mode element
	const float fs;
	const int pm;
	const float fc1;
	const float fc2;
	const float frand1;
	const float frand2;
} PmElement;

typedef struct {  // a Set of pulse mode pattern
	const PmElement* pmpattern;
	const size_t N;      // number of pulse mode elements
	const float lost;    // regen lost frequency [Hz]
	const float acc0;    // starting acceleration [km/h/s]
	const float brk0;    // usual max break acceleration [km/h/s]
	const float eb0;     // emergency break acceleration [km/h/s]
	const float gr;      // gear ratio
	const float pp;      // number of pole pair
	const float rw;      // radius of wheel [m]
} CarData;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define D_CAR_NUMOFCAR 4

/* Private variables ---------------------------------------------------------*/

/* ========= DEFINITION OF CAR PULSEMODE ==========*/

const PmElement tobu100[] = {
	{0.0,	0,	200,200,0,0},
	{5.4,	45,	0,0,0,0},
	{7.0,	27,	0,0,0,0},
	{13.7,	15,	0,0,0,0},
	{25.0,	9,	0,0,0,0}
};

const PmElement E231tsuiraku[] = {
	{0.0,	0,	1050,1050,0,0},
	{23.0,	0,	1050,700,0,0},
	{48.0,	0,	700,1800,0,0},
	{59.0,	9,	0,0,0,0}
};

const PmElement keikyu1000[] = {
	{0.0,	0,	175,175,0,0},
	{1.5,	0,	196,196,0,0},
	{2.0,	0,	220.5,220.5,0,0},
	{2.5,	0,	233,233,0,0},
	{3.0,	0,	262,262,0,0},
	{3.5,	0,	293.5,293.5,0,0},
	{4.0,	0,	311,311,0,0},
	{4.5,	0,	349.5,349.5,0,0},
	{5.0,	0,	400,400,0,0},
	{24.1,	18,	0,0,0,0},
	{26.6,	15,	0,0,0,0},
	{30.5,	12,	0,0,0,0},
	{36.0,	9,	0,0,0,0}
};

const PmElement metro16000[] = {
	{0.0,	0,	2000,2000,0,0},
	{40.0,	27,	0,0,0,0}
};

/* ========= DEFINITION OF CAR PARAMETERS ==========*/
const CarData cardata[D_CAR_NUMOFCAR] = {
	{tobu100, sizeof(tobu100)/sizeof(PmElement),			3.0, 2.0, -3.7, -5.3, 5.31, 2, 0.43},
	{E231tsuiraku, sizeof(E231tsuiraku)/sizeof(PmElement),	0.0, 2.3, -4.5, -4.5, 6.6, 2, 0.43},
	{keikyu1000, sizeof(keikyu1000)/sizeof(PmElement),		5.0, 2.0, -4.0, -4.5, 5.93, 2, 0.43},
	{metro16000, sizeof(metro16000)/sizeof(PmElement),		0.0, 3.3, -4.0, -4.5, 7.2, 2, 0.43}
};

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/**
  * @brief Return pulsemode data .
  * @param id: car id (starting from zero), dest: pointer of the destination to store pulsemode
  * @retval 0:success, -1:error
  */
int Cardata_getPulsemode(int id, PulseMode* dest) {
	if (id >= D_CAR_NUMOFCAR) {
		return -1;
	}
	size_t N = cardata[id].N;
	dest->N = N;
	// copy pulse mode pattern
	for (int i=0; i<D_CAR_MAXPM; i++) {
		if (i < N) {
			dest->fs[i] = cardata[id].pmpattern[i].fs;
			dest->pm[i] = cardata[id].pmpattern[i].pm;
			dest->fc1[i] = cardata[id].pmpattern[i].fc1;
			dest->fc2[i] = cardata[id].pmpattern[i].fc2;
			dest->frand1[i] = cardata[id].pmpattern[i].frand1;
			dest->frand2[i] = cardata[id].pmpattern[i].frand2;
		} else {
			dest->fs[i] = 0.0;
			dest->pm[i] = 0;
			dest->fc1[i] = 0.0;
			dest->fc2[i] = 0.0;
			dest->frand1[i] = 0.0;
			dest->frand2[i] = 0.0;
		}
	}
	return 0;
}

/**
  * @brief Return car parameters.
  * @param id: car id (starting from zero), dest: pointer of the destination to store car params
  * @retval 0:success, -1:error
  */
int Cardata_getCarparam(int id, CarParam* dest) {
	if (id >= D_CAR_NUMOFCAR) {
		return -1;
	}
	dest->acc0 = cardata[id].acc0;
	dest->brk0 = cardata[id].brk0;
	dest->eb0 = cardata[id].eb0;
	dest->lost = cardata[id].lost;
	dest->gr = cardata[id].gr;
	dest->pp = cardata[id].pp;
	dest->rw = cardata[id].rw;
	return 0;
}


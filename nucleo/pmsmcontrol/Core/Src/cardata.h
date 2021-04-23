/**
  ******************************************************************************
  * @file        :cardata.h
  * @brief       :This file provides car informations such as pulse mode pattern
  ******************************************************************************
  */

#define NUMOFCAR 2
const char* carstr[NUMOFCAR] = {
	"0,0,2000,2000,0,0,\n40,0,1600,1700,0,0,",
	"0,0,1050,1050,0,0,\n23,0,1050,700,0,0,\n48,0,700,1800,0,0,\n59,3,0,0,0,0,"
};

const float acc0[NUMOFCAR] = {3.3, 2.3};  // starting acceleration [km/h/s]
const float brk0[NUMOFCAR] = {-4.0, -4.5};  // usual max break acceleration [km/h/s]
const float eb0[NUMOFCAR] = {-4.5, -4.5};  // emergency break acceleration [km/h/s]
const float gr[NUMOFCAR] = {7.2, 6.6};  // gear ratio
const float pp[NUMOFCAR] = {2, 2};  // pole pair
const float rwheel[NUMOFCAR] = {0.43, 0.43};  // radius of wheel [m]

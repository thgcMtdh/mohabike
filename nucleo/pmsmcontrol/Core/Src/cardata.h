/**
  ******************************************************************************
  * @file        :cardata.h
  * @brief       :This file provides car informations such as pulse mode pattern
  ******************************************************************************
  */

#define NUMOFCAR 3

struct CarData {
	char* pattern; // pulsemode pattern
	float acc0;    // starting acceleration [km/h/s]
	float brk0;    // usual max break acceleration [km/h/s]
	float eb0;     // emergency break acceleration [km/h/s]
	float gr;      // gear ratio
	float pp;      // number of pole pair
	float rwheel;  // radius of wheel [m]
};

struct CarData cardata[NUMOFCAR] = {
	{"0,0,2000,2000,0,0,",//\n40,0,1600,1700,0,0
			3.3, -4.0, -4.5, 7.2, 2, 0.43},
	{"0,0,1050,1050,0,0,\n23,0,1050,700,0,0,\n48,0,700,1800,0,0,\n59,0,885,1035,0,0,",
			2.3, -4.5, -4.5, 6.6, 2, 0.43},
	{"0,0,175,175,0,0,\n1.5,0,196,196,0,0,\n2.0,0,220.5,220.5,0,0,\n2.5,0,233,233,0,0,\n3.0,0,262,262,0,0,\n3.5,0,293.5,293.5,0,0,\n4.0,0,311,311,0,0,\n4.5,0,349.5,349.5,0,0,\n5.0,0,400,400,0,0,\n24.1,0,650.7,718.2,0,0,\n26.6,0,598.5,686.25,0,0,\n30.5,0,549,648,0,0,\n36.0,0,486,594,0,0,",//\n44.0,0,396,468,0,0,",
			3.5, -4.0, -4.5, 5.93, 2, 0.43}
};

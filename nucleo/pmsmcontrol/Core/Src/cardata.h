/**
  ******************************************************************************
  * @file        :cardata.h
  * @brief       :This file provides car informations such as pulse mode pattern
  ******************************************************************************
  */

#define NUMOFCAR 4

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
	{"0,0,200,200,0,0,\n5.4,45,243,315,0,0,\n7.0,27,189,369.9,0,0,\n13.7,15,205.5,375,0,0,\n25.0,9,225,252,0,0,",//\n28.0,5,140,190,0,0,\n38.0,3,114,144,0,0,",
			2.0, -3.7, -5.3, 5.31, 2, 0.43},
	{"0,0,1050,1050,0,0,\n23,0,1050,700,0,0,\n48,0,700,1800,0,0,\n59,15,885,1035,0,0,",
			2.3, -4.5, -4.5, 6.6, 2, 0.43},
	{"0,0,175,175,0,0,\n1.5,0,196,196,0,0,\n2.0,0,220.5,220.5,0,0,\n2.5,0,233,233,0,0,\n3.0,0,262,262,0,0,\n3.5,0,293.5,293.5,0,0,\n4.0,0,311,311,0,0,\n4.5,0,349.5,349.5,0,0,\n5.0,0,400,400,0,0,\n24.1,18,650.7,718.2,0,0,\n26.6,15,598.5,686.25,0,0,\n30.5,12,549,648,0,0,\n36.0,9,486,594,0,0,",//\n44.0,0,396,468,0,0,",
			3.5, -4.0, -4.5, 5.93, 2, 0.43},
	{"0,0,2000,2000,0,0,\n40,27,1080,1350,0,0,",
			3.3, -4.0, -4.5, 7.2, 2, 0.43}
};

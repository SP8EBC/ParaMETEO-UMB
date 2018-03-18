#ifndef __MS5611_H
#define __MS5611_H

#include <math.h>

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif


void SensorReset(int addr);
int SensorReadCalData(int addr, unsigned short* cal_data);
unsigned char crc4(unsigned short n_prom[]);
long int SensorStartMeas(int param_to_meas);
float SensorBringTemperature(void);
double SensorBringPressure(void);
float CalcQNHFromQFE(float qfe, float alti, float temp);


extern char state;
extern unsigned short SensorCalData[9];


/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif


#ifndef __SERIAL_H
#define __SERIAL_H

extern char srlTXData[128];
extern char srlRXData[128];
extern int srlTXQueueLen;
extern int srlTRXDataCounter;
extern int srlTXing;
extern int srlRXing;
extern int srlRXBytesNum;
extern char srlRxDummy;
extern char srlStartChar;
extern char srlStopChar;
extern int srlIdle;

#ifdef __cplusplus
extern "C" {
#endif

void SrlConfig(void);
void SrlSendData(char* data, char mode, short leng);
void SrlStartTX(short leng);
void USART1_IRQHandler(void);
void SrlReceiveData(int num, char start, char stop, char echo, char len_addr, char len_modifier);

#ifdef __cplusplus
}
#endif


#endif

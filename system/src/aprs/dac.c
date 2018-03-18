//
//#include "aprs/dac.h"
//
//#include <aprs/adc.h>
//
//Afsk *dac_afsk;
//
/////*********************************************************************************************************************/
////void TIM2_IRQHandler(void) {
/////*********************************************************************************************************************/
////
//////	TIM2->SR = ~TIM_IT_Update;
//////
//////	DAC->DHR8R1 = AFSK_DAC_ISR(dac_afsk);
//////	DAC->SWTRIGR |= 1;
////
////}
//
///*********************************************************************************************************************/
//void DA_Init(Afsk *af) {
///*********************************************************************************************************************/
//
//
////	GPIO_InitTypeDef GPIO_InitStructure;
////	DAC_InitTypeDef	DAC_InitStructure;
////	NVIC_InitTypeDef NVIC_InitStructure;
////
////	//Dolacz zegar do GPIOA
////	RCC->APB2ENR |= RCC_APB2Periph_GPIOA;
////
////	//wlaczenie sygnalu zegarowego DAC
////	RCC->APB1ENR |= RCC_APB1Periph_DAC;
////
////	//wlacznie sygnalu zegarowego Timer2
////	RCC->APB1ENR |= RCC_APB1Periph_TIM2;
////
////	//Konfiguracja Portu
////	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
////	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
////	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
////	GPIO_Init(GPIOA, &GPIO_InitStructure);
////
////
////	//DeInit DAC
////	RCC->APB1RSTR |= RCC_APB1Periph_DAC;
////	RCC->APB1RSTR &= ~RCC_APB1Periph_DAC;
////
////	DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
////	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
////	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
////	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
////
////	DAC_Cmd(DAC_Channel_1, ENABLE);
////
////	DAC->DHR8R1 = 0;
////	DAC->SWTRIGR |= 1;
////
////	dac_afsk = af;
////
////	//ustawienie przerwanie
////	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
////	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
////	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
////	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
////	NVIC_Init(&NVIC_InitStructure);
//
//}
//
///*********************************************************************************************************************/
//void DA_SetTimer(uint16_t prescaler, uint16_t period) {
///*********************************************************************************************************************/
//
////	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
////
////	//DeInit Timer2
////	RCC->APB1RSTR |= RCC_APB1Periph_TIM2;
////	RCC->APB1RSTR &= ~RCC_APB1Periph_TIM2;
////
////	TIM_TimeBaseStructure.TIM_Period = period - 1;
////	TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;
////	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
////	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
////	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0 ;
////	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
////
////	//uruchomienie przerwania w licnzku
////	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
//
//}
//
///*********************************************************************************************************************/
//void DA_Start() {
///*********************************************************************************************************************/
//
//	AD_Stop();
////
////	//stm32_gpioPinWrite(PTT_GPIO_BASE, PTT_PIN, 1);
////	GPIO_SetBits(GPIOB, GPIO_Pin_11);
//
//	GPIOC->BSRR |= GPIO_BSRR_BR3;	 /// bez sep
////	GPIOC->BSRR |= GPIO_BSRR_BS3;	 //// sep
////	GPIOC->BSRR |= GPIO_BSRR_BS8;
////
////	//Timer2 ENABLE
//	TIM4->CR1 |= TIM_CR1_CEN;
//
//}
//
///*********************************************************************************************************************/
//void DA_Stop() {
///*********************************************************************************************************************/
//
//	AD_Start();
////	//stm32_gpioPinWrite(PTT_GPIO_BASE, PTT_PIN, 0);
////	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
////
////	GPIOC->BSRR |= GPIO_BSRR_BR3; /// sep
//	GPIOC->BSRR |= GPIO_BSRR_BS3; //// bez sep
////	GPIOC->BSRR |= GPIO_BSRR_BR8;
//
////	//Timer2 DISABLE
//	TIM4->CR1 &= ~TIM_CR1_CEN;
//
//}

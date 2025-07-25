#include "stm32l1xx.h"
#include "stm32l1xx_ll_system.h"
#include "stm32l1xx_ll_bus.h"
#include "stm32l1xx_ll_utils.h"
#include "stm32l1xx_ll_rcc.h"
#include "stm32l1xx_ll_pwr.h"
#include "stm32l1xx_ll_gpio.h"
#include "stm32l1xx_ll_lcd.h"
#include "stm32l152_glass_lcd.h"
#include "stm32l1xx_ll_tim.h"
#include "stm32l1xx_ll_adc.h"
#include "math.h"

#define E				(uint16_t)659	
#define TIMx_PSC			3 
#define ARR_CALCULATE(N) ((32000000) / ((TIMx_PSC) * (N)))

void SystemClock_Config(void);

void people_in(void); //check when people in
void int_to_arr(int);
void display_7seg (void); //show 7seg
void adc_capture(void);
void ultra_capture(void);
	
void S7_seg_con(void);
void buzz_con(void); //PB5 buzz   TIM3_CH2
void ultra_con(void); //PA1 Echo  PA2 Trig  use Timer 4 for count second
void LDR_con (void);
void PA5_adc_con(void);

void ultra_tim2_con(void); //use for let ultra get distance 
void tim3_con(uint16_t); //buzzer
void tim3_buzz(uint16_t); // compare PWM mode for buzz
void tim4_con(void); // count second 

void TIM2_IRQHandler(void); // work process ultrasonic
void TIM4_IRQHandler (void); //count second for ultra

uint16_t uwRI = 0;
uint16_t uwFA = 0;
uint16_t uwDiff = 0;
uint16_t uhICIndex = 0;

uint32_t TIM2CLK;
uint32_t PSC;
uint32_t IC1PSC;

uint32_t ditg_to_seg[10] = {LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14 , //0
															LL_GPIO_PIN_10 | LL_GPIO_PIN_11 , //1
															LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_15 , //2
															LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_15 , //3
															LL_GPIO_PIN_14 | LL_GPIO_PIN_15 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11, //4 
															LL_GPIO_PIN_2 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15 , //5
															LL_GPIO_PIN_2 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13  | LL_GPIO_PIN_14 | LL_GPIO_PIN_15 , //6
															LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 ,//7
															LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15,//8
															LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15 }; //9
uint32_t digit[4] = {LL_GPIO_PIN_0 , LL_GPIO_PIN_1 , LL_GPIO_PIN_2 , LL_GPIO_PIN_3 };
uint32_t d[4] = {0, 0, 0, 0};

float period = 0.0;
int time_count = 0;
int adc = 0;

int main(void){
  SystemClock_Config();
	ultra_tim2_con();
	S7_seg_con();
	PA5_adc_con();
	tim4_con();
	
	while(1){
			ultra_capture();
			people_in();
			display_7seg();
		
	}
}

void people_in(void){
	if(period > 2 && period < 3.9){
		if(LL_GPIO_IsOutputPinSet(GPIOA, LL_GPIO_PIN_3) == RESET){
				LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);
				LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_3);
			  tim3_buzz(ARR_CALCULATE(E));
		    LL_mDelay(200);
		    LL_TIM_DisableCounter(TIM3);
				time_count = 0;
			  adc = 0000;
		}
		else 
				time_count = 0;
	}
}

void int_to_arr(int num){
		d[3] = num % 10;
		d[2] = (num/10) % 10;
		d[1] = (num/100) % 10;
		d[0] = num/1000;
}

void display_7seg (void){
		int_to_arr(adc);
		for(uint8_t i = 0; i <4 ; i++){
				LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 |  LL_GPIO_PIN_3);
				LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_2 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 
															| LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15);
			
				LL_GPIO_SetOutputPin(GPIOB, ditg_to_seg[d[i]]);
				LL_GPIO_SetOutputPin(GPIOC, digit[i]);
			
				LL_mDelay(1);
			}
}

void adc_capture(void){
		LL_ADC_REG_StartConversionSWStart(ADC1);
		while(LL_ADC_IsActiveFlag_EOCS(ADC1) == 0);
		adc = LL_ADC_REG_ReadConversionData10(ADC1);
		
		
		if(adc < 100) 
			LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);
		else
			LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

void ultra_capture(void){
			LL_GPIO_SetOutputPin(GPIOA , LL_GPIO_PIN_2);
			LL_mDelay(1);
			LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_2);

			if(uhICIndex == 2) {
				PSC = LL_TIM_GetPrescaler(TIM2) + 1;
				TIM2CLK = SystemCoreClock / PSC;
				IC1PSC = __LL_TIM_GET_ICPSC_RATIO(LL_TIM_IC_GetPrescaler(TIM2, LL_TIM_CHANNEL_CH2));
			
				period = ((uwDiff *340.0*pow(10,2))/2.0) / SystemCoreClock;
			
				uhICIndex = 0;
			}
}

void S7_seg_con(void){
	LL_GPIO_InitTypeDef seg;
	
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
	seg.Mode = LL_GPIO_MODE_OUTPUT;
	seg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	seg.Pull = LL_GPIO_PULL_NO;
	seg.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	seg.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 
								| LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15;
	LL_GPIO_Init(GPIOB , &seg);
	
	seg.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3;
	LL_GPIO_Init(GPIOC , &seg);
}

void buzz_con (void){
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_GPIO_InitTypeDef buzz;
	
	buzz.Mode = LL_GPIO_MODE_OUTPUT;
	buzz.Pull = LL_GPIO_PULL_NO;
	buzz.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	buzz.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	buzz.Pin = LL_GPIO_PIN_4;
	
	LL_GPIO_Init(GPIOA, &buzz);
	
	buzz.Pin = LL_GPIO_PIN_3;
	
	LL_GPIO_Init(GPIOA, &buzz);
	
	
	buzz.Mode = LL_GPIO_MODE_ALTERNATE;
	buzz.Alternate = LL_GPIO_AF_2;
	buzz.Pin = LL_GPIO_PIN_5;
	
	LL_GPIO_Init(GPIOB, &buzz);
}

void ultra_con(void){
	LL_GPIO_InitTypeDef timic_gp;
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	
	timic_gp.Mode = LL_GPIO_MODE_ALTERNATE;
	timic_gp.Pull = LL_GPIO_PULL_NO;
	timic_gp.Pin = LL_GPIO_PIN_1 ;
	timic_gp.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	timic_gp.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	timic_gp.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(GPIOA, &timic_gp);
	
	
	timic_gp.Mode = LL_GPIO_MODE_OUTPUT;
	timic_gp.Pin = LL_GPIO_PIN_2;
	LL_GPIO_Init(GPIOA, &timic_gp);
	
	
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	timic_gp.Mode = LL_GPIO_MODE_OUTPUT;
	timic_gp.Pin = LL_GPIO_PIN_4;
	LL_GPIO_Init(GPIOA, &timic_gp);
	
	timic_gp.Pin = LL_GPIO_PIN_3;
	LL_GPIO_Init(GPIOA, &timic_gp);

}
void LDR_con (void){
	
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA); //for ldr
	LL_GPIO_InitTypeDef ldr;
	
	ldr.Mode = LL_GPIO_MODE_ANALOG;
	ldr.Pin = LL_GPIO_PIN_5;
	ldr.Pull = LL_GPIO_PULL_NO;
	ldr.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	ldr.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	
	LL_GPIO_Init(GPIOA, &ldr); // ldr con 
}

void PA5_adc_con(void){
	LL_ADC_InitTypeDef adc_con;
	LDR_con();
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);
	
	adc_con.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
	adc_con.Resolution = LL_ADC_RESOLUTION_10B;
	adc_con.LowPowerMode = LL_ADC_LP_AUTOWAIT_NONE;
	adc_con.SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE;
	LL_ADC_Init(ADC1, &adc_con);
	
	LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_5);
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_5, LL_ADC_SAMPLINGTIME_48CYCLES);
	
	LL_ADC_Enable(ADC1);
}

void ultra_tim2_con(void){
	ultra_con();
	LL_TIM_IC_InitTypeDef timic;
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
	
	timic.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
	timic.ICFilter = LL_TIM_IC_FILTER_FDIV1_N2;
	timic.ICPolarity = LL_TIM_IC_POLARITY_BOTHEDGE;
	timic.ICPrescaler = LL_TIM_ICPSC_DIV1;
	LL_TIM_IC_Init(TIM2, LL_TIM_CHANNEL_CH2, &timic);

	
	NVIC_SetPriority(TIM2_IRQn, 0);
	NVIC_EnableIRQ(TIM2_IRQn);
	LL_TIM_EnableIT_CC2(TIM2);
	LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);		
	
	LL_TIM_EnableCounter(TIM2);
}

void tim3_con(uint16_t ARR){
	LL_TIM_InitTypeDef timbase_initstructure;
	
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
	//Time-base configure
	timbase_initstructure.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	timbase_initstructure.CounterMode = LL_TIM_COUNTERMODE_UP;
	timbase_initstructure.Autoreload = ARR - 1;
	timbase_initstructure.Prescaler =  TIMx_PSC- 1;
	LL_TIM_Init(TIM3, &timbase_initstructure);
	
	LL_TIM_EnableCounter(TIM3);
}

void tim3_buzz(uint16_t note){
	LL_TIM_OC_InitTypeDef tim_oc;
	
	buzz_con();
	tim3_con(note);
	
	tim_oc.OCState = LL_TIM_OCSTATE_DISABLE;
	tim_oc.OCMode = LL_TIM_OCMODE_PWM1;
	tim_oc.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	tim_oc.CompareValue = LL_TIM_GetAutoReload(TIM3) /2; 
	LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH2, &tim_oc);
	/*Start Output Compare in PWM Mode*/
	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH2);
	LL_TIM_EnableCounter(TIM3);
}

void tim4_con(void){
	LL_TIM_InitTypeDef timbase_initstructure;
	
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
	//Time-base configure
	timbase_initstructure.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	timbase_initstructure.CounterMode = LL_TIM_COUNTERMODE_UP;
	timbase_initstructure.Autoreload = 3200 - 1;
	timbase_initstructure.Prescaler =  10000 - 1;
	LL_TIM_Init(TIM4, &timbase_initstructure);
	
	LL_TIM_EnableIT_UPDATE(TIM4);
	
	NVIC_SetPriority(TIM4_IRQn, 0);
	NVIC_EnableIRQ(TIM4_IRQn);
	
	LL_TIM_EnableCounter(TIM4);
}

void TIM2_IRQHandler(void){
	if(LL_TIM_IsActiveFlag_CC2(TIM2) == SET){
		LL_TIM_ClearFlag_CC2(TIM2);
		if(uhICIndex == 0){
			uwRI = LL_TIM_IC_GetCaptureCH2(TIM2);
			uhICIndex = 1;
		}
		else if(uhICIndex == 1){
			uwFA = LL_TIM_IC_GetCaptureCH2(TIM2);
		
		if(uwFA > uwRI)
			uwDiff = uwFA-uwRI;
		else if(uwFA < uwRI)
			uwDiff = ((LL_TIM_GetAutoReload(TIM2) - uwRI) + uwFA) +1;
 		uhICIndex = 2;
		}
	}
}

void TIM4_IRQHandler (void){
	if(LL_TIM_IsActiveFlag_UPDATE(TIM4) == SET){
		LL_TIM_ClearFlag_UPDATE(TIM4);
		time_count++;
		if(time_count > 5 && LL_GPIO_IsOutputPinSet(GPIOA, LL_GPIO_PIN_3)== SET){
			time_count = 0 ;
			LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_3);
		}
		else if(time_count == 10){
			adc_capture();
			tim3_buzz(ARR_CALCULATE(E));
		  LL_mDelay(500);
		  LL_TIM_DisableCounter(TIM3);
		}
		else if(time_count > 10){
			adc_capture();
		}
	}
	LL_TIM_SetCounter(TIM4, 0);
}


void SystemClock_Config(void){
  /* Enable ACC64 access and set FLASH latency */ 
  LL_FLASH_Enable64bitAccess();; 
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

  /* Set Voltage scale1 as MCU will run at 32MHz */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  
  /* Poll VOSF bit of in PWR_CSR. Wait until it is reset to 0 */
  while (LL_PWR_IsActiveFlag_VOSF() != 0)
  {
  };
  
  /* Enable HSI if not already activated*/
  if (LL_RCC_HSI_IsReady() == 0)
  {
    /* HSI configuration and activation */
    LL_RCC_HSI_Enable();
    while(LL_RCC_HSI_IsReady() != 1)
    {
    };
  }
  
	
  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLL_MUL_6, LL_RCC_PLL_DIV_3);

  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  };
  
  /* Sysclk activation on the main PLL */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  };
  
  /* Set APB1 & APB2 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  /* Set systick to 1ms in using frequency set to 32MHz                             */
  /* This frequency can be calculated through LL RCC macro                          */
  /* ex: __LL_RCC_CALC_PLLCLK_FREQ (HSI_VALUE, LL_RCC_PLL_MUL_6, LL_RCC_PLL_DIV_3); */
  LL_Init1msTick(32000000);
  
  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(32000000);

}
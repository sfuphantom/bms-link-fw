/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
#include "ADC_CurntTrans.h"
#include "adc.h"
/* USER CODE END */

/* USER CODE BEGIN (2) */
/* USER CODE END */


void main(void)
{
/* USER CODE BEGIN (3) */
    adcInit(); // Init the ADC


    while(1)
    {  
        float current = get_currentOnce(ADC_MODULE, adcGROUP1);
    };
}
/* USER CODE END */



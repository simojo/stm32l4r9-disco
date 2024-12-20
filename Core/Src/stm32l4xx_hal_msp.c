/**
  ******************************************************************************
  * @file    DCMI/DCMI_CaptureMode/Src/stm32l4xx_hal_msp.c
  * @author  MCD Application Team
  * @brief   HAL MSP module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32L4xx_HAL_Examples
  * @{
  */

/** @defgroup HAL_MSP
  * @brief HAL MSP module.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
DMA_HandleTypeDef hdma_M2M;
DMA_HandleTypeDef hdma_eval;

void DMA_DCMI_M2M_Transfer_Complete(DMA_HandleTypeDef *hdma);
void DMA_DCMI_M2M_Transfer_Error(DMA_HandleTypeDef *hdma);
void show_to_lcd(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief LTDC MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
  RCC_PeriphCLKInitTypeDef  PeriphClkInit;

  /* Enable the LTDC clock */
  __HAL_RCC_LTDC_CLK_ENABLE();

  /* Reset of LTDC IP */
  __HAL_RCC_LTDC_FORCE_RESET();
  __HAL_RCC_LTDC_RELEASE_RESET();

  /* Configure the clock for the LTDC */
  /* We want DSI PHI at 500MHz */
  /* We have only one line => 500Mbps */
  /* With 24bits per pixel, equivalent PCLK is 500/24 = 20.8MHz */
  /* We will set PCLK at 15MHz */
  /* Following values are OK with MSI = 4MHz */
  /* (4*60)/(1*4*4) = 15MHz */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInit.PLLSAI2.PLLSAI2Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI2.PLLSAI2M = 1;
  PeriphClkInit.PLLSAI2.PLLSAI2N = 60;
  PeriphClkInit.PLLSAI2.PLLSAI2R = RCC_PLLR_DIV4;
  PeriphClkInit.LtdcClockSelection = RCC_LTDCCLKSOURCE_PLLSAI2_DIV4;
  PeriphClkInit.PLLSAI2.PLLSAI2ClockOut = RCC_PLLSAI2_LTDCCLK;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    while(1);
  }

  /* NVIC configuration for LTDC interrupts that are now enabled */
  HAL_NVIC_SetPriority(LTDC_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);
  HAL_NVIC_SetPriority(LTDC_ER_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(LTDC_ER_IRQn);
}

/**
  * @brief DSI MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  * @param hdsi: DSI handle pointer
  * @retval None
  */
void HAL_DSI_MspInit(DSI_HandleTypeDef *hdsi)
{
  RCC_OscInitTypeDef        OscInitStruct;

  /* Enable DSI Host and wrapper clocks */
  __HAL_RCC_DSI_CLK_ENABLE();

  /* Reset the DSI Host and wrapper */
  __HAL_RCC_DSI_FORCE_RESET();
  __HAL_RCC_DSI_RELEASE_RESET();

  /* Enable HSE used for DSI PLL */
  HAL_RCC_GetOscConfig(&OscInitStruct);
  if(OscInitStruct.HSEState == RCC_HSE_OFF)
  {
    /* Workaround for long HSE startup time (set PH0 to output PP low) */
    GPIO_InitTypeDef  GPIO_InitStruct;

    __HAL_RCC_GPIOH_CLK_ENABLE();
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_0, GPIO_PIN_RESET);

    OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    OscInitStruct.HSEState       = RCC_HSE_ON;
    OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&OscInitStruct) != HAL_OK)
    {
      while(1);
    }
  }

  /* NVIC configuration for DSI interrupt that is now enabled */
  HAL_NVIC_SetPriority(DSI_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(DSI_IRQn);
}

/**
  * @brief DMA2D MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  * @param Dma2dHandle: DMA2D handle pointer
  * @retval None
  */
void HAL_DMA2D_MspInit(DMA2D_HandleTypeDef *Dma2dHandle)
{
  /* Enable the DMA2D clock */
  __HAL_RCC_DMA2D_CLK_ENABLE();

  /* Reset of DMA2D IP */
  __HAL_RCC_DMA2D_FORCE_RESET();
  __HAL_RCC_DMA2D_RELEASE_RESET();
}

/**
  * @brief  Initialize the DCMI MSP.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
void HAL_DCMI_MspInit(DCMI_HandleTypeDef* hdcmi)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMAMUX1_CLK_ENABLE();

  /* Set the parameters to be configured */
  hdma_M2M.Init.Request             = DMA_REQUEST_MEM2MEM;
  hdma_M2M.Init.Direction           = DMA_MEMORY_TO_MEMORY;
  hdma_M2M.Init.PeriphInc           = DMA_PINC_ENABLE;
  hdma_M2M.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_M2M.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_M2M.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
  hdma_M2M.Init.Mode                = DMA_NORMAL;
  hdma_M2M.Init.Priority            = DMA_PRIORITY_HIGH;

  hdma_M2M.Instance = DMA1_Channel5;

  __HAL_LINKDMA(hdcmi, DMAM2M_Handle, hdma_M2M);

  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0x0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

   /* Deinitialize the Channel for new transfer */
  HAL_DMA_DeInit(&hdma_M2M);

  /* Configure the DMA Channel */
  HAL_DMA_Init(&hdma_M2M);

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);

  /*** Configure dma for internal sram to external sram data transfer ***/
  hdma_M2M.XferCpltCallback = DMA_DCMI_M2M_Transfer_Complete;
  hdma_M2M.XferErrorCallback = DMA_DCMI_M2M_Transfer_Error;
}

void DMA_DCMI_M2M_Transfer_Complete(DMA_HandleTypeDef *hdma)
{
  show_to_lcd();
}

void DMA_DCMI_M2M_Transfer_Error(DMA_HandleTypeDef *hdma)
{
  BSP_LED_On(LED1);
  BSP_LED_On(LED2);
}


/**
  * @}
  */

/**
  * @}
  */


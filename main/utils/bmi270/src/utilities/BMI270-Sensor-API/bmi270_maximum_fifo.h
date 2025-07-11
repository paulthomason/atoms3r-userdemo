/*
* SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
*
* SPDX-License-Identifier: MIT
*/

/**
 * \ingroup bmi2xy
 * \defgroup bmi270_maximum_fifo BMI270_MAXIMUM_FIFO
 * @brief Sensor driver for BMI270_MAXIMUM_FIFO sensor
 */

#ifndef BMI270_MAXIMUM_FIFO_H_
#define BMI270_MAXIMUM_FIFO_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/

/*!             Header files
 ****************************************************************************/
#include "bmi2.h"

/***************************************************************************/

/*!               Macro definitions
 ****************************************************************************/

/*! @name BMI270 Chip identifier */
#define BMI270_MAXIMUM_FIFO_CHIP_ID       UINT8_C(0x24)

/*! @name Defines maximum number of pages */
#define BMI270_MAXIMUM_FIFO_MAX_PAGE_NUM  UINT8_C(0)

/*! @name Defines maximum number of feature input configurations */
#define BMI270_MAXIMUM_FIFO_MAX_FEAT_IN   UINT8_C(0)

/*! @name Defines maximum number of feature outputs */
#define BMI270_MAXIMUM_FIFO_MAX_FEAT_OUT  UINT8_C(0)

/*! @name Mask definitions for feature interrupt status bits */

/***************************************************************************/

/*!     BMI270 User Interface function prototypes
 ****************************************************************************/

/**
 * \ingroup bmi270_maximum_fifo
 * \defgroup bmi270_maximum_fifoApiInit Initialization
 * @brief Initialize the sensor and device structure
 */

/*!
 * \ingroup bmi270_maximum_fifoApiInit
 * \page bmi270_maximum_fifo_api_bmi270_maximum_fifo_init bmi270_maximum_fifo_init
 * \code
 * int8_t bmi270_maximum_fifo_init(struct bmi2_dev *dev);
 * \endcode
 * @details This API:
 *  1) updates the device structure with address of the configuration file.
 *  2) Initializes BMI270 sensor.
 *  3) Writes the configuration file.
 *  4) Updates the feature offset parameters in the device structure.
 *  5) Updates the maximum number of pages, in the device structure.
 *
 * @param[in, out] dev      : Structure instance of bmi2_dev.
 *
 * @return Result of API execution status
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int8_t bmi270_maximum_fifo_init(struct bmi2_dev *dev);

/******************************************************************************/
/*! @name       C++ Guard Macros                                      */
/******************************************************************************/
#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* BMI270_MAXIMUM_FIFO_H_ */

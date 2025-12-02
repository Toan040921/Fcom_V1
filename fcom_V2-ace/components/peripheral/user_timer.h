/*
 * UserTimer.h
 *
 *  Created on: Mar 6, 2019
 *      Author: giangnv3
 */

#ifndef MAIN_INTERFACE_USERTIMER_H_
#define MAIN_INTERFACE_USERTIMER_H_


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"

#define TIMER_INTERVAL			1000 /*1ms*/

void UserTimer_Init(void);
uint32_t usertimer_gettick( void );

#endif /* MAIN_INTERFACE_USERTIMER_H_ */

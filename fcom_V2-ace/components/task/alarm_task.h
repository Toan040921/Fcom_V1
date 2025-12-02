/*
 * plan_task.h
 *
 *  Created on: Jan 7, 2021
 *      Author: delvin
 */

#ifndef ALARM_TASK_H_
#define ALARM_TASK_H_
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "../../common_interface.h"
#include "../user_driver/flash/user_flash.h"
#include "hmi_service.h"
#include <stdlib.h>
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
// ARLAM Manager
#define HEART_BEAT_TIME 0x141DD76000 // 24*60*60*1000*1000=24h
 //#define HEART_BEAT_TIME 30*1000*1000 //  30s

struct dtor {
    int id;
    int64_t timestamp;
    struct dtor *next;
};

void push_state(struct dtor **list, int id, int64_t timestamp);

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
struct dtor *get_head(void);
void set_head(struct dtor **list);
void alarm_task(void);
void print_state(struct dtor **list);
void pop_state(struct dtor **list, int id);
void add_state(struct dtor **list, int id, int64_t timestamp);
#endif /* ALARM_TASK_H_ */

#ifndef __RyanMqttThread__
#define __RyanMqttThread__

#ifdef __cplusplus
extern "C"
{
#endif

#include "RyanMqttLog.h"
#include "RyanMqttUtile.h"
#include "RyanMqttClient.h"

// 定义枚举类型

// 定义结构体类型

/* extern variables-----------------------------------------------------------*/
extern void RyanMqttThread(void *argument);
extern void RyanMqttEventMachine(RyanMqttClient_t *client, RyanMqttEventId_e eventId, void *eventData);
extern void RyanMqttRefreshKeepaliveTime(RyanMqttClient_t *client);

#ifdef __cplusplus
}
#endif

#endif

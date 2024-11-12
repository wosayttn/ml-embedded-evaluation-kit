

#ifndef __mqttClient__
#define __mqttClient__

#ifdef __cplusplus
extern "C"
{
#endif

#include "MQTTPacket.h"
#include "RyanList.h"
#include "RyanMqttPublic.h"
#include "platformTimer.h"
#include "platformSystem.h"
#include "platformNetwork.h"

// Callback function type for receiving subscription messages, do not modify eventData, or the mqtt client may crash
typedef void (*RyanMqttEventHandle)(void *client, RyanMqttEventId_e event, const void const *eventData);

// Enumeration type definition

// Structure type definition
typedef struct
{
    uint8_t retained;    // Retained flag
    uint8_t dup;         // Duplicate flag
    uint16_t packetId;   // Packet identifier, system-generated
    RyanMqttQos_e qos;   // Quality of Service level
    uint32_t payloadLen; // Data length
    char *topic;         // Topic information
    char *payload;       // Data content
} RyanMqttMsgData_t;

typedef struct
{
    RyanMqttQos_e qos; // QoS level
    RyanList_t list;   // Linked list node, do not modify
    char *topic;       // Topic
} RyanMqttMsgHandler_t;

typedef struct
{
    uint16_t repeatCount;             // Current ack timeout resend count
    uint16_t packetId;                // Packet identifier, system-generated, do not modify
    uint32_t packetLen;               // Packet length
    enum msgTypes packetType;         // Expected ack message type
    RyanList_t list;                  // Linked list node, do not modify
    platformTimer_t timer;            // Ack timeout timer, do not modify
    RyanMqttMsgHandler_t *msgHandler; // Message information
    char *packet;                     // Original packet to be resent if the expected ack is not received
} RyanMqttAckHandler_t;

typedef struct
{
    uint8_t retain;      // Retained flag for Will message
    uint32_t payloadLen; // Message length
    RyanMqttQos_e qos;   // Will message QoS level
    char *topic;         // Will message topic
    char *payload;       // Will message content
} lwtOptions_t;

typedef struct
{
    char *clientId;                        // Client ID
    char *userName;                        // Username
    char *password;                        // Password
    char *host;                            // MQTT server address
    char *port;                            // MQTT server port
    char *taskName;                        // Thread name
    char *recvBuffer;                      // MQTT receive buffer
    char *sendBuffer;                      // MQTT send buffer
    uint8_t autoReconnectFlag : 1;         // Auto-reconnect flag
    uint8_t cleanSessionFlag : 1;          // Clean session flag
    uint8_t mqttVersion : 4;               // MQTT version, 3.1.1 is 4, 3.1 is 3
    uint16_t ackHandlerRepeatCountWarning; // Event callback trigger count when ack resend exceeds this value, choose according to actual hardware. Typical value is * ackTimeout ~= 300 seconds
    uint16_t taskPrio;                     // MQTT thread priority
    uint16_t taskStack;                    // Thread stack size
    uint16_t recvTimeout;                  // MQTT wait for receive command timeout, choose according to actual hardware. Recommended > ackTimeout && <= (keepaliveTimeoutS / 2)
    uint16_t sendTimeout;                  // MQTT send command timeout, choose according to actual hardware.
    uint16_t ackTimeout;                   // MQTT ack wait command timeout, typical value is 5 - 30
    uint16_t keepaliveTimeoutS;            // MQTT heartbeat interval in seconds
    uint16_t ackHandlerCountWarning;       // Warning count for waiting ack. Trigger event callback when the total ack count is greater than or equal to this value. Choose according to actual hardware. Typical value is 32
    uint32_t recvBufferSize;               // MQTT receive buffer size
    uint32_t sendBufferSize;               // MQTT send buffer size
    uint16_t reconnectTimeout;             // MQTT reconnect interval time
    RyanMqttEventHandle mqttEventHandle;   // MQTT event callback function
    void *userData;                        // User-defined data, the user needs to ensure the persistence of the pointer pointing to the content
} RyanMqttClientConfig_t;

typedef struct
{
    uint8_t lwtFlag : 1;               // Will flag
    uint8_t destoryFlag : 1;           // Destroy flag
    uint8_t keepaliveTimeoutCount : 6; // Heartbeat timeout counter
    uint16_t ackHandlerCount;          // Number of ack records waiting
    uint16_t packetId;                 // MQTT message identifier, control messages must include a non-zero 16-bit message identifier
    uint32_t eventFlag;                // Event flag
    RyanMqttState_e clientState;       // MQTT client state
    RyanList_t msgHandlerList;         // Maintain message processing list, this is a must-implement content of the MQTT protocol. All publish messages from the server will be processed (provided the corresponding messages are subscribed to or interceptors are set)
    RyanList_t ackHandlerList;         // Maintain ack list
    platformTimer_t keepaliveTimer;    // Keepalive timer
    platformNetwork_t *network;        // Network component
    RyanMqttClientConfig_t *config;    // MQTT config
    platformThread_t *mqttThread;      // MQTT thread
    platformMutex_t *sendBufLock;      // Write buffer lock
    lwtOptions_t *lwtOptions;          // Will related configuration
} RyanMqttClient_t;

    /* extern variables-----------------------------------------------------------*/

    extern RyanMqttError_e RyanMqttInit(RyanMqttClient_t **pClient);
    extern RyanMqttError_e RyanMqttDestroy(RyanMqttClient_t *client);
    extern RyanMqttError_e RyanMqttStart(RyanMqttClient_t *client);
    extern RyanMqttError_e RyanMqttDisconnect(RyanMqttClient_t *client, RyanMqttBool_e sendDiscFlag);
    extern RyanMqttError_e RyanMqttReconnect(RyanMqttClient_t *client);
    extern RyanMqttError_e RyanMqttSubscribe(RyanMqttClient_t *client, char *topic, RyanMqttQos_e qos);
    extern RyanMqttError_e RyanMqttUnSubscribe(RyanMqttClient_t *client, char *topic);
    extern RyanMqttError_e RyanMqttPublish(RyanMqttClient_t *client, char *topic, char *payload, uint32_t payloadLen, RyanMqttQos_e qos, RyanMqttBool_e retain);

    extern RyanMqttState_e RyanMqttGetState(RyanMqttClient_t *client);
    extern RyanMqttError_e RyanMqttGetSubscribe(RyanMqttClient_t *client, RyanMqttMsgHandler_t *msgHandles, int32_t msgHandleSize, int32_t *subscribeNum);
    extern RyanMqttError_e RyanMqttSetConfig(RyanMqttClient_t *client, RyanMqttClientConfig_t *clientConfig);
    extern RyanMqttError_e RyanMqttSetLwt(RyanMqttClient_t *client, char *topicName, char *payload, uint32_t payloadLen, RyanMqttQos_e qos, RyanMqttBool_e retain);

    extern RyanMqttError_e RyanMqttDiscardAckHandler(RyanMqttClient_t *client, enum msgTypes packetType, uint16_t packetId);
    extern RyanMqttError_e RyanMqttRegisterEventId(RyanMqttClient_t *client, RyanMqttEventId_e eventId);
    extern RyanMqttError_e RyanMqttCancelEventId(RyanMqttClient_t *client, RyanMqttEventId_e eventId);

#ifdef __cplusplus
}
#endif

#endif

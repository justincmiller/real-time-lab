/*
 * SensorController.c
 *
 *  Created on: Oct 24, 2022
 *      Author: kadh1
 */


#include <stdio.h>

#include "main.h"
#include "User/Comm_Datalink.h"
#include "User/AcousticSensor.h"
#include "User/DepthSensor.h"
#include "User/SensorPlatform.h"
#include "User/SensorController.h"
#include "User/util.h"
#include "User/common.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

const uint16_t T_ACOUSTIC_MS = 5000U;
const uint16_t T_DEPTH_MS = 2000U;

const TickType_t ACK_TIMEOUT = pdMS_TO_TICKS(2000U);
const TickType_t POLL_INTERVAL = pdMS_TO_TICKS(10U);

typedef enum {
    SYSTEM_INIT,
    START_SENSORS,
    PARSE_DATA,
    DISABLE_SENSORS
} ControllerState;

typedef struct {
    ControllerState state;
    struct SensorStates sensors;
    TickType_t ackTick;
} SensorController;

QueueHandle_t Queue_Sensor_Data = NULL;
QueueHandle_t Queue_HostPC_Data = NULL;

static void ResetMessageStruct(struct CommMessage* currentRxMessage) {
	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

// Set controller state and perform transistion behaviour
static void setControllerState(SensorController *sc, ControllerState state) {
    if (sc->state == state)
        return;

    sc->state = state;
    
    if (state == START_SENSORS)
        sc->ackTick = xTaskGetTickCount();
}

static void resetSensors(SensorController *sc) {
    sc->sensors = (struct SensorStates){0};
}

// Send commands to enable both sensors
static void startSensors(SensorController *sc) {
    resetSensors(sc);

    send_sensorEnable_message(Acoustic, T_ACOUSTIC_MS);
    send_sensorEnable_message(Depth, T_DEPTH_MS);

    setControllerState(sc, START_SENSORS);
    print_str("Enabling sensors\r\n");
}

// Retry sensors that have not acknowledged
static void restartSensors(SensorController *sc) {
    if (!sc->sensors.IsAcousticAck)
        send_sensorEnable_message(Acoustic, T_ACOUSTIC_MS);

    if (!sc->sensors.IsDepthAck)
        send_sensorEnable_message(Depth, T_DEPTH_MS);

    sc->ackTick = xTaskGetTickCount();
    print_str("Retrying sensor enable\r\n");
}

// Process commands received from the host PC
static void pollCommands(SensorController *sc) {
    enum HostPCCommands cmd;

    if (xQueueReceive(Queue_HostPC_Data, &cmd, 0) != pdPASS)
        return;

    switch (cmd) {
    case PC_Command_START:
        if (sc->state == SYSTEM_INIT)
            startSensors(sc);
        break;
    case PC_Command_RESET:
        if (sc->state != SYSTEM_INIT)
            setControllerState(sc, DISABLE_SENSORS);
        break;
    default:
        break;
    }
}

// Record sensor acknowlegements
static void processAck(SensorController *sc, const struct CommMessage *msg) {
    switch (msg->SensorID) {
    case Acoustic:
        sc->sensors.IsAcousticAck = true;
        break;
    case Depth:
        sc->sensors.IsDepthAck = true;
        break;
    default:
        break;
    }
}

// Store and print data
static void processData(SensorController *sc, const struct CommMessage *msg) {
    switch (msg->SensorID) {
    case Acoustic:
        sc->sensors.AcousticData = msg->params;
        print_fmt("Acoustic: %u\r\n", (unsigned)msg->params);
        break;
    case Depth:
        sc->sensors.DepthData = msg->params;
        print_fmt("Depth: %u\r\n", (unsigned)msg->params);
        break;
    default:
        break;
    }
}

// Process sensor messages
static void pollMessages(SensorController *sc) {
    struct CommMessage msg;

    while (xQueueReceive(Queue_Sensor_Data, &msg, 0) == pdPASS) {
        if (!msg.IsMessageReady || !msg.IsCheckSumValid)
            continue;

        switch (sc->state) {
        case START_SENSORS:
            if (msg.messageId == MSG_ACK)
                processAck(sc, &msg);
            break;
        case PARSE_DATA:
            if (msg.messageId == MSG_DATA)
                processData(sc, &msg);
            break;
        default:
            // Discard messages in inactive states.
            break;
        }
    }
}

/******************************************************************************
This task is created from the main.
******************************************************************************/
void SensorControllerTask(void *params) {
    (void)params;

    SensorController sc = {.state = SYSTEM_INIT};

    // Wait for queue initialization
    while (Queue_HostPC_Data == NULL || Queue_Sensor_Data == NULL)
        vTaskDelay(POLL_INTERVAL);

    while (1) {
        pollCommands(&sc);
        pollMessages(&sc);
    
        switch (sc.state) {
        case SYSTEM_INIT:
            break;
        case START_SENSORS:
            if (sc.sensors.IsAcousticAck && sc.sensors.IsDepthAck) {
                setControllerState(&sc, PARSE_DATA);
                print_str("Sensors enabled\r\n");
            } else if ((TickType_t)(xTaskGetTickCount() - sc.ackTick) >= ACK_TIMEOUT) {
                restartSensors(&sc);
            }
            break;
        case PARSE_DATA:
            break;
        case DISABLE_SENSORS:
            send_sensorReset_message();
            resetSensors(&sc);
            setControllerState(&sc, SYSTEM_INIT);
            print_str("Sensors disabled\r\n");
            break;
        }

        vTaskDelay(POLL_INTERVAL);
    }
}

/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void SensorPlatform_RX_Task(void* params) {
    (void)params;

	struct CommMessage currentRxMessage = {0};
	Queue_Sensor_Data = xQueueCreate(80, sizeof(struct CommMessage));
	request_sensor_read();  // requests a usart read (through the callback)

	while(1){
		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){
			xQueueSendToBack(Queue_Sensor_Data, &currentRxMessage, 0);
			ResetMessageStruct(&currentRxMessage);
		}
	}
}

/*
 * This task reads the queue of characters from the Host PC when available
 * It then sends the processed data to the Sensor Controller Task
 */
void HostPC_RX_Task(void* params) {
    (void)params;

	enum HostPCCommands HostPCCommand = PC_Command_NONE;
	Queue_HostPC_Data = xQueueCreate(80, sizeof(enum HostPCCommands));
	request_hostPC_read();

	while (1) {
		HostPCCommand = parse_hostPC_message();
		if (HostPCCommand != PC_Command_NONE)
			xQueueSendToBack(Queue_HostPC_Data, &HostPCCommand, 0);
	}
}

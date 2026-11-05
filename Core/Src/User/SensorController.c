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

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

QueueHandle_t Queue_Sensor_Data;
QueueHandle_t Queue_HostPC_Data;


static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

/******************************************************************************
This task is created from the main.
******************************************************************************/

typedef enum {
    CONTROLLER_INIT,
    CONTROLLER_START_SENSORS,
    CONTROLLER_PARSE_DATA,
    CONTROLLER_DISABLE_SENSORS
} ControllerState;

void SensorControllerTask(void *params) {
    (void)params;

    const uint16_t ACOUSTIC_PERIOD_MS = 5000U;
    const uint16_t DEPTH_PERIOD_MS = 2000U;

    const TickType_t ACK_TIMEOUT = pdMS_TO_TICKS(2000U);
    const TickType_t POLL_PERIOD = pdMS_TO_TICKS(10U);

    ControllerState state = CONTROLLER_INIT;
    struct SensorStates sensors = {0};
    struct CommMessage message;
    enum HostPCCommands command;
    TickType_t startTick = 0;
    char buffer[64];

    while(1) {
        // Receive tasks create these queues at startup.
        if (Queue_HostPC_Data == NULL || Queue_Sensor_Data == NULL) {
            vTaskDelay(POLL_PERIOD);
            continue;
        }

        // Process a command from the host PC, if available.
        if (xQueueReceive(Queue_HostPC_Data, &command, 0) == pdPASS) {
            if (command == PC_Command_START && state == CONTROLLER_INIT) {
                sensors = (struct SensorStates){0};
                send_sensorEnable_message(Acoustic, ACOUSTIC_PERIOD_MS);
                send_sensorEnable_message(Depth, DEPTH_PERIOD_MS);
                startTick = xTaskGetTickCount();
                state = CONTROLLER_START_SENSORS;
                print_str("Enabling sensors\r\n");
            } else if (command == PC_Command_RESET && state != CONTROLLER_INIT) {
                state = CONTROLLER_DISABLE_SENSORS;
            }
        }

        // Process messages received from the sensor platform.
        if (state == CONTROLLER_START_SENSORS || state == CONTROLLER_PARSE_DATA) {
            while (xQueueReceive(Queue_Sensor_Data, &message, 0) == pdPASS) {
                if (!message.IsMessageReady || !message.IsCheckSumValid)
                    continue;

                // Track sensor enable acknowledgements.
                if (state == CONTROLLER_START_SENSORS && message.messageId == 1) {
                    if (message.SensorID == Acoustic) {
                        sensors.IsAcousticAck = true;
                    } else if (message.SensorID == Depth) {
                        sensors.IsDepthAck = true;
                    }
                }

                // Forward measurements to the host PC.
                if (state == CONTROLLER_PARSE_DATA && message.messageId == 3) {
                    if (message.SensorID == Acoustic) {
                        sensors.AcousticData = message.params;
                        snprintf(buffer, sizeof(buffer), "Acoustic: %u\r\n", (unsigned)sensors.AcousticData);
                        print_str(buffer);
                    } else if (message.SensorID == Depth) {
                        sensors.DepthData = message.params;
                        snprintf(buffer, sizeof(buffer), "Depth: %u\r\n", (unsigned)sensors.DepthData);
                        print_str(buffer);
                    }
                }
            }
        } else {
            // Discard messages outside active states.
            while (xQueueReceive(Queue_Sensor_Data, &message, 0) == pdPASS);
        }

        switch (state) {
        case CONTROLLER_INIT:
            // Wait for the START command.
            break;

        case CONTROLLER_START_SENSORS:
            if ((TickType_t)(xTaskGetTickCount() - startTick) >= ACK_TIMEOUT) {
                if (sensors.IsAcousticAck && sensors.IsDepthAck) {
                    state = CONTROLLER_PARSE_DATA;
                    print_str("Sensors enabled\r\n");
                } else {
                    // Retry only the unacknowledged sensors.
                    if (!sensors.IsAcousticAck)
                        send_sensorEnable_message(Acoustic, ACOUSTIC_PERIOD_MS);

                    if (!sensors.IsDepthAck)
                        send_sensorEnable_message(Depth, DEPTH_PERIOD_MS);

                    startTick = xTaskGetTickCount();
                    print_str("Retrying sensor enable\r\n");
                }
            }
            break;

        case CONTROLLER_PARSE_DATA:
            // Measurements are handled above.
            break;

        case CONTROLLER_DISABLE_SENSORS:
            send_sensorReset_message();
            sensors = (struct SensorStates){0};

            print_str("Sensors disabled\r\n");
            state = CONTROLLER_INIT;
            break;
        }

        vTaskDelay(POLL_PERIOD);
    }
}




/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void SensorPlatform_RX_Task(){
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
void HostPC_RX_Task(){

	enum HostPCCommands HostPCCommand = PC_Command_NONE;

	Queue_HostPC_Data = xQueueCreate(80, sizeof(enum HostPCCommands));

	request_hostPC_read();

	while(1){
		HostPCCommand = parse_hostPC_message();

		if (HostPCCommand != PC_Command_NONE){
			xQueueSendToBack(Queue_HostPC_Data, &HostPCCommand, 0);
		}

	}
}

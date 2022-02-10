/*
 * FreeRTOS V1.4.7
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

#include "iot_config.h"

/* FreeRTOS includes. */

#include "FreeRTOS.h"
#include "task.h"

/* Demo includes */
#include "aws_demo.h"
#include "aws_dev_mode_key_provisioning.h"

/* AWS System includes. */
#include "bt_hal_manager.h"
#include "iot_system_init.h"
#include "iot_logging_task.h"

#include "nvs_flash.h"
#if !AFR_ESP_LWIP
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#endif

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_interface.h"
#include "esp_bt.h"
#if CONFIG_NIMBLE_ENABLED == 1
    #include "esp_nimble_hci.h"
#else
    #include "esp_gap_ble_api.h"
    #include "esp_bt_main.h"
#endif
#include "esp_log.h"
#include "esp_log_internal.h"

#include "driver/uart.h"
#include "aws_application_version.h"
#include "tcpip_adapter.h"

#include "iot_network_manager_private.h"

#include "iot_uart.h"

#if BLE_ENABLED
    #include "bt_hal_manager_adapter_ble.h"
    #include "bt_hal_manager.h"
    #include "bt_hal_gatt_server.h"

    #include "iot_ble.h"
    #include "iot_ble_config.h"
    #include "iot_ble_wifi_provisioning.h"
    #include "iot_ble_numericComparison.h"
#endif

// Include other necessary files here
#include "spi_pn532.h"

/* Logging Task Defines. */
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 32 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 4 )
#define mainDEVICE_NICK_NAME                "Espressif_Demo"

// static const char *TAG = "APP";
static pn532_t nfc;

/* Static arrays for FreeRTOS+TCP stack initialization for Ethernet network connections
 * are use are below. If you are using an Ethernet connection on your MCU device it is
 * recommended to use the FreeRTOS+TCP stack. The default values are defined in
 * FreeRTOSConfig.h. */

/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void );

#if BLE_ENABLED
/* Initializes bluetooth */
    static esp_err_t prvBLEStackInit( void );
    /** Helper function to teardown BLE stack. **/
    esp_err_t xBLEStackTeardown( void );
#endif

IotUARTHandle_t xConsoleUart;


static void iot_uart_init( void )
{
    IotUARTConfig_t xUartConfig;
    int32_t status = IOT_UART_SUCCESS;
    
    xConsoleUart = iot_uart_open( UART_NUM_0 );
    configASSERT( xConsoleUart );
    
    status = iot_uart_ioctl( xConsoleUart, eUartGetConfig, &xUartConfig );
    configASSERT( status == IOT_UART_SUCCESS );
    
    xUartConfig.ulBaudrate = 115200;
    xUartConfig.xParity = eUartParityNone;
    xUartConfig.xStopbits = eUartStopBitsOne;
    xUartConfig.ucFlowControl = true;

    status = iot_uart_ioctl( xConsoleUart, eUartSetConfig, &xUartConfig );
    configASSERT( status == IOT_UART_SUCCESS );
}
/*-----------------------------------------------------------*/


void nfc_task(void *pvParameter)
{
    configPRINTF(("Checking version data. \n"));
    pn532_spi_init(&nfc);
    // pn532_begin(&nfc);

    // uint32_t versiondata = pn532_getFirmwareVersion(&nfc);
    // if (!versiondata)
    // {
    //     configPRINTF(("Did not find PN532 board. \n"));
        // while (1)
        // {
        //     vTaskDelay(1000 / portTICK_RATE_MS);
        // }
    // }
    // // Got ok data, print it out!
    // configPRINTF(("Found PN532. \n"));
    // configPRINTF(("Firmware ver. %d.%d \n", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF));

    // // configure board to read RFID tags
    // pn532_SAMConfig(&nfc);

    // configPRINTF(("Waiting for an ISO14443A Card ... \n");)

    // while (1)
    // {
    //     uint8_t success;
    //     uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
    //     uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    //     // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    //     // 'uid' will be populated with the UID, and uidLength will indicate
    //     // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    //     success = pn532_readPassiveTargetID(&nfc, PN532_MIFARE_ISO14443A, uid, &uidLength, 0);

    //     if (success)
    //     {
    //         // Display some basic information about the card
    //         configPRINTF(("Found an ISO14443A card \n"));
    //         configPRINTF(("UID Length: %d bytes \n", uidLength));
    //         configPRINTF(("UID Value: \n"));  
    //         vTaskDelay(1000 / portTICK_RATE_MS);         
    //     }
    //     else
    //     {
    //         // PN532 probably timed out waiting for a card
    //         configPRINTF(("Timed out waiting for a card \n"));
    //     }
    // }
}

/**
 * @brief Application runtime entry point.
 */
int app_main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */

    prvMiscInitialization();

    configPRINTF( ( "Entered main function. \n " ) );

    if( SYSTEM_Init() == pdPASS )
    {
        /* A simple example to demonstrate key and certificate provisioning in
         * microcontroller flash using PKCS#11 interface. This should be replaced
         * by production ready key provisioning mechanism. */
        vDevModeKeyProvisioning();

        #if BLE_ENABLED
            /* Initialize BLE. */
            ESP_ERROR_CHECK( esp_bt_controller_mem_release( ESP_BT_MODE_CLASSIC_BT ) );
            configPRINTF( ( "BLE enabled.\n " ) );

            if( prvBLEStackInit() != ESP_OK )
            {
                configPRINTF( ( "Failed to initialize the bluetooth stack\n " ) );

                while( 1 )
                {
                }
            }
            configPRINTF( ( "BLE stack is OK.\n " ) );
        #else
            configPRINTF( ( "BLE not enabled.\n " ) );
            ESP_ERROR_CHECK( esp_bt_controller_mem_release( ESP_BT_MODE_CLASSIC_BT ) );
            ESP_ERROR_CHECK( esp_bt_controller_mem_release( ESP_BT_MODE_BLE ) );
        #endif /* if BLE_ENABLED */
        /* Run all demos. */
        // DEMO_RUNNER_RunDemos();

        // Put code here to run pn532 tasks
        xTaskCreate(&nfc_task, "nfc_task", 4096, NULL, 4, NULL);
    }

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the WiFi initialization, is performed in the RTOS daemon task
     * startup hook. */
    /* Following is taken care by initialization code in ESP IDF */
    /* vTaskStartScheduler(); */
    return 0;
}

/*-----------------------------------------------------------*/
extern void vApplicationIPInit( void );
static void prvMiscInitialization( void )
{
    int32_t uartRet;
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if( ( ret == ESP_ERR_NVS_NO_FREE_PAGES ) || ( ret == ESP_ERR_NVS_NEW_VERSION_FOUND ) )
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    iot_uart_init();

    #if BLE_ENABLED
        NumericComparisonInit();
    #endif

    /* Create tasks that are not dependent on the WiFi being initialized. */
    xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                            tskIDLE_PRIORITY + 5,
                            mainLOGGING_MESSAGE_QUEUE_LENGTH );
configPRINTF( ( "Entered prvMiscInitialization \n " ) );

#if AFR_ESP_LWIP
    configPRINTF( ("Initializing lwIP TCP stack\r\n") );
    tcpip_adapter_init();
#else
    configPRINTF( ("Initializing FreeRTOS TCP stack\r\n") );
    vApplicationIPInit();
#endif
}

/*-----------------------------------------------------------*/

#if BLE_ENABLED

    #if CONFIG_NIMBLE_ENABLED == 1
        esp_err_t prvBLEStackInit( void )
        {
            return ESP_OK;
        }


        esp_err_t xBLEStackTeardown( void )
        {
            esp_err_t xRet;

            xRet = esp_bt_controller_mem_release( ESP_BT_MODE_BLE );

            return xRet;
        }

    #else /* if CONFIG_NIMBLE_ENABLED == 1 */

        static esp_err_t prvBLEStackInit( void )
        {
            return ESP_OK;
        }

        esp_err_t xBLEStackTeardown( void )
        {
            esp_err_t xRet = ESP_OK;

            if( esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED )
            {
                xRet = esp_bluedroid_disable();
            }

            if( xRet == ESP_OK )
            {
                xRet = esp_bluedroid_deinit();
            }

            if( xRet == ESP_OK )
            {
                if( esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED )
                {
                    xRet = esp_bt_controller_disable();
                }
            }

            if( xRet == ESP_OK )
            {
                xRet = esp_bt_controller_deinit();
            }

            if( xRet == ESP_OK )
            {
                xRet = esp_bt_controller_mem_release( ESP_BT_MODE_BLE );
            }

            if( xRet == ESP_OK )
            {
                xRet = esp_bt_controller_mem_release( ESP_BT_MODE_BTDM );
            }

            return xRet;
        }
    #endif /* if CONFIG_NIMBLE_ENABLED == 1 */
#endif /* if BLE_ENABLED */

/*-----------------------------------------------------------*/


#if BLE_ENABLED
/*-----------------------------------------------------------*/

    static void prvUartCallback( IotUARTOperationStatus_t xStatus,
                                      void * pvUserContext )
    {
        SemaphoreHandle_t xUartSem = ( SemaphoreHandle_t ) pvUserContext;
        configASSERT( xUartSem != NULL );
        xSemaphoreGive( xUartSem );
    }

  
    BaseType_t getUserMessage( INPUTMessage_t * pxINPUTmessage,
                               TickType_t xAuthTimeout )
    {
        BaseType_t xReturnMessage = pdFALSE;
        SemaphoreHandle_t xUartSem;
        int32_t status, bytesRead = 0;
        uint8_t *pucResponse;

        xUartSem = xSemaphoreCreateBinary();

        
        /* BLE Numeric comparison response is one character (y/n). */
        pucResponse = ( uint8_t * ) pvPortMalloc( sizeof( uint8_t ) );

        if( ( xUartSem != NULL ) && ( pucResponse != NULL ) )
        {
            iot_uart_set_callback( xConsoleUart, prvUartCallback, xUartSem );

            status = iot_uart_read_async( xConsoleUart, pucResponse, 1 );

            /* Wait for  auth timeout to get the input character. */
            xSemaphoreTake( xUartSem, xAuthTimeout );

            /* Cancel the uart operation if the character is received or timeout occured. */
            iot_uart_cancel( xConsoleUart );

            /* Reset the callback. */
            iot_uart_set_callback( xConsoleUart, NULL, NULL );

            iot_uart_ioctl( xConsoleUart, eGetRxNoOfbytes, &bytesRead );

            if( bytesRead == 1 )
            {
                pxINPUTmessage->pcData = pucResponse;
                pxINPUTmessage->xDataSize = 1;
                xReturnMessage = pdTRUE;
            }

            vSemaphoreDelete( xUartSem );
        }

        return xReturnMessage;
    }
#endif /* if BLE_ENABLED */

/*-----------------------------------------------------------*/

extern void esp_vApplicationTickHook();
void IRAM_ATTR vApplicationTickHook()
{
    esp_vApplicationTickHook();
}

/*-----------------------------------------------------------*/
extern void esp_vApplicationIdleHook();
void vApplicationIdleHook()
{
    esp_vApplicationIdleHook();
}

/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{

}

#if !AFR_ESP_LWIP
/*-----------------------------------------------------------*/
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    system_event_t evt;

    if( eNetworkEvent == eNetworkUp )
    {
        /* Print out the network configuration, which may have come from a DHCP
         * server. */
        FreeRTOS_GetAddressConfiguration(
            &ulIPAddress,
            &ulNetMask,
            &ulGatewayAddress,
            &ulDNSServerAddress );

        evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
        evt.event_info.got_ip.ip_changed = true;
        evt.event_info.got_ip.ip_info.ip.addr = ulIPAddress;
        evt.event_info.got_ip.ip_info.netmask.addr = ulNetMask;
        evt.event_info.got_ip.ip_info.gw.addr = ulGatewayAddress;
        esp_event_send( &evt );
    }
}
#endif

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
#include "iot_wifi.h"
#include <stdlib.h>

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

// #if BLE_ENABLED
    #include "bt_hal_manager_adapter_ble.h"
    #include "bt_hal_manager.h"
    #include "bt_hal_gatt_server.h"

    #include "iot_ble.h"
    #include "iot_ble_config.h"
    #include "iot_ble_wifi_provisioning.h"
    #include "iot_ble_numericComparison.h"
// #endif

// Include other necessary files here
#include "spi_pn532.h"

/* Logging Task Defines. */
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 32 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 4 )
#define mainDEVICE_NICK_NAME                "Espressif_Demo"

// String to store SSID and password for Wi-Fi network
#define clientCredentialSsid   ssid
#define clientCredentialPassword  password
#define maxSsid 32
#define maxPassword 63

// uint8_t * response = NULL;

char ssid[maxSsid] = "";
char password[maxPassword] = "";

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

void connectToNetwork()
{
    WIFINetworkParams_t xNetworkParams;
    WIFIReturnCode_t xWifiStatus;
    // Ensure Wi-fi module is on
    xWifiStatus = WIFI_On();
    if(xWifiStatus == eWiFiSuccess)
    {
        configPRINTF(("Wi-Fi library initialized \n"));
    }
    else
    {
        configPRINTF(("Failed to initialize Wi-Fi library \n"));
    }

    // Setup parameters
    const char * pcSSID = clientCredentialSsid;
    size_t xSSIDLength = strlen( pcSSID );
    const char * pcPassword = clientCredentialPassword;
    size_t xPasswordLength = strlen( pcPassword );

    configPRINTF(("ssid length: %d \n", xSSIDLength));
    configPRINTF(("password length: %d \n", xPasswordLength));

    memcpy( xNetworkParams.ucSSID, pcSSID, xSSIDLength );
    xNetworkParams.ucSSIDLength = xSSIDLength;
    xNetworkParams.xSecurity = clientcredentialWIFI_SECURITY;

    if( ( xNetworkParams.xSecurity == eWiFiSecurityWPA2 ) ||
        ( xNetworkParams.xSecurity == eWiFiSecurityWPA ) )
    {
        memcpy( xNetworkParams.xPassword.xWPA.cPassphrase, pcPassword, xPasswordLength );
        xNetworkParams.xPassword.xWPA.ucLength = xPasswordLength;
    }

    // Connect
    xWifiStatus = WIFI_ConnectAP(&xNetworkParams);
    vTaskDelay(4000);

    if(xWifiStatus == eWiFiSuccess)
    {
        configPRINTF(("Connected to network \n"));
    }
    else
    {
        configPRINTF(("Failed to connect to network \n"));
    }

}

// Include function here to check for networks and initiate ble provisioning
// This function will then start nfc task if not able to connect

void nfc_task(void *pvParameter)
{
    configPRINTF(("Checking version data. \n"));
    pn532_spi_init(&nfc, PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    pn532_begin(&nfc);

    uint32_t versiondata = pn532_getFirmwareVersion(&nfc);
    if (!versiondata)
    {
        configPRINTF(("Did not find PN532 board. \n"));
        while (1)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }
    // Got ok data, print it out!
    configPRINTF(("Found PN532. \n"));
    configPRINTF(("Firmware ver. %d.%d \n", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF));

    // configure board to read RFID tags
    configPRINTF(("Configuring SAM \n");)
    pn532_SAMConfig(&nfc);

    configPRINTF(("Waiting for an NFC Card ... \n");)

    while (1)
    {
        uint8_t success;
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
        uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        
        // Inlist smartphone
        success = pn532_inListPassiveTarget(&nfc);
        if (success)
        {
            configPRINTF(("Successfully inlisted target\n"));
            vTaskDelay(10000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("Could not inlist target \n"));
        }

        // NDEF detection procedure must correspond to the following steps:
        // 1. Select the NDEF tag
        // 2. Select the capability container
        // 3. Read the capability container and select the NDEF file
        // 4. Read NLEN (the NDEF length) from the NDEF file
        //      If NLEN > 0x00 and NLEN =< max NDEF file size-2, the message is detected
        //      If NLEN == 0x00, no NDEF message is detected
        //      If NLEN > max NDEF file size-2, the tag is not in a valid state
        // See specification: http://apps4android.org/nfc-specifications/NFCForum-TS-Type-4-Tag_2.0.pdf

        // In this case, the smartphone is emulating an NFC Type 4 tag
        // 1. Send command to select smartphone
        configPRINTF(("Selecting tag.... \n"));
        uint8_t tagSelect[] = { 0x00, // Class byte
                                0xa4, // Instruction for select command
                                0x04, // P1
                                0x00, // P2
                                0x07, // Length of AID
                                0xd2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, // D2760000850101 AID specified in app
                                0x00 }; // le
        uint8_t response[63] = {0};
        uint8_t responseLength = 32;
        success = pn532_inDataExchange(&nfc, tagSelect, sizeof(tagSelect), response, &responseLength);
        if (success)
        {
            configPRINTF(("Tag selection successful \n"));
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("Tag selection unsuccessful \n"));
        }

        // 2. Select the capability container
        configPRINTF(("Selecting capability container.... \n"));
        uint8_t ccSelect[] = {  0x00, // Class byte
                                0xa4, // Instruction for select command
                                0x00, // P1
                                0x0c, // P2
                                0x02, // lc
                                0xe1, 0x03 }; // File identifier
        success = pn532_inDataExchange(&nfc, ccSelect, sizeof(ccSelect), response, &responseLength);
        if (success)
        {
            configPRINTF(("Capability container selection successful \n"));
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("Capability container selection unsuccessful \n"));
        }

        // 3a. Read the capability container
        configPRINTF(("Reading capability container binary file.... \n"));
        uint8_t ccRead[] = { 0x00, // Class byte
                            0xb0, // Instruction for select command
                            0x00, // P1
                            0x00, // P2
                            0x0f }; // File identifier
        success = pn532_inDataExchange(&nfc, ccRead, sizeof(ccRead), response, &responseLength);
        if (success)
        {
            configPRINTF(("Capability container read successful \n"));
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("Capability container read unsuccessful \n"));
        }


        // 3b. Select the NDEF file
        configPRINTF(("Reading NDEF file.... \n"));
        uint8_t ndefSelect[] = { 0x00, // Class byte
                                0xa4, // Instruction for select command
                                0x00, // P1
                                0x0c, // P2
                                0x02, // lc
                                0xe1, 0x04 }; // File identifier
        success = pn532_inDataExchange(&nfc, ndefSelect, sizeof(ndefSelect), response, &responseLength);
        if (success)
        {
            configPRINTF(("NDEF file read successful \n"));
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("NDEF file read unsuccessful \n"));
        }

        // 4. Read NLEN (the NDEF length) from the NDEF file
        configPRINTF(("Reading NDEF file length.... \n"));
        uint8_t nlenRead[] = { 0x00, // Class byte
                                0xb0, // Instruction for read binary command
                                0x00, // P1
                                0x00, // P2 - in this case P1 and P2 represent the offset in the CC file
                                0x02 }; // le
        success = pn532_inDataExchange(&nfc, nlenRead, sizeof(nlenRead), response, &responseLength);
        if (success)
        {
            configPRINTF(("NDEF file length read successful \n"));
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("NDEF file length read unsuccessful \n"));
        }
        

        // Reading data from the NDEF file
        configPRINTF(("Reading data from NDEF file.... \n"));
        uint8_t ndefRead[] = { 0x00, // Class byte
                                0xb0, // Instruction for read binary command
                                0x00, // P1
                                0x01, // P2 - in this case P1 and P2 represent the offset in the CC file
                                0x0f }; // le
        success = pn532_inDataExchange(&nfc, ndefRead, sizeof(ndefRead), response, &responseLength);
        if (success)
        {
            configPRINTF(("NDEF file read successful \n"));

            // (testing) Print out response buffer
            for(int i =0; i < (sizeof(response)); i++)
            {
                configPRINTF(("message: %02x \n", response[i]));
            }
            

            // convert utf-8 hex string to character string
            configPRINTF(("Converting hex string....\n"));
            int part;
            int count1 = 0;
            int count2 = 0;
            int term = 0;
            for(int i = 17; i < (sizeof(response)-21); i++)
            {
                part = response[i];
                configPRINTF(("%c \n", part));

                if (part == 32)
                {
                    // configPRINTF(("Space character.....\n"));
                    term = 1;
                }

                if(term == 0)
                {
                    // configPRINTF(("Term = 0 \n"));
                    ssid[count1] = (char)part;
                    count1++;
                }
                if(term ==1 && part != 32){
                    // configPRINTF(("Term = 1 \n"));
                    password[count2] = part;
                    count2++;
                }
            }
            // configPRINTF(("finalString: %s \n", finalString));
            configPRINTF(("ssid: %s \n", clientCredentialSsid));
            configPRINTF(("password: %s \n", clientCredentialPassword));

            // // Store credentials
            connectToNetwork();

            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        else
        {
            configPRINTF(("NDEF file read unsuccessful \n"));
        }

        

        // End of steps
        configPRINTF(("Waiting.....\n"));
        vTaskDelay(5000 / portTICK_RATE_MS);

    }
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

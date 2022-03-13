#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iot_wifi.h"
#include "esp_system.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "nvs_flash.h"

// Include other necessary files here
#include "aws_clientcredential.h"
#include "wifi.h"

int wifiStatus = 1;


// Poll Wi-Fi connection status for mqtt demo
int returnWifiStatus()
{
    return wifiStatus;
}
/*-----------------------------------------------------------*/

// Set the Wi-Fi provisioning flag
void setProvFlag()
{
    nvs_handle nvs_write_handler;
    esp_err_t err;
    // Open storage partition
    err = nvs_open("storage", NVS_READWRITE, &nvs_write_handler);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to open storage\n"));
    }
    // Set flag in nvs flash storage
    // esp_err_t nvs_set_u8(nvs_handle_thandle, const char *key, uint8_t value)
    err = nvs_set_u8(nvs_write_handler, "provFlag", 1);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to set flag \n"));
    }
    // Close storage partition
    nvs_close(nvs_write_handler);
}

/*-----------------------------------------------------------*/

// Check Wi-Fi provisioning flag to determine if the device was provisioned
uint8_t getProvFlag()
{
    nvs_handle nvs_read_handler;
    esp_err_t err;
    uint8_t flag = 0;
    // Open storage partition
    err = nvs_open("storage", NVS_READWRITE, &nvs_read_handler);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to open storage\n"));
    }

    // Read from storage
    // esp_err_t nvs_get_u8(nvs_handle_thandle, const char *key, uint8_t *out_value)
    
    err = nvs_get_u8(nvs_read_handler, "provFlag", &flag);
    if(err != ESP_OK)
    {
        configPRINTF(("Provisioning has not occured\n"));
    }
    // Close storage partition
    nvs_close(nvs_read_handler);
    return flag;
}

/*-----------------------------------------------------------*/

// Retrieve credentials from flash and connect to network
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

    nvs_handle nvs_read_handler;
    esp_err_t err;
    // Open storage partition
    err = nvs_open("storage", NVS_READWRITE, &nvs_read_handler);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to open storage\n"));
    }

    // Setup parameters
    size_t xSSIDLength;
    size_t xPasswordLength;

    // Get the size of ssid and network password
    // esp_err_tnvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length)
    // out_value is a pointer to string used to store data
    // length points to variable holding length of out_value
    err = nvs_get_str(nvs_read_handler, "ssid", NULL, &xSSIDLength);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to read ssid size\n"));
    }
    err = nvs_get_str(nvs_read_handler, "password", NULL, &xPasswordLength);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to read password size\n"));
    }

    configPRINTF(("ssid length: %d \n", xSSIDLength));
    configPRINTF(("password length: %d \n", xPasswordLength));

    char * pcSSID = malloc(xSSIDLength);
    char * pcPassword = malloc(xPasswordLength);

    // Read from storage
    err = nvs_get_str(nvs_read_handler, "ssid", pcSSID, &xSSIDLength);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to read ssid\n"));
    }
    err = nvs_get_str(nvs_read_handler, "password", pcPassword, &xPasswordLength);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to read password\n"));
    }
    // Close storage partition
    nvs_close(nvs_read_handler);

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
    vTaskDelay(10);

    if(xWifiStatus == eWiFiSuccess)
    {
        configPRINTF(("Connected to network \n"));
        wifiStatus = 0;
    }
    else
    {
        configPRINTF(("Failed to connect to network \n"));
        while(WIFI_IsConnected(&xNetworkParams) == pdFALSE)
        {
            configPRINTF(("Reconnecting \n"));
            xWifiStatus = WIFI_ConnectAP(&xNetworkParams);
        }
    }
    return;
}
/*-----------------------------------------------------------*/
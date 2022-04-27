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
#include "spi_pn532.h"
#include "nfc_tasks.h"
#include "wifi.h"
#include "aws_clientcredential.h"

#include "iot_ble_wifi_provisioning.h"
#include "aws_wifi_connect_task.h"

// String to store SSID and password for Wi-Fi network
#define clientCredentialSsid   ssid
#define clientCredentialPassword  password
#define maxSsid 32
#define maxPassword 63


char ssid[maxSsid] = "";
char password[maxPassword] = "";
static pn532_t nfc;

// Store network ssid and password in nvs flash
void storeCredentials()
{
    // Write ssid and password to nvs flash
    nvs_handle nvs_write_handler;
    esp_err_t err;
    // Open storage partition
    err = nvs_open("storage", NVS_READWRITE, &nvs_write_handler);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to open storage\n"));
    }
    // Store ssid and password in nvs flash storage
    // esp_err_tnvs_set_str(nvs_handle_t handle, const char *key, const char *value)
    // key is limited to 14 characters
    // value is the string to store. This is limited to 4000 bytes
    err = nvs_set_str(nvs_write_handler, "ssid", clientCredentialSsid);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to write ssid\n"));
    }
    err = nvs_set_str(nvs_write_handler, "password", clientCredentialPassword);
    if(err != ESP_OK)
    {
        configPRINTF(("Failed to write password\n"));
    }
    // Close storage partition
    nvs_close(nvs_write_handler);

    // Set provisioning flag in flash
    setProvFlag();

}
/*-----------------------------------------------------------*/

void nfc_task(void *pvParameter)
{
    // Get ble mac address
    // uint8_t mac[6];
    // esp_err_t err;
    // err = esp_read_mac(&mac, 2);
    // if(err != ESP_OK)
    // {
    //     configPRINTF(("Failed to read mac address\n"));
    // }
    // else
    // {
    //     configPRINTF(("Checking device MAC: %02x%02x%02x%02x%02x%02x \n", mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]));
    // }

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
    // configPRINTF(("Firmware ver. %02x \n", versiondata));

    // configure board to read RFID tags
    configPRINTF(("Configuring SAM \n");)
    pn532_SAMConfig(&nfc);

    // configPRINTF(("Waiting for an NFC Card ... \n");)

    while (1)
    {
        uint8_t success;                   // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        
        // Inlist smartphone
        success = pn532_inListPassiveTarget(&nfc);
        if (success)
        {
            configPRINTF(("Successfully inlisted target\n"));
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("Could not inlist target \n"));
        // }

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
        // configPRINTF(("Selecting tag.... \n"));
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
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("Tag selection unsuccessful \n"));
        // }

        // 2. Select the capability container
        // configPRINTF(("Selecting capability container.... \n"));
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
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("Capability container selection unsuccessful \n"));
        // }

        // 3a. Read the capability container
        // configPRINTF(("Reading capability container binary file.... \n"));
        uint8_t ccRead[] = { 0x00, // Class byte
                            0xb0, // Instruction for select command
                            0x00, // P1
                            0x00, // P2
                            0x0f }; // File identifier
        success = pn532_inDataExchange(&nfc, ccRead, sizeof(ccRead), response, &responseLength);
        if (success)
        {
            configPRINTF(("Capability container read successful \n"));
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("Capability container read unsuccessful \n"));
        // }


        // 3b. Select the NDEF file
        // configPRINTF(("Reading NDEF file.... \n"));
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
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("NDEF file read unsuccessful \n"));
        // }

        // 4. Read NLEN (the NDEF length) from the NDEF file
        // configPRINTF(("Reading NDEF file length.... \n"));
        uint8_t nlenRead[] = { 0x00, // Class byte
                                0xb0, // Instruction for read binary command
                                0x00, // P1
                                0x00, // P2 - in this case P1 and P2 represent the offset in the CC file
                                0x02 }; // le
        success = pn532_inDataExchange(&nfc, nlenRead, sizeof(nlenRead), response, &responseLength);
        if (success)
        {
            configPRINTF(("NDEF file length read successful \n"));
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("NDEF file length read unsuccessful \n"));
        // }
        

        // Reading data from the NDEF file
        // configPRINTF(("Reading data from NDEF file.... \n"));
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

                if (part == 63)
                {
                    term++;
                }

                if(term == 0)
                {
                    ssid[count1] = (char)part;
                    count1++;
                }
                if(term ==1 && part != 63){
                    password[count2] = part;
                    count2++;
                }
            }
            configPRINTF(("ssid: %s \n", clientCredentialSsid));
            configPRINTF(("password: %s \n", clientCredentialPassword));

            // Store credentials
            storeCredentials();
            connectToNetwork();
        
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        // else
        // {
        //     configPRINTF(("NDEF file read unsuccessful \n"));
        // }
    
        // End of steps
        vTaskDelay(5000 / portTICK_RATE_MS);
    }

}
/*-----------------------------------------------------------*/
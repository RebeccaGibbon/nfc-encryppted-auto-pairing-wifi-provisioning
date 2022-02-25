#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iot_wifi.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

// Include other necessary files here
#include "spi_pn532.h"
#include "nfc_tasks.h"
#include "aws_clientcredential.h"

// String to store SSID and password for Wi-Fi network
#define clientCredentialSsid   ssid
#define clientCredentialPassword  password
#define maxSsid 32
#define maxPassword 63


char ssid[maxSsid] = "";
char password[maxPassword] = "";

static pn532_t nfc;


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
/*-----------------------------------------------------------*/

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

            // Store credentials
            connectToNetwork();
            // Run mqtt demo
            // DEMO_RUNNER_RunDemos();

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
/*-----------------------------------------------------------*/
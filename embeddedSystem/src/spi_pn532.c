// This is based on esp-idf's SPI Master example code
// https://github.com/espressif/esp-idf/blob/master/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "iot_logging_task.h"
#include "spi_pn532.h"

// Define pins for using SPI2 bus
#define PN532_HOST    HSPI_HOST

// #define PIN_NUM_MISO 12
// #define PIN_NUM_MOSI 13
// #define PIN_NUM_CLK  14 // SCK
// #define PIN_NUM_CS   15 // SS


// #define PN532_MISO 12
// #define PN532_MOSI 13
// #define PN532_SCK  14 // SCK
// #define PN532_SS   15 // SS

#define PN532_PACKBUFFSIZ 64

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define PN532_DELAY(ms) vTaskDelay(ms / portTICK_RATE_MS)

static uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
static uint8_t pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};
static uint8_t pn532_packetbuffer[PN532_PACKBUFFSIZ];

static void pn532_readdata(pn532_t *obj, uint8_t *buff, uint8_t n);
static void pn532_writecommand(pn532_t *obj, uint8_t *cmd, uint8_t cmdlen);
static bool pn532_readack(pn532_t *obj);
static bool pn532_isready(pn532_t *obj);
static bool pn532_waitready(pn532_t *obj, uint16_t timeout);
static void pn532_spi_write(pn532_t *obj, uint8_t c);
static uint8_t pn532_spi_read(pn532_t *obj);


void pn532_spi_init(pn532_t *obj, uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss)
{
    obj->_clk = clk;
    obj->_miso = miso;
    obj->_mosi = mosi;
    obj->_ss = ss;

    gpio_pad_select_gpio(obj->_clk);
    gpio_pad_select_gpio(obj->_miso);
    gpio_pad_select_gpio(obj->_mosi);
    gpio_pad_select_gpio(obj->_ss);

    gpio_set_direction(obj->_ss, GPIO_MODE_OUTPUT);
    gpio_set_level(obj->_ss, 1);
    gpio_set_direction(obj->_clk, GPIO_MODE_OUTPUT);
    gpio_set_direction(obj->_mosi, GPIO_MODE_OUTPUT);
    gpio_set_direction(obj->_miso, GPIO_MODE_INPUT);
    
}

/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
void pn532_begin(pn532_t *obj)
{
    gpio_set_level(obj->_ss, 0);
    // gpio_set_level(PN532_SS, 0);

    PN532_DELAY(1000);

    // not exactly sure why but we have to send a dummy command to get synced up
    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
    pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 1, 1000);

    // ignore response!
    gpio_set_level(obj->_ss, 1);
    // gpio_set_level(PN532_SS, 1);
}

/**************************************************************************/
/*!
    @brief  Checks the firmware version of the PN5xx chip
    @returns  The chip's firmware version and ID
*/
/**************************************************************************/
uint32_t pn532_getFirmwareVersion(pn532_t *obj)
{
    uint32_t response;

    configPRINTF(("Getting firmware version.\n"));

    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 1, 1000))
    {
        configPRINTF(("!pn532_sendCommandCheckAck!\n"));
        return 0;
    }

    configPRINTF(("Got firmware version.\n"));

    // read data packet
    pn532_readdata(obj, pn532_packetbuffer, 12);

    // check some basic stuff
    if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6))
    {
        configPRINTF(("Firmware doesn't match!\n"));
        return 0;
    }

    int offset = 6;
    response = pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];

    return response;
}

/**************************************************************************/
/*!
    @brief  Sends a command and waits a specified period for the ACK
    @param  cmd       Pointer to the command buffer
    @param  cmdlen    The size of the command in bytes
    @param  timeout   timeout before giving up
    @returns  1 if everything is OK, 0 if timeout occured before an
              ACK was recieved
*/
/**************************************************************************/
// default timeout of one second
bool pn532_sendCommandCheckAck(pn532_t *obj, uint8_t *cmd, uint8_t cmdlen, uint16_t timeout)
{
    configPRINTF(("Writing command.\n"));
    // write the command
    pn532_writecommand(obj, cmd, cmdlen);

    configPRINTF(("Waiting for ACK 1.\n"));
    // Wait for chip to say its ready!
    if (!pn532_waitready(obj, timeout))
    {
        configPRINTF(("Not ready 1.\n"));
        return false;
    }

    // read acknowledgement
    if (!pn532_readack(obj))
    {
        configPRINTF(("No ACK frame received!\n"));
        return false;
    }

    configPRINTF(("Waiting for ACK 2.\n"));
    // For SPI only wait for the chip to be ready again.
    // This is unnecessary with I2C.
    if (!pn532_waitready(obj, timeout))
    {
        configPRINTF(("Not ready 2.\n"));
        return false;
    }

    return true; // ack'd command
}


/**************************************************************************/
/*!
    @brief  Configures the SAM (Secure Access Module)
*/
/**************************************************************************/
bool pn532_SAMConfig(pn532_t *obj)
{
    pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!

    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 4, 1000))
        return false;

    // read data packet
    pn532_readdata(obj, pn532_packetbuffer, 8);

    int offset = 5;
    return (pn532_packetbuffer[offset] == 0x15);
}


/***** ISO14443A Commands ******/

/**************************************************************************/
/*!
    Waits for an ISO14443A target to enter the field
    @param  cardBaudRate  Baud rate of the card
    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 bytes)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool pn532_readPassiveTargetID(pn532_t *obj, uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout)
{
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;

    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 3, timeout))
    {
        configPRINTF(("No card(s) read\n"));
        return 0x0; // no cards read
    }

    // read data packet
    pn532_readdata(obj, pn532_packetbuffer, 20);
    // check some basic stuff

    /* ISO14443A card response should be in the following format:
    uint8_t            Description
    -------------   ------------------------------------------
    b0..6           Frame header and preamble
    b7              Tags Found
    b8              Tag Number (only one used in this example)
    b9..10          SENS_RES
    b11             SEL_RES
    b12             NFCID Length
    b13..NFCIDLen   NFCID                                      */

    configPRINTF(("Found %d tags\n", pn532_packetbuffer[7]));
    if (pn532_packetbuffer[7] != 1)
        return 0;

    uint16_t sens_res = pn532_packetbuffer[9];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[10];
    configPRINTF(("ATQA: %02x\n", sens_res));
    configPRINTF(("SAK: %02x\n", pn532_packetbuffer[11]));

    /* Card appears to be Mifare Classic */
    *uidLength = pn532_packetbuffer[12];

    for (uint8_t i = 0; i < pn532_packetbuffer[12]; i++)
    {
        uid[i] = pn532_packetbuffer[13 + i];
    }

    configPRINTF(("UID: \n"));
    for (int i = 0; i < pn532_packetbuffer[12]; i++)
    {
        configPRINTF(("Printing UID\t"));
        configPRINTF((" %02x \n", uid[i]));
    }
    configPRINTF(("\n"));

    return 1;
}

/**************************************************************************/
/*!
    @brief  Exchanges an APDU with the currently inlisted peer
    @param  send            Pointer to data to send
    @param  sendLength      Length of the data to send
    @param  response        Pointer to response data
    @param  responseLength  Pointer to the response data length
*/
/**************************************************************************/
bool pn532_inDataExchange(pn532_t *obj, uint8_t *send, uint8_t sendLength, uint8_t *response, uint8_t *responseLength)
{
    if (sendLength > PN532_PACKBUFFSIZ - 2)
    {
        configPRINTF(("APDU length too long for packet buffer\n"));
        return false;
    }
    uint8_t i;

    pn532_packetbuffer[0] = 0x40; // PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = obj->_inListedTag;
    for (i = 0; i < sendLength; ++i)
    {
        pn532_packetbuffer[i + 2] = send[i];
    }

    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, sendLength + 2, 1000))
    {
        configPRINTF(("Could not send APDU\n"));
        return false;
    }

    if (!pn532_waitready(obj, 1000))
    {
        configPRINTF(("Response never received for APDU...\n"));
        return false;
    }

    pn532_readdata(obj, pn532_packetbuffer, sizeof(pn532_packetbuffer));

    if (pn532_packetbuffer[0] == 0 && pn532_packetbuffer[1] == 0 && pn532_packetbuffer[2] == 0xff)
    {
        uint8_t length = pn532_packetbuffer[3];
        if (pn532_packetbuffer[4] != (uint8_t)(~length + 1))
        {
            configPRINTF(("Length check invalid %02x%02x\n", length, (~length) + 1));
            return false;
        }
        if (pn532_packetbuffer[5] == PN532_PN532TOHOST && pn532_packetbuffer[6] == PN532_RESPONSE_INDATAEXCHANGE)
        {
            if ((pn532_packetbuffer[7] & 0x3f) != 0)
            {
                configPRINTF(("Status code indicates an error\n"));
                return false;
            }

            length -= 3;

            if (length > *responseLength)
            {
                length = *responseLength; // silent truncation...
            }

            for (i = 0; i < length; ++i)
            {
                response[i] = pn532_packetbuffer[8 + i];
            }
            *responseLength = length;

            return true;
        }
        else
        {
            configPRINTF(("Don't know how to handle this command: %02x\n", pn532_packetbuffer[6]));
            return false;
        }
    }
    else
    {
        configPRINTF(("Preamble missing\n"));
        return false;
    }
}

/**************************************************************************/
/*!
    @brief  'InLists' a passive target. PN532 acting as reader/initiator,
            peer acting as card/responder.
*/
/**************************************************************************/
bool pn532_inListPassiveTarget(pn532_t *obj)
{
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;
    pn532_packetbuffer[2] = 0;

    configPRINTF(("About to inList passive target\n"));

    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 3, 1000))
    {
        configPRINTF(("Could not send inlist message\n"));
        return false;
    }

    if (!pn532_waitready(obj, 30000))
    {
        return false;
    }

    pn532_readdata(obj, pn532_packetbuffer, sizeof(pn532_packetbuffer));

    if (pn532_packetbuffer[0] == 0 && pn532_packetbuffer[1] == 0 && pn532_packetbuffer[2] == 0xff)
    {
        uint8_t length = pn532_packetbuffer[3];
        if (pn532_packetbuffer[4] != (uint8_t)(~length + 1))
        {
            configPRINTF(("Length check invalid %02x%02x\n", length, (~length) + 1));
            return false;
        }
        if (pn532_packetbuffer[5] == PN532_PN532TOHOST && pn532_packetbuffer[6] == PN532_RESPONSE_INLISTPASSIVETARGET)
        {
            if (pn532_packetbuffer[7] != 1)
            {
                configPRINTF(("Unhandled number of targets inlisted\n"));
                configPRINTF(("Number of tags inlisted: %d\n", pn532_packetbuffer[7]));
                return false;
            }

            obj->_inListedTag = pn532_packetbuffer[8];
            configPRINTF(("Tag number: %d\n", obj->_inListedTag));

            return true;
        }
        else
        {
            configPRINTF(("Unexpected response to inlist passive host\n"));
            return false;
        }
    }
    else
    {
        configPRINTF(("Preamble missing\n"));
        return false;
    }

    return true;
}

/***** Mifare Classic Functions ******/

/**************************************************************************/
/*!
      Indicates whether the specified block number is the first block
      in the sector (block 0 relative to the current sector)
*/
/**************************************************************************/
bool pn532_mifareclassic_IsFirstBlock(pn532_t *obj, uint32_t uiBlock)
{
    // Test if we are in the small or big sectors
    if (uiBlock < 128)
        return ((uiBlock) % 4 == 0);
    else
        return ((uiBlock) % 16 == 0);
}

/**************************************************************************/
/*!
      Indicates whether the specified block number is the sector trailer
*/
/**************************************************************************/
bool pn532_mifareclassic_IsTrailerBlock(pn532_t *obj, uint32_t uiBlock)
{
    // Test if we are in the small or big sectors
    if (uiBlock < 128)
        return ((uiBlock + 1) % 4 == 0);
    else
        return ((uiBlock + 1) % 16 == 0);
}

/**************************************************************************/
/*!
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.
    @param  uid           Pointer to a uint8_t array containing the card UID
    @param  uidLen        The length (in bytes) of the card's UID (Should
                          be 4 for MIFARE Classic)
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  keyNumber     Which key type to use during authentication
                          (0 = MIFARE_CMD_AUTH_A, 1 = MIFARE_CMD_AUTH_B)
    @param  keyData       Pointer to a uint8_t array containing the 6 uint8_t
                          key value
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_AuthenticateBlock(pn532_t *obj, uint8_t *uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t *keyData)
{
    uint8_t i;

    // Hang on to the key and uid data
    memcpy(obj->_key, keyData, 6);
    memcpy(obj->_uid, uid, uidLen);
    obj->_uidLen = uidLen;

    configPRINTF(("Trying to authenticate card\n"));
    for (int i = 0; i < obj->_uidLen; i++)
    {
        configPRINTF((" %02x", obj->_uid[i]));
    }
    configPRINTF(("\n"));
    configPRINTF(("Using authentication KEY %c\n", keyNumber ? 'B' : 'A'));
    for (int i = 0; i < 6; i++)
    {
        configPRINTF((" %02x", obj->_key[i]));
    }
    configPRINTF(("\n"));

    // Prepare the authentication command //
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE; /* Data Exchange Header */
    pn532_packetbuffer[1] = 1;                            /* Max card numbers */
    pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
    pn532_packetbuffer[3] = blockNumber; /* Block Number (1K = 0..63, 4K = 0..255 */
    memcpy(pn532_packetbuffer + 4, obj->_key, 6);
    for (i = 0; i < obj->_uidLen; i++)
    {
        pn532_packetbuffer[10 + i] = obj->_uid[i]; /* 4 uint8_t card ID */
    }

    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 10 + obj->_uidLen, 1000))
        return 0;

    // Read the response packet
    pn532_readdata(obj, pn532_packetbuffer, 12);

    // check if the response is valid and we are authenticated???
    // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
    // Mifare auth error is technically uint8_t 7: 0x14 but anything other and 0x00 is not good
    if (pn532_packetbuffer[7] != 0x00)
    {
        configPRINTF(("Authentification failed\n"));
        for (int i = 0; i < 12; i++)
    {
        configPRINTF((" %02x \n", pn532_packetbuffer[i]));
    }
    configPRINTF(("\n"));
        return 0;
    }

    return 1;
}

/**************************************************************************/
/*!
    Tries to read an entire 16-uint8_t data block at the specified block
    address.
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the uint8_t array that will hold the
                          retrieved data (if any)
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_ReadDataBlock(pn532_t *obj, uint8_t blockNumber, uint8_t *data)
{
    configPRINTF(("Trying to read 16 bytes from block %d\n", blockNumber));

    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;               /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = blockNumber;     /* Block Number (0..63 for 1K, 0..255 for 4K) */

    /* Send the command */
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 4, 1000))
    {
        configPRINTF(("Failed to receive ACK for read command\n"));
        return 0;
    }

    /* Read the response packet */
    pn532_readdata(obj, pn532_packetbuffer, 26);

    /* If uint8_t 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[7] != 0x00)
    {
        configPRINTF(("Unexpected response: \n"));
        for (int i = 0; i < 26; i++)
        {
            configPRINTF((" %02x \n", pn532_packetbuffer[i]));
        }
        configPRINTF(("\n"));
        return 0;
    }

    /* Copy the 16 data bytes to the output buffer        */
    /* Block content starts at uint8_t 9 of a valid response */
    memcpy(data, pn532_packetbuffer + 8, 16);

/* Display data for debug if requested */
    configPRINTF(("Block %d\n", blockNumber));
    for (int i = 0; i < 16; i++)
    {
        configPRINTF((" %02x", data[i]));
    }
    configPRINTF(("\n"));

    return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 16-uint8_t data block at the specified block
    address.
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          The uint8_t array that contains the data to write.
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_WriteDataBlock(pn532_t *obj, uint8_t blockNumber, uint8_t *data)
{
    configPRINTF(("Trying to write 16 bytes to block %d\n", blockNumber));

    /* Prepare the first command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
    pn532_packetbuffer[3] = blockNumber;      /* Block Number (0..63 for 1K, 0..255 for 4K) */
    memcpy(pn532_packetbuffer + 4, data, 16); /* Data Payload */

    /* Send the command */
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 20, 1000))
    {
        configPRINTF(("Failed to receive ACK for write command\n"));
        return 0;
    }
    PN532_DELAY(10);

    /* Read the response packet */
    pn532_readdata(obj, pn532_packetbuffer, 26);

    return 1;
}

/**************************************************************************/
/*!
    Formats a Mifare Classic card to store NDEF Records
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_FormatNDEF(pn532_t *obj)
{
    uint8_t sectorbuffer1[16] = {0x14, 0x01, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
    uint8_t sectorbuffer2[16] = {0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
    uint8_t sectorbuffer3[16] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0x78, 0x77, 0x88, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Note 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 must be used for key A
    // for the MAD sector in NDEF records (sector 0)

    // Write block 1 and 2 to the card
    if (!(pn532_mifareclassic_WriteDataBlock(obj, 1, sectorbuffer1)))
        return 0;
    if (!(pn532_mifareclassic_WriteDataBlock(obj, 2, sectorbuffer2)))
        return 0;
    // Write key A and access rights card
    if (!(pn532_mifareclassic_WriteDataBlock(obj, 3, sectorbuffer3)))
        return 0;

    // Seems that everything was OK (?!)
    return 1;
}

/**************************************************************************/
/*!
    Writes an NDEF URI Record to the specified sector (1..15)
    Note that this function assumes that the Mifare Classic card is
    already formatted to work as an "NFC Forum Tag" and uses a MAD1
    file system.  You can use the NXP TagWriter app on Android to
    properly format cards for this.
    @param  sectorNumber  The sector that the URI record should be written
                          to (can be 1..15 for a 1K card)
    @param  uriIdentifier The uri identifier code (0 = none, 0x01 =
                          "http://www.\n", etc.)
    @param  url           The uri text to write (max 38 characters).
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareclassic_WriteNDEFURI(pn532_t *obj, uint8_t sectorNumber, uint8_t uriIdentifier, const char *url)
{
    // Figure out how long the string is
    uint8_t len = strlen(url);

    // Make sure we're within a 1K limit for the sector number
    if ((sectorNumber < 1) || (sectorNumber > 15))
        return 0;

    // Make sure the URI payload is between 1 and 38 chars
    if ((len < 1) || (len > 38))
        return 0;

    // Note 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 must be used for key A
    // in NDEF records

    // Setup the sector buffer (w/pre-formatted TLV wrapper and NDEF message)
    uint8_t sectorbuffer1[16] = {0x00, 0x00, 0x03, len + 5, 0xD1, 0x01, len + 1, 0x55, uriIdentifier, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t sectorbuffer2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t sectorbuffer3[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t sectorbuffer4[16] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7, 0x7F, 0x07, 0x88, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (len <= 6)
    {
        // Unlikely we'll get a url this short, but why not ...
        memcpy(sectorbuffer1 + 9, url, len);
        sectorbuffer1[len + 9] = 0xFE;
    }
    else if (len == 7)
    {
        // 0xFE needs to be wrapped around to next block
        memcpy(sectorbuffer1 + 9, url, len);
        sectorbuffer2[0] = 0xFE;
    }
    else if ((len > 7) && (len <= 22))
    {
        // Url fits in two blocks
        memcpy(sectorbuffer1 + 9, url, 7);
        memcpy(sectorbuffer2, url + 7, len - 7);
        sectorbuffer2[len - 7] = 0xFE;
    }
    else if (len == 23)
    {
        // 0xFE needs to be wrapped around to final block
        memcpy(sectorbuffer1 + 9, url, 7);
        memcpy(sectorbuffer2, url + 7, len - 7);
        sectorbuffer3[0] = 0xFE;
    }
    else
    {
        // Url fits in three blocks
        memcpy(sectorbuffer1 + 9, url, 7);
        memcpy(sectorbuffer2, url + 7, 16);
        memcpy(sectorbuffer3, url + 23, len - 24);
        sectorbuffer3[len - 22] = 0xFE;
    }

    // Now write all three blocks back to the card
    if (!(pn532_mifareclassic_WriteDataBlock(obj, sectorNumber * 4, sectorbuffer1)))
        return 0;
    if (!(pn532_mifareclassic_WriteDataBlock(obj, (sectorNumber * 4) + 1, sectorbuffer2)))
        return 0;
    if (!(pn532_mifareclassic_WriteDataBlock(obj, (sectorNumber * 4) + 2, sectorbuffer3)))
        return 0;
    if (!(pn532_mifareclassic_WriteDataBlock(obj, (sectorNumber * 4) + 3, sectorbuffer4)))
        return 0;

    // Seems that everything was OK (?!)
    return 1;
}

/***** Mifare Ultralight Functions ******/

/**************************************************************************/
/*!
    Tries to read an entire 4-uint8_t page at the specified address.
    @param  page        The page number (0..63 in most cases)
    @param  buffer      Pointer to the uint8_t array that will hold the
                        retrieved data (if any)
*/
/**************************************************************************/
uint8_t pn532_mifareultralight_ReadPage(pn532_t *obj, uint8_t page, uint8_t *buffer)
{
    if (page >= 64)
    {
        configPRINTF(("Page value out of range\n"));
        return 0;
    }

    configPRINTF(("Reading page %d\n", page));

    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;               /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = page;            /* Page Number (0..63 in most cases) */

    /* Send the command */
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 4, 1000))
    {
        configPRINTF(("Failed to receive ACK for write command\n"));
        return 0;
    }

    /* Read the response packet */
    pn532_readdata(obj, pn532_packetbuffer, 26);
    configPRINTF(("Received: \n"));
    for (int i = 0; i < 26; i++)
    {
        configPRINTF((" %02x", pn532_packetbuffer[i]));
    }
    configPRINTF(("\n"));

    /* If uint8_t 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[7] == 0x00)
    {
        /* Copy the 4 data bytes to the output buffer         */
        /* Block content starts at uint8_t 9 of a valid response */
        /* Note that the command actually reads 16 uint8_t or 4  */
        /* pages at a time ... we simply discard the last 12  */
        /* bytes                                              */
        memcpy(buffer, pn532_packetbuffer + 8, 4);
    }
    else
    {
        configPRINTF(("Unexpected response reading block: \n"));
        for (int i = 0; i < 26; i++)
        {
            configPRINTF((" %02x", pn532_packetbuffer[i]));
        }
        configPRINTF(("\n"));
        return 0;
    }

/* Display data for debug if requested */
    configPRINTF(("Page %d:", page));
    for (int i = 0; i < 4; i++)
    {
        configPRINTF((" %02x", buffer[i]));
    }
    configPRINTF(("\n"));

    // Return OK signal
    return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 4-uint8_t page at the specified block
    address.
    @param  page          The page number to write.  (0..63 for most cases)
    @param  data          The uint8_t array that contains the data to write.
                          Should be exactly 4 bytes long.
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_mifareultralight_WritePage(pn532_t *obj, uint8_t page, uint8_t *data)
{

    if (page >= 64)
    {
        configPRINTF(("Page value out of range\n"));
        // Return Failed Signal
        return 0;
    }

    configPRINTF(("Trying to write 4 uint8_t page %d\n", page));

    /* Prepare the first command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                           /* Card number */
    pn532_packetbuffer[2] = MIFARE_ULTRALIGHT_CMD_WRITE; /* Mifare Ultralight Write command = 0xA2 */
    pn532_packetbuffer[3] = page;                        /* Page Number (0..63 for most cases) */
    memcpy(pn532_packetbuffer + 4, data, 4);             /* Data Payload */

    /* Send the command */
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 8, 1000))
    {
        configPRINTF(("Failed to receive ACK for write command\n"));
        // Return Failed Signal
        return 0;
    }
    PN532_DELAY(10);

    /* Read the response packet */
    pn532_readdata(obj, pn532_packetbuffer, 26);

    // Return OK Signal
    return 1;
}

/***** NTAG2xx Functions ******/

/**************************************************************************/
/*!
    Tries to read an entire 4-uint8_t page at the specified address.
    @param  page        The page number (0..63 in most cases)
    @param  buffer      Pointer to the uint8_t array that will hold the
                        retrieved data (if any)
*/
/**************************************************************************/
uint8_t pn532_ntag2xx_ReadPage(pn532_t *obj, uint8_t page, uint8_t *buffer)
{
    // TAG Type       PAGES   USER START    USER STOP
    // --------       -----   ----------    ---------
    // NTAG 203       42      4             39
    // NTAG 213       45      4             39
    // NTAG 215       135     4             129
    // NTAG 216       231     4             225

    if (page >= 231)
    {
        configPRINTF(("Page value out of range\n"));
        return 0;
    }

    configPRINTF(("Reading page %d\n", page));

    /* Prepare the command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;               /* Card number */
    pn532_packetbuffer[2] = MIFARE_CMD_READ; /* Mifare Read command = 0x30 */
    pn532_packetbuffer[3] = page;            /* Page Number (0..63 in most cases) */

    /* Send the command */
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 4, 1000))
    {
        configPRINTF(("Failed to receive ACK for write command\n"));
        return 0;
    }

    /* Read the response packet */
    pn532_readdata(obj, pn532_packetbuffer, 26);
    configPRINTF(("Received:"));
    for (int i = 0; i < 26; i++)
    {
        configPRINTF((" %02x", pn532_packetbuffer[i]));
    }
    configPRINTF(("\n"));

    /* If uint8_t 8 isn't 0x00 we probably have an error */
    if (pn532_packetbuffer[7] == 0x00)
    {
        /* Copy the 4 data bytes to the output buffer         */
        /* Block content starts at uint8_t 9 of a valid response */
        /* Note that the command actually reads 16 uint8_t or 4  */
        /* pages at a time ... we simply discard the last 12  */
        /* bytes                                              */
        memcpy(buffer, pn532_packetbuffer + 8, 4);
    }
    else
    {
        configPRINTF(("Unexpected response reading block:"));
        for (int i = 0; i < 26; i++)
        {
            configPRINTF((" %02x", pn532_packetbuffer[i]));
        }
        configPRINTF(("\n"));
        return 0;
    }

/* Display data for debug if requested */
    configPRINTF(("Page %d:", page));
    for (int i = 0; i < 4; i++)
    {
        configPRINTF((" %02x", buffer[i]));
    }
    configPRINTF(("\n"));

    // Return OK signal
    return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 4-uint8_t page at the specified block
    address.
    @param  page          The page number to write.  (0..63 for most cases)
    @param  data          The uint8_t array that contains the data to write.
                          Should be exactly 4 bytes long.
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_ntag2xx_WritePage(pn532_t *obj, uint8_t page, uint8_t *data)
{
    // TAG Type       PAGES   USER START    USER STOP
    // --------       -----   ----------    ---------
    // NTAG 203       42      4             39
    // NTAG 213       45      4             39
    // NTAG 215       135     4             129
    // NTAG 216       231     4             225

    if ((page < 4) || (page > 225))
    {
        configPRINTF(("Page value out of range\n"));
        // Return Failed Signal
        return 0;
    }

    configPRINTF(("Trying to write 4 uint8_t page %d\n", page));

    /* Prepare the first command */
    pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 1;                           /* Card number */
    pn532_packetbuffer[2] = MIFARE_ULTRALIGHT_CMD_WRITE; /* Mifare Ultralight Write command = 0xA2 */
    pn532_packetbuffer[3] = page;                        /* Page Number (0..63 for most cases) */
    memcpy(pn532_packetbuffer + 4, data, 4);             /* Data Payload */

    /* Send the command */
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 8, 1000))
    {
        configPRINTF(("Failed to receive ACK for write command\n"));

        // Return Failed Signal
        return 0;
    }
    PN532_DELAY(10);

    /* Read the response packet */
    pn532_readdata(obj, pn532_packetbuffer, 26);

    // Return OK Signal
    return 1;
}

/**************************************************************************/
/*!
    Writes an NDEF URI Record starting at the specified page (4..nn)
    Note that this function assumes that the NTAG2xx card is
    already formatted to work as an "NFC Forum Tag".
    @param  uriIdentifier The uri identifier code (0 = none, 0x01 =
                          "http://www.\n", etc.)
    @param  url           The uri text to write (null-terminated string).
    @param  dataLen       The size of the data area for overflow checks.
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t pn532_ntag2xx_WriteNDEFURI(pn532_t *obj, uint8_t uriIdentifier, char *url, uint8_t dataLen)
{
    uint8_t pageBuffer[4] = {0, 0, 0, 0};

    // Remove NDEF record overhead from the URI data (pageHeader below)
    uint8_t wrapperSize = 12;

    // Figure out how long the string is
    uint8_t len = strlen(url);

    // Make sure the URI payload will fit in dataLen (include 0xFE trailer)
    if ((len < 1) || (len + 1 > (dataLen - wrapperSize)))
        return 0;

    // Setup the record header
    // See NFCForum-TS-Type-2-Tag_1.1.pdf for details
    uint8_t pageHeader[12] =
        {
            /* NDEF Lock Control TLV (must be first and always present) */
            0x01, /* Tag Field (0x01 = Lock Control TLV) */
            0x03, /* Payload Length (always 3) */
            0xA0, /* The position inside the tag of the lock bytes (upper 4 = page address, lower 4 = uint8_t offset) */
            0x10, /* Size in bits of the lock area */
            0x44, /* Size in bytes of a page and the number of bytes each lock bit can lock (4 bit + 4 bits) */
            /* NDEF Message TLV - URI Record */
            0x03,         /* Tag Field (0x03 = NDEF Message) */
            len + 5,      /* Payload Length (not including 0xFE trailer) */
            0xD1,         /* NDEF Record Header (TNF=0x1:Well known record + SR + ME + MB) */
            0x01,         /* Type Length for the record type indicator */
            len + 1,      /* Payload len */
            0x55,         /* Record Type Indicator (0x55 or 'U' = URI Record) */
            uriIdentifier /* URI Prefix (ex. 0x01 = "http://www.\n") */
        };

    // Write 12 uint8_t header (three pages of data starting at page 4)
    memcpy(pageBuffer, pageHeader, 4);
    if (!(pn532_ntag2xx_WritePage(obj, 4, pageBuffer)))
        return 0;
    memcpy(pageBuffer, pageHeader + 4, 4);
    if (!(pn532_ntag2xx_WritePage(obj, 5, pageBuffer)))
        return 0;
    memcpy(pageBuffer, pageHeader + 8, 4);
    if (!(pn532_ntag2xx_WritePage(obj, 6, pageBuffer)))
        return 0;

    // Write URI (starting at page 7)
    uint8_t currentPage = 7;
    char *urlcopy = url;
    while (len)
    {
        if (len < 4)
        {
            memset(pageBuffer, 0, 4);
            memcpy(pageBuffer, urlcopy, len);
            pageBuffer[len] = 0xFE; // NDEF record footer
            if (!(pn532_ntag2xx_WritePage(obj, currentPage, pageBuffer)))
                return 0;
            // DONE!
            return 1;
        }
        else if (len == 4)
        {
            memcpy(pageBuffer, urlcopy, len);
            if (!(pn532_ntag2xx_WritePage(obj, currentPage, pageBuffer)))
                return 0;
            memset(pageBuffer, 0, 4);
            pageBuffer[0] = 0xFE; // NDEF record footer
            currentPage++;
            if (!(pn532_ntag2xx_WritePage(obj, currentPage, pageBuffer)))
                return 0;
            // DONE!
            return 1;
        }
        else
        {
            // More than one page of data left
            memcpy(pageBuffer, urlcopy, 4);
            if (!(pn532_ntag2xx_WritePage(obj, currentPage, pageBuffer)))
                return 0;
            currentPage++;
            urlcopy += 4;
            len -= 4;
        }
    }

    // Seems that everything was OK (?!)
    return 1;
}

/************** high level communication functions (handles both I2C and SPI) */

/**************************************************************************/
/*!
    @brief  Tries to read the SPI or I2C ACK signal
*/
/**************************************************************************/
bool pn532_readack(pn532_t *obj)
{
    uint8_t ackbuff[6];

    pn532_readdata(obj, ackbuff, 6);

    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/**************************************************************************/
/*!
    @brief  Return true if the PN532 is ready with a response.
*/
/**************************************************************************/
bool pn532_isready(pn532_t *obj)
{
    gpio_set_level(obj->_ss, 0);
    // gpio_set_level(PN532_SS, 0);
    PN532_DELAY(10);
    pn532_spi_write(obj, PN532_SPI_STATREAD);
    // read uint8_t
    uint8_t x = pn532_spi_read(obj);

    gpio_set_level(obj->_ss, 1);
    // gpio_set_level(PN532_SS, 1);

    // Check if status is ready.
    return x == PN532_SPI_READY;
}

/**************************************************************************/
/*!
    @brief  Waits until the PN532 is ready.
    @param  timeout   Timeout before giving up
*/
/**************************************************************************/
bool pn532_waitready(pn532_t *obj, uint16_t timeout)
{
    uint16_t timer = 0;
    while (!pn532_isready(obj))
    {
        if (timeout != 0)
        {
            timer += 10;
            if (timer > timeout)
            {
                configPRINTF(("TIMEOUT!\n"));
                return false;
            }
        }
        PN532_DELAY(10);
    }
    return true;
}

/**************************************************************************/
/*!
    @brief  Reads n bytes of data from the PN532 via SPI or I2C.
    @param  buff      Pointer to the buffer where data will be written
    @param  n         Number of bytes to be read
*/
/**************************************************************************/
void pn532_readdata(pn532_t *obj, uint8_t *buff, uint8_t n)
{
    gpio_set_level(obj->_ss, 0);
    // gpio_set_level(PN532_SS, 0);
    PN532_DELAY(10);
    pn532_spi_write(obj, PN532_SPI_DATAREAD);

    configPRINTF(("Reading: \n"));
    for (uint8_t i = 0; i < n; i++)
    {
        PN532_DELAY(10);
        buff[i] = pn532_spi_read(obj);
    }
    for (int i = 0; i < n; i++)
    {
        configPRINTF((" %02x \n", buff[i]));
    }
    configPRINTF(("\n"));

    gpio_set_level(obj->_ss, 1);
    // gpio_set_level(PN532_SS, 1);
}

/**************************************************************************/
/*!
    @brief  set the PN532 as iso14443a Target behaving as a SmartCard
    @param  None
    #author Salvador Mendoza(salmg.net) new functions:
    -AsTarget
    -getDataTarget
    -setDataTarget
*/
/**************************************************************************/
uint8_t pn532_AsTarget(pn532_t *obj)
{
    pn532_packetbuffer[0] = 0x8C;
    uint8_t target[] = {
        0x8C,             // INIT AS TARGET
        0x00,             // MODE -> BITFIELD
        0x08, 0x00,       //SENS_RES - MIFARE PARAMS
        0xdc, 0x44, 0x20, //NFCID1T
        0x60,             //SEL_RES
        0x01, 0xfe,       //NFCID2T MUST START WITH 01fe - FELICA PARAMS - POL_RES
        0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,                                    //PAD
        0xff, 0xff,                                                                        //SYSTEM CODE
        0xaa, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01, 0x00,            //NFCID3t MAX 47 BYTES ATR_RES
        0x0d, 0x52, 0x46, 0x49, 0x44, 0x49, 0x4f, 0x74, 0x20, 0x50, 0x4e, 0x35, 0x33, 0x32 //HISTORICAL BYTES
    };
    if (!pn532_sendCommandCheckAck(obj, target, sizeof(target), 1000))
        return false;

    // read data packet
    pn532_readdata(obj, pn532_packetbuffer, 8);

    int offset = 5;
    return (pn532_packetbuffer[offset] == 0x15);
}
/**************************************************************************/
/*!
    @brief  retrieve response from the emulation mode
    @param  cmd    = data
    @param  cmdlen = data length
*/
/**************************************************************************/
uint8_t pn532_getDataTarget(pn532_t *obj, uint8_t *cmd, uint8_t *cmdlen)
{
    uint8_t length;
    pn532_packetbuffer[0] = 0x86;
    if (!pn532_sendCommandCheckAck(obj, pn532_packetbuffer, 1, 1000))
    {
        configPRINTF(("Error en ack\n"));
        return false;
    }

    // read data packet
    pn532_readdata(obj, pn532_packetbuffer, 64);
    length = pn532_packetbuffer[3] - 3;

    //if (length > *responseLength) {// Bug, should avoid it in the reading target data
    //  length = *responseLength; // silent truncation...
    //}

    for (int i = 0; i < length; ++i)
    {
        cmd[i] = pn532_packetbuffer[8 + i];
    }
    *cmdlen = length;
    return true;
}

/**************************************************************************/
/*!
    @brief  set data in PN532 in the emulation mode
    @param  cmd    = data
    @param  cmdlen = data length
*/
/**************************************************************************/
uint8_t pn532_setDataTarget(pn532_t *obj, uint8_t *cmd, uint8_t cmdlen)
{
    uint8_t length;
    //cmd1[0] = 0x8E; Must!

    if (!pn532_sendCommandCheckAck(obj, cmd, cmdlen, 1000))
        return false;

    // read data packet
    pn532_readdata(obj, pn532_packetbuffer, 8);
    length = pn532_packetbuffer[3] - 3;
    for (int i = 0; i < length; ++i)
    {
        cmd[i] = pn532_packetbuffer[8 + i];
    }
    //cmdl = 0
    cmdlen = length;

    int offset = 5;
    return (pn532_packetbuffer[offset] == 0x15);
}

/**************************************************************************/
/*!
    @brief  Writes a command to the PN532, automatically inserting the
            preamble and required frame details (checksum, len, etc.)
    @param  cmd       Pointer to the command buffer
    @param  cmdlen    Command length in bytes
*/
/**************************************************************************/
void pn532_writecommand(pn532_t *obj, uint8_t *cmd, uint8_t cmdlen)
{
    uint8_t checksum;

    cmdlen++;

    configPRINTF(("Sending: \n"));

    gpio_set_level(obj->_ss, 0);
    // gpio_set_level(PN532_SS, 0);
    PN532_DELAY(10); // or whatever the PN532_DELAY is for waking up the board
    pn532_spi_write(obj, PN532_SPI_DATAWRITE);

    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    pn532_spi_write(obj, PN532_PREAMBLE);
    pn532_spi_write(obj, PN532_PREAMBLE);
    pn532_spi_write(obj, PN532_STARTCODE2);

    pn532_spi_write(obj, cmdlen);
    pn532_spi_write(obj, ~cmdlen + 1);

    pn532_spi_write(obj, PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

    configPRINTF((" %02x %02x %02x %02x %02x %02x \n ", (uint8_t)PN532_PREAMBLE, (uint8_t)PN532_PREAMBLE, (uint8_t)PN532_STARTCODE2, (uint8_t)cmdlen, (uint8_t)(~cmdlen + 1), (uint8_t)PN532_HOSTTOPN532));

    for (uint8_t i = 0; i < cmdlen - 1; i++)
    {
        pn532_spi_write(obj, cmd[i]);
        checksum += cmd[i];
        configPRINTF((" %02x \n", cmd[i]));
    }

    pn532_spi_write(obj, ~checksum);
    pn532_spi_write(obj, PN532_POSTAMBLE);
    gpio_set_level(obj->_ss, 1);
    // gpio_set_level(PN532_SS, 1);

    configPRINTF((" %02x %02x\n", (uint8_t)~checksum, (uint8_t)PN532_POSTAMBLE));
}
/************** low level SPI */

/**************************************************************************/
/*!
    @brief  Low-level SPI write wrapper
    @param  c       8-bit command to write to the SPI bus
*/
/**************************************************************************/
void pn532_spi_write(pn532_t *obj, uint8_t c)
{
    int8_t i;
    gpio_set_level(obj->_clk, 1);
    // gpio_set_level(PN532_SCK, 1);

    for (i = 0; i < 8; i++)
    {
        gpio_set_level(obj->_clk, 0);
        // gpio_set_level(PN532_SCK, 0);
        if (c & _BV(i))
        {
            gpio_set_level(obj->_mosi, 1);
            // gpio_set_level(PN532_MOSI, 1);
        }
        else
        {
            gpio_set_level(obj->_mosi, 0);
            // gpio_set_level(PN532_MOSI, 0);
        }
        gpio_set_level(obj->_clk, 1);
        // gpio_set_level(PN532_SCK, 1);
    }
}

/**************************************************************************/
/*!
    @brief  Low-level SPI read wrapper
    @returns The 8-bit value that was read from the SPI bus
*/
/**************************************************************************/
uint8_t pn532_spi_read(pn532_t *obj)
{
    int8_t i, x;
    x = 0;

    gpio_set_level(obj->_clk, 1);
    // gpio_set_level(PN532_SCK, 1);

    for (i = 0; i < 8; i++)
    {
        if (gpio_get_level(obj->_miso)) // (gpio_set_level(PN532_MISO, 1)) //(gpio_get_level(obj->_miso))
        {
            x |= _BV(i);
        }
        gpio_set_level(obj->_clk, 0);
        gpio_set_level(obj->_clk, 1);
        // gpio_set_level(PN532_SCK, 0);
        // gpio_set_level(PN532_SCK, 1);
    }

    return x;
}



// // Start sending and receiving data from PN532
// void pn532_init(spi_device_handle_t spi)
// {
//    // Delay 1 second
//    PN532_DELAY(1000);
//    packetBuffer[0] = COMMAND_GETFIRMWAREVERSION;

//    // test command

// }




// int main (void)
// {
//     esp_err_t ret;
//     spi_device_handle_t spi;

//     // Cnfigure the spi bus
//     spi_bus_config_t buscfg = {
//             .miso_io_num = PIN_NUM_MISO,
//             .mosi_io_num = PIN_NUM_MOSI,
//             .sclk_io_num = PIN_NUM_CLK,
//             .quadwp_io_num = -1,
//             .quadhd_io_num = -1
//     };
    
//     // Configure the spi interface
//     spi_device_interface_config_t devcfg = {
//         .command_bits = 0,
//         .address_bits = 0,
//         .dummy_bits = 0,
//         .clock_speed_hz = 10*1000*1000,  // Clock out at 10 MHz
//         .duty_cycle_pos = 128,
//         .mode = 0,                  // spi mode 0
//         // .spics_io_num = PIN_NUM_CS, // SS pin
//         // .cs_ena_posttrans = 3,
//         // .cs_ena_pretrans = 0,
//         .queue_size = 3             // Number of transactions to queue at a time
//     };

//     // Initialise spi bus
//     ret = spi_bus_initialize(PN532_HOST, &buscfg, DMA_CHAN);
//     ESP_ERROR_CHECK(ret);

//     // Register device
//     ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
//     ESP_ERROR_CHECK(ret);

//     // Interact with device
//     pn532_init(spi);

//     return 0;
// }
#ifndef NFC_TASKS_H
#define NFC_TASKS_H

// #define BLE_PROVISIONING provMode

int returnWifiStatus();
// bool getProvMode();
void storeCredentials();
void connectToNetwork();
void nfc_task(void *pvParameter);


#endif
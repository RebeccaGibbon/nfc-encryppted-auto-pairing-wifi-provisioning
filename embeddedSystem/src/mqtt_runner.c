/*
 * FreeRTOS V202012.00
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

/**
 * @file iot_demo_freertos.c
 * @brief Generic demo runner for C SDK libraries on FreeRTOS.
 */

/* The config header is always included first. */
#include "iot_config.h"

#include <string.h>
#include "aws_clientcredential.h"
#include "aws_clientcredential_keys.h"
#include "iot_network_manager_private.h"
#include "platform/iot_threads.h"
#include "iot_init.h"
#include "core_mqtt.h"
#include "iot_wifi.h"

#include "mqtt_runner.h"
#include "nfc_tasks.h"


#define PRIORITY    ( tskIDLE_PRIORITY + 5 )
#define STACKSIZE   ( configMINIMAL_STACK_SIZE * 8 )
// #define configENABLED_NETWORKS      ( AWSIOT_NETWORK_TYPE_WIFI )

static IotNetworkManagerSubscription_t subscription = IOT_NETWORK_MANAGER_SUBSCRIPTION_INITIALIZER;
// Semaphore used to wait for a network to be available.
static IotSemaphore_t networkSemaphore; 
// Variable used to indicate the connected network.
static uint32_t connectedNetwork = AWSIOT_NETWORK_TYPE_NONE;

/*-----------------------------------------------------------*/

static uint32_t _getConnectedNetwork( context_t * pContext )
{
    uint32_t ret = ( AwsIotNetworkManager_GetConnectedNetworks() & pContext->networkTypes );

    if( ( ret & AWSIOT_NETWORK_TYPE_WIFI ) == AWSIOT_NETWORK_TYPE_WIFI )
    {
        ret = AWSIOT_NETWORK_TYPE_WIFI;
    }
    else if( ( ret & AWSIOT_NETWORK_TYPE_BLE ) == AWSIOT_NETWORK_TYPE_BLE )
    {
        ret = AWSIOT_NETWORK_TYPE_BLE;
    }
    else if( ( ret & AWSIOT_NETWORK_TYPE_ETH ) == AWSIOT_NETWORK_TYPE_ETH )
    {
        ret = AWSIOT_NETWORK_TYPE_ETH;
    }
    else
    {
        ret = AWSIOT_NETWORK_TYPE_NONE;
    }

    return ret;
}
/*-----------------------------------------------------------*/

static uint32_t _waitForNetworkConnection( context_t * pContext )
{
    // Poll until successfull connection to network
    IotSemaphore_Wait( &networkSemaphore );

    return _getConnectedNetwork( pContext );
}
/*-----------------------------------------------------------*/

static void _onNetworkStateChangeCallback( uint32_t network,
                                           AwsIotNetworkState_t state,
                                           context_t * pContext )
{
    const IotNetworkInterface_t * pNetworkInterface = NULL;
    void * pConnectionParams = NULL, * pCredentials = NULL;
    uint32_t disconnectedNetworks = AWSIOT_NETWORK_TYPE_NONE;

    if( ( state == eNetworkStateEnabled ) && ( connectedNetwork == AWSIOT_NETWORK_TYPE_NONE ) )
    {
        connectedNetwork = network;
        IotSemaphore_Post( &networkSemaphore );

        // Disable the disconnected networks to save power and reclaim any unused memory. 
        disconnectedNetworks = configENABLED_NETWORKS & ( ~connectedNetwork );
        if( disconnectedNetworks != AWSIOT_NETWORK_TYPE_NONE )
        {
            AwsIotNetworkManager_DisableNetwork( disconnectedNetworks );
        }

        if( pContext->networkConnectedCallback != NULL )
        {
            pNetworkInterface = AwsIotNetworkManager_GetNetworkInterface( network );
            pConnectionParams = AwsIotNetworkManager_GetConnectionParams( network );
            pCredentials = AwsIotNetworkManager_GetCredentials( network ),

            pContext->networkConnectedCallback( true,
                                                clientcredentialIOT_THING_NAME,
                                                pConnectionParams,
                                                pCredentials,
                                                pNetworkInterface );
        }
    }
    else if( ( state == eNetworkStateDisabled ) && ( connectedNetwork == network ) )
    {
        if( pContext->networkDisconnectedCallback != NULL )
        {
            pNetworkInterface = AwsIotNetworkManager_GetNetworkInterface( network );
            pContext->networkDisconnectedCallback( pNetworkInterface );
        }

        // Re-enable all the networks for reconnection.
        disconnectedNetworks = configENABLED_NETWORKS & ( ~connectedNetwork );

        if( disconnectedNetworks != AWSIOT_NETWORK_TYPE_NONE )
        {
            AwsIotNetworkManager_EnableNetwork( disconnectedNetworks );
        }

        connectedNetwork = _getConnectedNetwork( pContext );

        if( connectedNetwork != AWSIOT_NETWORK_TYPE_NONE )
        {
            if( pContext->networkConnectedCallback != NULL )
            {
                pNetworkInterface = AwsIotNetworkManager_GetNetworkInterface( connectedNetwork );
                pConnectionParams = AwsIotNetworkManager_GetConnectionParams( connectedNetwork );
                pCredentials = AwsIotNetworkManager_GetCredentials( connectedNetwork );

                pContext->networkConnectedCallback( true,
                                                    clientcredentialIOT_THING_NAME,
                                                    pConnectionParams,
                                                    pCredentials,
                                                    pNetworkInterface );
            }
        }
    }
}
/*-----------------------------------------------------------*/

static int _initialize (context_t * pContext)
{
    int status = EXIT_SUCCESS;
    bool commonLibrariesInitialized = false;
    bool semaphoreCreated = false;

    /* Initialize the C-SDK common libraries. This function must be called
    * once (and only once) before calling any other C-SDK function. */

    if (IotSdk_Init() == true)
    {
        commonLibrariesInitialized = true;
    }
    else
    {
        configPRINTF( ("Failed to initialize the common library......\n") );
        status =  EXIT_FAILURE;
    }

    if (status == EXIT_SUCCESS)
    {
        if (AwsIotNetworkManager_Init() != pdTRUE)
        {
        configPRINTF( ("Failed to initialize network manager library \n") );
        status = EXIT_FAILURE;
        }
    }

    if ( status == EXIT_SUCCESS )
    {
        // Create a semaphore to signal that a network is available
        if ( IotSemaphore_Create( &networkSemaphore, 0, 1) != true ) 
        {
        configPRINTF ( ("Failed to create semaphore to wait for a network connection.") );
        status = EXIT_FAILURE;
        }
        else
        {
        semaphoreCreated = true;
        }
    }

    if( status == EXIT_SUCCESS )
    {
        /* Subscribe for network state change from Network Manager. */
        if( AwsIotNetworkManager_SubscribeForStateChange( pContext->networkTypes,
                                                            _onNetworkStateChangeCallback,
                                                            pContext,
                                                            &subscription ) != pdTRUE )
        {
            configPRINTF( ("Failed to subscribe network state change callback. \n") );
            status = EXIT_FAILURE;
        }
    }

    // Initialize all the  networks configured for the device. 
    if( status == EXIT_SUCCESS )
    {
        if( AwsIotNetworkManager_EnableNetwork( configENABLED_NETWORKS ) != configENABLED_NETWORKS )
        {
            configPRINTF(( "Enabled network: %d \n",  configENABLED_NETWORKS));
            configPRINTF( ("Failed to intialize all the networks configured for the device. \n") );
            // Put nfc task here....?
            xTaskCreate(nfc_task, "nfc_task", 4096, NULL, 4, NULL);
            status = EXIT_FAILURE;
        }
    }

    if( status == EXIT_SUCCESS )
    {
        /* Wait for network configured for the demo to be initialized. */
            if( pContext->networkTypes != AWSIOT_NETWORK_TYPE_NONE )
            {
                configPRINTF(( "Checking for connected networks. " ));
                connectedNetwork = _getConnectedNetwork( pContext );

                if( connectedNetwork == AWSIOT_NETWORK_TYPE_NONE )
                {
                    /* Network not yet initialized. Block for a network to be initialized. */
                    configPRINTF(( "No networks connected for the demo. Waiting for a network connection. " ));
                    connectedNetwork = _waitForNetworkConnection( pContext );
                }
            }
    }

    if( status == EXIT_FAILURE )
    {
        configPRINTF( ("Initialization failed \n") );
        if( semaphoreCreated == true )
        {
            IotSemaphore_Destroy( &networkSemaphore );
        }

        if( commonLibrariesInitialized == true )
        {
            IotSdk_Cleanup();
        }
    }

    return status;
}
/*-----------------------------------------------------------*/

static void _cleanup(void)
{
    // Remove network manager subscription
    AwsIotNetworkManager_RemoveSubscription(subscription);
    // Disable all the networks used by the demo.
    AwsIotNetworkManager_DisableNetwork(configENABLED_NETWORKS);
    IotSemaphore_Destroy(&networkSemaphore);
    IotSdk_Cleanup();
}
/*-----------------------------------------------------------*/


void runMqttTask(void * pArgument)
{
    context_t * pContext = ( context_t * ) pArgument;
    const IotNetworkInterface_t * pNetworkInterface = NULL;
    void * pConnectionParams = NULL, * pCredentials = NULL;
    int status;

    configPRINTF( ( "-----------STARTING MQTT DEMO-----------\n " ) );

    // Checks for network status in initialize function
    status = _initialize( pContext );

    if( status == EXIT_SUCCESS )
    {
        configPRINTF( ( "Successfully initialized. Network type: %d\n ", connectedNetwork ) );

        pNetworkInterface = AwsIotNetworkManager_GetNetworkInterface( connectedNetwork );
        pConnectionParams = AwsIotNetworkManager_GetConnectionParams( connectedNetwork );
        pCredentials = AwsIotNetworkManager_GetCredentials( connectedNetwork );

        // Run MQTT demo
        status = pContext->mqttFunctionRun (true,
                                        clientcredentialIOT_THING_NAME,
                                        pConnectionParams,
                                        pCredentials,
                                        pNetworkInterface);

        /* Give a chance to drain the logging queue to increase the probability
        * of the following messages used by the test framework not getting
        * dropped. */
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );

        /* Log the demo status. */
        if( status == EXIT_SUCCESS )
        {
            /* DO NOT EDIT - This message is used in the test framework to
            * determine whether or not the demo was successful. */
            configPRINTF(( "MQTT demo completed successfully." ));
        }
        else
        {
            configPRINTF(( "Error running MQTT demo." ));
        }

        _cleanup();
    }
    else
    {
        configPRINTF(( "Failed to initialize the demo. exiting..." ));
    }

configPRINTF( ( "-----------MQTT DEMO FINISHED-----------\n " ) );
}

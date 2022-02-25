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

/* Called from all the example projects to start tasks that demonstrate Amazon
 * FreeRTOS libraries.
 *
 * If the project was created using the AWS console then this file will have been
 * auto generated and only reference and start the demos that were selected in the
 * console.  If the project was obtained from a source control repository then this
 * file will reference all the available and the developer can selectively comment
 * in or out the demos to execute. */

/* The config header is always included first. */
#include "iot_config.h"
#include "iot_network_manager_private.h"

typedef int (* fnPointer  )( bool awsIotMqttMode,
                            const char * pIdentifier,
                            void * pNetworkServerInfo,
                            void * pNetworkCredentialInfo,
                            const IotNetworkInterface_t * pNetworkInterface);

typedef void (* networkConnectedCallback_t)( bool awsIotMqttMode,
                                             const char * pIdentifier,
                                             void * pNetworkServerInfo,
                                             void * pNetworkCredentialInfo,
                                             const IotNetworkInterface_t * pNetworkInterface );

typedef void (* networkDisconnectedCallback_t)( const IotNetworkInterface_t * pNetworkInteface );

typedef struct context
{
  uint32_t networkTypes;
  fnPointer mqttFunctionRun;
  networkConnectedCallback_t networkConnectedCallback;
  networkDisconnectedCallback_t networkDisconnectedCallback;
} context_t;

int RunCoreMqttMutualAuthDemo( bool awsIotMqttMode,
                               const char * pIdentifier,
                               void * pNetworkServerInfo,
                               void * pNetworkCredentialInfo,
                               const IotNetworkInterface_t * pNetworkInterface );

void runMqttTask(void * pArgument);

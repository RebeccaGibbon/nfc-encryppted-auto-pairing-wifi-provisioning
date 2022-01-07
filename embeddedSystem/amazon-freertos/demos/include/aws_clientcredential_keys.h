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

/*
 ****************************************************************************
 * NOTE!
 * This file is for ease of demonstration only.  Secret information should not
 * be pasted into the header file in production devices.  Do not paste
 * production secrets here!  Production devices should store secrets such as
 * private keys securely, such as within a secure element.  See our examples that
 * demonstrate how to use the PKCS #11 API for secure keys access.
 ****************************************************************************
 */

#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

/*
 * @brief PEM-encoded client certificate.
 *
 * @todo If you are running one of the FreeRTOS demo projects, set this
 * to the certificate that will be used for TLS client authentication.
 *
 * @note Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyCLIENT_CERTIFICATE_PEM                   
"-----BEGIN CERTIFICATE-----\n"\
"MIIDWTCCAkGgAwIBAgIUV9i1Grthbh3gCw5I5PCWpjAkMMIwDQYJKoZIhvcNAQEL\n"\
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"\
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMDkzMDE3MDAz\n"\
"NloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"\
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALZb8PegGxMmZ5Yl2DVS\n"\
"ASP0IzerEyYevaqrZG8c/0sU8eXTIvC2izP7c6PY/Ey5pgkk2VDehZLVYlqBb6fB\n"\
"QbRKoiiqSumAzjkFTD3fskuc/WrZ6nAh+QvssHLTpi5C6ZoH7TmH+4b8PwlRedNZ\n"\
"k46feYd/QqIDGsqO/SQb/5JhImKASmfc5oHpwyB3UyZrzVeWFIdA2tHaitarSW6D\n"\
"ooo+paJ84rFK58eTaS/BgPDdVBCCBuK2MjnVmJJ2T19SmQ3aAhN+c+1t8RfCpYqc\n"\
"rtKrx06nrdge3McjKGLpWaFkeJPQE536ukzxmK0pA934n3+zUznP70UIaG66gOSY\n"\
"TTUCAwEAAaNgMF4wHwYDVR0jBBgwFoAUNTZQiBlczgun1juZWtdmsvi4oA8wHQYD\n"\
"VR0OBBYEFDKkKJaG74yxATLW5gD4M1bQcCJwMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"\
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAXPxAeVci/LNC0PcLO4M0ZU+hx\n"\
"8ajyV9JBYUdxcdmY9qG/Z7gN2R2u6GpAq5jeprzaEq96Yz+ShXheuepAGyC1F9H5\n"\
"6+SqKOULqx1xlzsXkftb31QJFWkgMVeLhPWw7aqVM7fZz63bCycdNefaPTm6DrA2\n"\
"XHeJHSyBwouNkEpXXHtaNWhso2P2mN4gwA+aLjXwjQ5MW0ZNfYKcJg6xgd69joAp\n"\
"DgAVdhNHQ2GPyP1hq1GMNvhXXYJ9H+BsgCZYY1Sz7+G4e+DsehDcO0X2IEVpk72o\n"\
"e/2XVmHyZGBi2JNE2CfeeYeMxFSPjhDJ64Cp2X23zXPFAdxo9UZOaGtiorV/\n"\
"-----END CERTIFICATE-----\n"

/*
 * @brief PEM-encoded issuer certificate for AWS IoT Just In Time Registration (JITR).
 *
 * @todo If you are using AWS IoT Just in Time Registration (JITR), set this to
 * the issuer (Certificate Authority) certificate of the client certificate above.
 *
 * @note This setting is required by JITR because the issuer is used by the AWS
 * IoT gateway for routing the device's initial request. (The device client
 * certificate must always be sent as well.) For more information about JITR, see:
 *  https://docs.aws.amazon.com/iot/latest/developerguide/jit-provisioning.html,
 *  https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/.
 *
 * If you're not using JITR, set below to NULL.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM    NULL

/*
 * @brief PEM-encoded client private key.
 *
 * @todo If you are running one of the FreeRTOS demo projects, set this
 * to the private key that will be used for TLS client authentication.
 * Please note pasting a key into the header file in this manner is for
 * convenience of demonstration only and should not be done in production.
 * Never past a production private key here!.  Production devices should
 * store keys securely, such as within a secure element.  Additionally,
 * we provide the corePKCS library that further enhances security by
 * enabling keys to be used without exposing them to software.
 *
 * @note Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 */
#define keyCLIENT_PRIVATE_KEY_PEM                
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEpQIBAAKCAQEAtlvw96AbEyZnliXYNVIBI/QjN6sTJh69qqtkbxz/SxTx5dMi\n"\
"8LaLM/tzo9j8TLmmCSTZUN6FktViWoFvp8FBtEqiKKpK6YDOOQVMPd+yS5z9atnq\n"\
"cCH5C+ywctOmLkLpmgftOYf7hvw/CVF501mTjp95h39CogMayo79JBv/kmEiYoBK\n"\
"Z9zmgenDIHdTJmvNV5YUh0Da0dqK1qtJboOiij6lonzisUrnx5NpL8GA8N1UEIIG\n"\
"4rYyOdWYknZPX1KZDdoCE35z7W3xF8Klipyu0qvHTqet2B7cxyMoYulZoWR4k9AT\n"\
"nfq6TPGYrSkD3fiff7NTOc/vRQhobrqA5JhNNQIDAQABAoIBAQCwQoPRxPYGd3tu\n"\
"HFwzhXTGvKYBm6Bhmxr2SL1FzMBRYQhCfOZrxfUcpHHsr8GFo3eBpRVL3hk7iHaN\n"\
"RkiW2+vXCMBeD+NiePZbFOzRbVNXVIF5cvHvulaPGRKfmkUot9I06G55yh7rJQsE\n"\
"xbS7/PvrwebkebP9BoUxvi2dZlaLsyX9poFQ0/0JV7GYHhp5H6D5wy2dDapoCtf9\n"\
"dEbPWeQIY89e5X9YXT3icYf30B6WRDNjQZSCFJGEZPUgKnSk6u0SEbzZghO5pTri\n"\
"aMOwnZRa2pcGMOIRqhNUvYgVvnnIZpt5mrr0yYEd4r1S35e/c9Y8KXtpHRzgJFsZ\n"\
"0Psd2ydBAoGBAOxSzIwiOOFwxj/+V+lfsfneyLBArYCbGviQRxovJEZA5k5mNLVF\n"\
"ShF1SsXFwe7BnrKGaKE/HYkBjY6zbnmNX+8cv+voTX9H2wwykGE6EX+JM8ywNq+k\n"\
"KzaNbKHdea7ILQvuA8FN44cMaKYFVjsD7sTy6vuw/LmID+v7oyKqc4ZRAoGBAMWK\n"\
"5qgNIQ5WJFpkofJeixEuj9gj/AWTqpF8VqD3AUvYWWmmK5BJSBTShLtRO4XjoKrx\n"\
"HRZ2StijJSrnoNZ/X11G9yY8ddPcPtoVAthaKDETV8tIiNgTFCh1g3i/Rh2zT53S\n"\
"c0alu7hoyI/k9A35l1CR3iWUskn+EFyL32Eb+UulAoGBAOEqpwAiH0YVuJ8CjOqs\n"\
"Jhxou5jLl0TBBDqu5UA6iHpznbw3P6MziLYHpp23pecCDl4VGJ4CT/bZRvIfXnRs\n"\
"Umq2Xtq8NRK/+SKvDsfElyWIKZWuk/xYFGBe8ZFsP2NcESebuXI170MefJsq3JVQ\n"\
"UvDObzwuzy1LSbhTw4TlGTtxAoGBAL1eN1k4JM0ihPzmZVz7huL0uM4hSNJryWJo\n"\
"SH9L6kEz6BrgQfiyV1gmqzkKWT4zhOz58MF8qqlcVB/6JfQZ7Gh1xC0yXl23T8Ut\n"\
"ySlv3AGXolzsbsp8F6HlbSAOdH+6SgNoIDvEWt0gDcvrVo4jCwwgmU17ayKFXFfr\n"\
"ejsUXvpFAoGAYbfRNenAArGWm55/WV26voDB6EcShzsVkzan73m8zSMnR7U3Lwey\n"\
"92gYe8RbR5T2KnnGDytzC42TdMlN23AG0HCHzxOXSQ2ETqRL1LlFwruD7yYuvpCF\n"\
"+h4Fl7uvqb+pDQQfBMopf9ToiAoX4+4yST/w13yDGy+5qaSKIKrqNdE=\n"
"-----END RSA PRIVATE KEY-----\n"

#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */

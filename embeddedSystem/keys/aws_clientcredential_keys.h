/*
 * FreeRTOS V201906.00 Major
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

#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

/*
 * PEM-encoded client certificate
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyCLIENT_CERTIFICATE_PEM \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDWjCCAkKgAwIBAgIVAJ9LUqHNYVcUBXqL7AT42QfmATFoMA0GCSqGSIb3DQEB\n"\
"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"\
"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMTEwMDIwMDMw\n"\
"MDhaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"\
"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC1I8xNCj1N8YUDq0me\n"\
"RhVcJkQ60zwyTiq2hXJLywXQibT98fd0H+P6/RkmY0Zy5NMX8f/Rj3AlvFN1ILmg\n"\
"E8jfTBy505XfudGGujk8FBr7DC5F+2koXGc4SxFwQX9rXFKaj8elecsH982EvtWk\n"\
"BVy6WqGc0xPV0+DC6MubbxAhNSVV0rf58PB9zbACZ72XOWZTMWBxPjjvgIMw9xFQ\n"\
"7PSlX1A8qH4WP0BOCr8e9apQA0aMmtbadwDBl84nCkGly3PnezZ4C1lXjuDDpLQ3\n"\
"roDz8DXiW/ajFBkQ/Dm8RRE47g5IlXpdxWTi9yU1i/8Xq7IAgsn2yuGR+QLAr794\n"\
"uIF3AgMBAAGjYDBeMB8GA1UdIwQYMBaAFDV5x2EpG1GvzYx8D6YVWF0DI51iMB0G\n"\
"A1UdDgQWBBRSOqJHZMfIzERx8VajAju992QvPjAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"\
"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAOw1GDen+ovpK0v0FxZ6nEiTD\n"\
"Shfgk2az0tvTmR2gHGsLRtUBCdCuON3dqasILHDnYNAur9oA9kFrxebrhtehhgWq\n"\
"SpLo7wIGAR9rAUZUeqY+k2Dn6F+RIlahdsL92U86WtpM80qr/w9nXQFTFpz0HhpI\n"\
"67m3bKeDyNOM3Y4o5+LoLQsf+1bVIWcvIsQHc/mBx6S3bP06hRflZnXVxxjmHWnb\n"\
"Gkml/1oMAcBu301PHIZYbWfyNNGvZUEJCJaI1R5b4MfzxjJGFLNK7417qu/eh5Dg\n"\
"nHSfWSLgv0oHa6LghIPz9pOaeHqqPGACd/wZWyjy51YrMgJYFDHCo7UBr9tLVQ==\n"\
"-----END CERTIFICATE-----\n"

/*
 * PEM-encoded issuer certificate for AWS IoT Just In Time Registration (JITR).
 * This is required if you're using JITR, since the issuer (Certificate 
 * Authority) of the client certificate is used by the server for routing the 
 * device's initial request. (The device client certificate must always be 
 * sent as well.) For more information about JITR, see:
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
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM  NULL

/*
 * PEM-encoded client private key.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 */
#define keyCLIENT_PRIVATE_KEY_PEM \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEowIBAAKCAQEAtSPMTQo9TfGFA6tJnkYVXCZEOtM8Mk4qtoVyS8sF0Im0/fH3\n"\
"dB/j+v0ZJmNGcuTTF/H/0Y9wJbxTdSC5oBPI30wcudOV37nRhro5PBQa+wwuRftp\n"\
"KFxnOEsRcEF/a1xSmo/HpXnLB/fNhL7VpAVculqhnNMT1dPgwujLm28QITUlVdK3\n"\
"+fDwfc2wAme9lzlmUzFgcT4474CDMPcRUOz0pV9QPKh+Fj9ATgq/HvWqUANGjJrW\n"\
"2ncAwZfOJwpBpctz53s2eAtZV47gw6S0N66A8/A14lv2oxQZEPw5vEUROO4OSJV6\n"\
"XcVk4vclNYv/F6uyAILJ9srhkfkCwK+/eLiBdwIDAQABAoIBABVWajKJCj+sO6b6\n"\
"/mkOiGWafF8Rkv26B34IqjYTbFPsZM2lOykKuKhQOC41c48FNXiUoTXmV6eaT5p0\n"\
"iGk662aiaJ/bNMRt9yiPtz5EgAkinjDQNMr0V5RiA2Pd00yqS3C5jyMG/cuqfSMt\n"\
"96yilRG+q1r14lhF5SwQkCt3IcdHcJjLYybp75JJZ4PF5uRZARPZrgmTm7RY9ZEf\n"\
"7Xiqf3QI+AElNFNxY713Qm18WDDMNTczd3AAv9xzcH0tEGOVmLPxTYN715TcyI2T\n"\
"rnqnYodzqsdRSdKHBS4U6esj+0rxyPgXRpOcRMYfej7orcnhbr4cVwCGlFkwnCvY\n"\
"GoZLwcECgYEA4vIx6ZoO68RYAgAA6JNDPJa4vKc8hqkpl85WOjvXZdZPalLYratm\n"\
"7sahKRibx9bqVN79Gf/wCwwqOLjnzy/4dhGD8/P7TGsfFUBtkLndzMMHHvnpjyOK\n"\
"vwFp0z/KPe2Xn60iQsLAWTWgE+J/UuZHjojtovttdvkZeA413qg7oJMCgYEAzFRf\n"\
"8hyY2Zu3JTdHcKnEo98h1Dpu0ZRxpleRw/ZP6qVl+1TQbbW7KgoZNU3A5Gy/m+DL\n"\
"JYaw4Rv74MAwUprD/lC57kMF3jXGoP0bJjMTYkGu6wl4/6Ut1pNZXTMQw2mtUNVG\n"\
"KbcLqjz7zCN3bTxLLlvkCaOiwmoExOmDe4dTfg0CgYEAhS93xU0pJooS0BGqdtM9\n"\
"qUgW4HgQ2EJca3+nMQO/dD9iWdaG3NzzSnv9Rn3vpkblPKukw+ZkvEcGXEV/nSLc\n"\
"4IWE990ehIl1M2adJks99JYcDrzBqcRE/7VXoqcUDGXsDsRvj9h1Q6Vp7WGGMWN9\n"\
"aRPIylZdtm2X2dIVITd/zssCgYBo5cQf+php1uwuGfP+HdnKuymV162+wHOcDwbR\n"\
"UKl0cBcr8T3TpOGCUo7gc5tcaYvwzXXdPQmfd4tWtbPZJIUMEYtyi0v+6SKg0W/X\n"\
"q47++XOATGum6rjemOTe2PF5MM5+Wg9cKRG/AvHNukN8e+Co2xn17OEI5/6szKjo\n"\
"iO/PHQKBgBiy7uQYUr6GaouyLxBE1D3dNqI5uw6MP0ibcp8XDZdqhZ8znSCW7Z2D\n"\
"gkqaY0MvRkwtOOsTJkmgjtjWQmbjbdFKePHxBbCKOV8BpUEuArqjl1Tqbv/6fO3D\n"\
"5J+0WeO065EXVbFBNycz1vhTyRyJIm/jZrqqEDdVnQU6f5PXW9J2\n"\
"-----END RSA PRIVATE KEY-----\n"


#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */

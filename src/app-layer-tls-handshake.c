/*
 * Copyright (C) 2011-2012 ANSSI
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * \author Pierre Chifflier <pierre.chifflier@ssi.gouv.fr>
 *
 * \brief Decode TLS Handshake messages, as described in RFC2246
 *
 */

#include "suricata-common.h"
#include "debug.h"
#include "decode.h"

#include "app-layer-ssl.h"

#include "app-layer-tls-handshake.h"

#include <stdint.h>

#include "util-decode-der.h"
#include "util-decode-der-get.h"

#define SSLV3_RECORD_LEN 5

int DecodeTLSHandshakeServerHello(SSLState *ssl_state, uint8_t *input, uint32_t input_len)
{
    uint32_t version, length, ciphersuite;
    uint8_t compressionmethod;

    if (input_len < 40)
        return -1;

    version = input[0]<<8 | input[1];
    ssl_state->handshake_server_hello_ssl_version = version;

    input += 2;
    input_len -= 2;

    /* skip the random field */
    input += 32;

    /* skip the session ID */
    length = input[0];
    input += 1 + length;

    ciphersuite = input[0]<<8 | input[1];
    ssl_state->ciphersuite = ciphersuite;

    input += 2;

    compressionmethod = input[0];
    ssl_state->compressionmethod = compressionmethod;

    input += 1;

    /* extensions (like renegotiation) */

    SCLogDebug("TLS Handshake Version %.4x Cipher %d Compression %d\n", version, ciphersuite, compressionmethod);

    return ssl_state->message_length;
}

int DecodeTLSHandshakeServerCertificate(SSLState *ssl_state, uint8_t *input, uint32_t input_len)
{
    uint32_t certificates_length, cur_cert_length;
    int i;
    Asn1Generic *cert;
    char buffer[256];
    int rc;
    int parsed;

    if (input_len < 3)
        return 1;

    certificates_length = input[0]<<16 | input[1]<<8 | input[2];
    /* check if the message is complete */
    if (input_len < certificates_length + 3)
        return 0;

    input += 3;
    parsed = 3;

    i = 0;
    while (certificates_length > 0) {
        cur_cert_length = input[0]<<16 | input[1]<<8 | input[2];
        input += 3;
        parsed += 3;

        cert = DecodeDer(input, cur_cert_length);
        if (cert == NULL) {
            SCLogWarning(SC_ERR_ALPARSER, "decoding ASN.1 structure for X509 certificate failed\n");
        }
        if (cert != NULL) {
            rc = Asn1DerGetSubjectDN(cert, buffer, sizeof(buffer));
            if (rc != 0) {
                SCLogWarning(SC_ERR_ALPARSER, "X509: could not get subject\n");
            } else {
                //SCLogInfo("TLS Cert %d: %s\n", i, buffer);
                if (i==0) {
                    ssl_state->cert0_subject = SCStrdup(buffer);
                }
            }
            rc = Asn1DerGetIssuerDN(cert, buffer, sizeof(buffer));
            if (rc != 0) {
                SCLogWarning(SC_ERR_ALPARSER, "X509: could not get issuerdn\n");
            } else {
                //SCLogInfo("TLS IssuerDN %d: %s\n", i, buffer);
                if (i==0) {
                    ssl_state->cert0_issuerdn = SCStrdup(buffer);
                }
            }
            DerFree(cert);
        }

        i++;
        certificates_length -= (cur_cert_length + 3);
        parsed += cur_cert_length;
        input += cur_cert_length;
    }

    return parsed;
}


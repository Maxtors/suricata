#include <suricata-common.h>

#include "app-layer-parser.h"

#include "app-layer-htp.h"
#include "app-layer-smtp.h"

#include "util-buffer.h"
#include "util-print.h"

#include "log-file-common.h"

#ifdef HAVE_LIBJANSSON
#include <jansson.h>
#endif

void LogFileMetaGetSmtpMessageID(const Packet *p, const File *ff, MemBuffer *buffer, uint32_t fflag) {
    SMTPState *state = (SMTPState *) p->flow->alstate;
    if (state != NULL) {
        SMTPTransaction *tx = AppLayerParserGetTx(IPPROTO_TCP, ALPROTO_SMTP, state, ff->txid);
        if (tx == NULL || tx->msg_tail == NULL)
            return;

        /* Message Id */
        if (tx->msg_tail->msg_id != NULL) {
                PrintRawUriBuf((char *)buffer->buffer, &buffer->offset, buffer->size,
                               (uint8_t *)tx->msg_tail->msg_id, tx->msg_tail->msg_id_len);
        }

    }
}

void LogFileMetaGetSmtpSender(const Packet *p, const File *ff, MemBuffer *buffer, uint32_t fflag) {
    SMTPState *state = (SMTPState *) p->flow->alstate;
    if (state != NULL) {
        SMTPTransaction *tx = AppLayerParserGetTx(IPPROTO_TCP, ALPROTO_SMTP, state, ff->txid);
        if (tx == NULL || tx->msg_tail == NULL)
            return;

        /* Sender */
        MimeDecField *field = MimeDecFindField(tx->msg_tail, "from");
        if (field != NULL) {
                PrintRawUriBuf((char *)buffer->buffer, &buffer->offset, buffer->size,
                               (uint8_t *) field->value, field->value_len);
        }

    }
}


void LogFileMetaGetUri(const Packet *p, const File *ff, MemBuffer *buffer, uint32_t fflag) {
    HtpState *htp_state = (HtpState *)p->flow->alstate;
    if (htp_state != NULL) {
        htp_tx_t *tx = AppLayerParserGetTx(IPPROTO_TCP, ALPROTO_HTTP, htp_state, ff->txid);
        if (tx != NULL) {
            HtpTxUserData *tx_ud = htp_tx_get_user_data(tx);
            if (tx_ud->request_uri_normalized != NULL) {
                PrintRawUriBuf((char *)buffer->buffer, &buffer->offset,
                               buffer->size,
                               (uint8_t *)bstr_ptr(tx_ud->request_uri_normalized),
                               bstr_len(tx_ud->request_uri_normalized));
                return;
            }
            return;
        }
    }
    snprintf((char *)buffer->buffer, buffer->size, "unknown");
}

void LogFileMetaGetHost(const Packet *p, const File *ff, MemBuffer *buffer, uint32_t fflag) {
    HtpState *htp_state = (HtpState *)p->flow->alstate;
    if (htp_state != NULL) {
        htp_tx_t *tx = AppLayerParserGetTx(IPPROTO_TCP, ALPROTO_HTTP, htp_state, ff->txid);
        if (tx != NULL && tx->request_hostname != NULL) {
            PrintRawUriBuf((char *)buffer->buffer, &buffer->offset, buffer->size,
                           (uint8_t *)bstr_ptr(tx->request_hostname),
                           bstr_len(tx->request_hostname));
            return;
        }
    }
    snprintf((char *)buffer->buffer, buffer->size, "unknown");
}

void LogFileMetaGetReferer(const Packet *p, const File *ff, MemBuffer *buffer, uint32_t fflag) {
    HtpState *htp_state = (HtpState *)p->flow->alstate;
    if (htp_state != NULL) {
        htp_tx_t *tx = AppLayerParserGetTx(IPPROTO_TCP, ALPROTO_HTTP, htp_state, ff->txid);
        if (tx != NULL) {
            htp_header_t *h = NULL;
            h = (htp_header_t *)htp_table_get_c(tx->request_headers, "Referer");
            if (h != NULL) {
                PrintRawUriBuf((char *)buffer->buffer, &buffer->offset,
                               buffer->size, (uint8_t *)bstr_ptr(h->value),
                               bstr_len(h->value));
                return;
            }
        }
    }
    snprintf((char *)buffer->buffer, buffer->size, "unknown");
}

void LogFileMetaGetUserAgent(const Packet *p, const File *ff, MemBuffer *buffer, uint32_t fflag) {
    HtpState *htp_state = (HtpState *)p->flow->alstate;
    if (htp_state != NULL) {
        htp_tx_t *tx = AppLayerParserGetTx(IPPROTO_TCP, ALPROTO_HTTP, htp_state, ff->txid);
        if (tx != NULL) {
            htp_header_t *h = NULL;
            h = (htp_header_t *)htp_table_get_c(tx->request_headers, "User-Agent");
            if (h != NULL) {
                PrintRawUriBuf((char *)buffer->buffer, &buffer->offset,
                               buffer->size, (uint8_t *)bstr_ptr(h->value),
                               bstr_len(h->value));
                return;
            }
        }
    }
    snprintf((char *)buffer->buffer, buffer->size, "unknown");
}

#ifdef HAVE_LIBJANSSON
void LogFileLogPrintJsonObj(FILE *fp, json_t *js) {
    char *js_data = json_dumps(js, JSON_PRESERVE_ORDER|JSON_COMPACT|JSON_ENSURE_ASCII|JSON_ESCAPE_SLASH);
    fprintf(fp, "%s", js_data);
}

void LogFileLogTransactionMeta(const Packet *p, const File *ff, json_t *js) {
    MemBuffer *buffer;
    buffer = MemBufferCreateNew(META_BUFFER_SIZE);

    if (p->flow->alproto == ALPROTO_HTTP) {
        json_t *http = json_object();

        LogFileMetaGetUri(p, ff, buffer, META_FORMAT_JSON);
        json_object_set_new(http, "uri", json_string((char *)buffer->buffer));
        MemBufferReset(buffer);
    
        LogFileMetaGetHost(p, ff, buffer, META_FORMAT_JSON);
        json_object_set_new(http, "host", json_string((char *)buffer->buffer));
        MemBufferReset(buffer);
    
        LogFileMetaGetReferer(p, ff, buffer, META_FORMAT_JSON);
        json_object_set_new(http, "referer", json_string((char *)buffer->buffer));
        MemBufferReset(buffer);
    
        LogFileMetaGetUserAgent(p, ff, buffer, META_FORMAT_JSON);
        json_object_set_new(http, "useragent", json_string((char *)buffer->buffer));
        MemBufferReset(buffer);
        
        json_object_set_new(js, "http", http);
    } else if (p->flow->alproto == ALPROTO_SMTP) {
        json_t *smtp = json_object();
    
        LogFileMetaGetSmtpMessageID(p, ff, buffer, META_FORMAT_JSON);
        json_object_set_new(smtp, "message-id", json_string((char *)buffer->buffer));
        MemBufferReset(buffer);

        LogFileMetaGetSmtpSender(p, ff, buffer, META_FORMAT_JSON);
        json_object_set_new(smtp, "sender", json_string((char *)buffer->buffer));
        json_object_set_new(js, "smtp", smtp);
    }

    MemBufferFree(buffer);
}


void LogFileLogFileMeta(const Packet *p, const File *ff, json_t *js) {
    json_t *container = json_object();

    MemBuffer *buffer;
    buffer = MemBufferCreateNew(META_BUFFER_SIZE);
    
    PrintRawUriBuf((char *)buffer->buffer, &buffer->offset, buffer->size, ff->name, ff->name_len);
    json_object_set_new(container, "filename", json_string((char *)buffer->buffer));

    if (ff->magic == NULL) {
        json_object_set_new(container, "magic", json_string("unknown"));
    } else {
        json_object_set_new(container, "magic", json_string(ff->magic));
    }

    switch(ff->state) {
        case FILE_STATE_CLOSED:
            json_object_set_new(container, "state", json_string("closed"));
#ifdef HAVE_NSS
            if (ff->flags & FILE_MD5) {
                char md5_buffer[META_MD5_BUFFER];
                size_t x;
                for (x = 0; x < sizeof(ff->md5); x++) {
                    snprintf(md5_buffer, META_MD5_BUFFER, "%02x", ff->md5[x]);
                }
                json_object_set_new(container, "md5", json_string(md5_buffer));
            }
#endif
            break;
        case FILE_STATE_TRUNCATED:
            json_object_set_new(container, "state", json_string("truncated"));
            break;
        case FILE_STATE_ERROR:
            json_object_set_new(container, "state", json_string("error"));
            break;
        default:
            json_object_set_new(container, "state", json_string("unknown"));
            break;
    }

    MemBufferReset(buffer);
    snprintf((char *)buffer->buffer, buffer->size, "%"PRIu64"", ff->size);
    json_object_set_new(container, "size", json_string((char *)buffer->buffer));

    MemBufferReset(buffer);
    snprintf((char *)buffer->buffer, buffer->size, ff->flags & FILE_STORED ? "true" : "false");
    json_object_set_new(container, "stored", json_string((char *)buffer->buffer));

    MemBufferFree(buffer);
    json_object_set_new(js, "metadata", container);
}
#endif
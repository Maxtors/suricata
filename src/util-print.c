/* Copyright (C) 2007-2010 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Victor Julien <victor@inliniac.net>
 *
 * Print utility functions
 */

#include "suricata-common.h"
#include "util-error.h"
#include "util-debug.h"

/**
 *  \brief print a buffer as hex on a single line
 *
 *  Prints in the format "00 AA BB"
 *
 *  \param fp FILE pointer to print to
 *  \param buf buffer to print from
 *  \param buflen length of the input buffer
 */
void PrintRawLineHexFp(FILE *fp, uint8_t *buf, uint32_t buflen)
{
#define BUFFER_LENGTH 2048
    char nbuf[BUFFER_LENGTH] = "";
    uint32_t offset = 0;
    uint32_t u = 0;

    for (u = 0; u < buflen; u++) {
        int cw = snprintf(nbuf + offset, BUFFER_LENGTH - offset, "%02X ",
                          buf[u]);
        if (cw >= 0) {
            if ((offset + cw) >= BUFFER_LENGTH) {
                offset = BUFFER_LENGTH - 1;
            } else {
                offset += cw;
            }
        }
    }
    fprintf(fp, "%s", nbuf);
}

/**
 *  \brief print a buffer as hex on a single line in to retbuf buffer
 *
 *  Prints in the format "00 AA BB"
 *
 *  \param retbuf pointer to the buffer which will have the result
 *  \param rebuflen lenght of the buffer
 *  \param buf buffer to print from
 *  \param buflen length of the input buffer
 */
void PrintRawLineHexBuf(char *retbuf, uint32_t retbuflen, uint8_t *buf, uint32_t buflen)
{
    uint32_t offset = 0;
    uint32_t u = 0;

    for (u = 0; u < buflen; u++) {
        int cw = snprintf(retbuf + offset, retbuflen - offset, "%02X ",
                          buf[u]);
        if (cw >= 0) {
            if ((offset + cw) >= retbuflen) {
                offset = retbuflen - 1;
            } else {
                offset += cw;
            }
        }
    }
}

void PrintRawJsonFp(FILE *fp, uint8_t *buf, uint32_t buflen)
{
#define BUFFER_LENGTH 2048
    char nbuf[BUFFER_LENGTH] = "";
    uint32_t offset = 0;
    uint32_t u = 0;

    for (u = 0; u < buflen; u++) {
        if (buf[u] == '\\' || buf[u] == '/' || buf[u] == '\"') {
            int cw = snprintf(nbuf + offset, BUFFER_LENGTH - offset, "\\%c",
                              buf[u]);
            if (cw >= 0) {
                if ((offset + cw) >= BUFFER_LENGTH) {
                    offset = BUFFER_LENGTH - 1;
                } else {
                    offset += cw;
                }
            }
        } else if (isprint(buf[u])) {
            int cw = snprintf(nbuf + offset, BUFFER_LENGTH - offset, "%c",
                              buf[u]);
            if (cw >= 0) {
                if ((offset + cw) >= BUFFER_LENGTH) {
                    offset = BUFFER_LENGTH - 1;
                } else {
                    offset += cw;
                }
            }
        } else {
            int cw = snprintf(nbuf + offset, BUFFER_LENGTH - offset, "\\\\x%02X",
                              buf[u]);
            if (cw >= 0) {
                if ((offset + cw) >= BUFFER_LENGTH) {
                    offset = BUFFER_LENGTH - 1;
                } else {
                    offset += cw;
                }
            }
        }
    }
    fprintf(fp, "%s", nbuf);
}

void PrintRawUriFp(FILE *fp, uint8_t *buf, uint32_t buflen)
{
#define BUFFER_LENGTH 2048
    char nbuf[BUFFER_LENGTH] = "";
    uint32_t offset = 0;
    uint32_t u = 0;

    for (u = 0; u < buflen; u++) {
        if (isprint(buf[u]) && buf[u] != '\"') {
            int cw = snprintf(nbuf + offset, BUFFER_LENGTH - offset, "%c",
                              buf[u]);
            if (cw >= 0) {
                if ((offset + cw) >= BUFFER_LENGTH) {
                    offset = BUFFER_LENGTH - 1;
                } else {
                    offset += cw;
                }
            }
        } else {
            int cw = snprintf(nbuf + offset, BUFFER_LENGTH - offset, "\\x%02X",
                              buf[u]);
            if (cw >= 0) {
                if ((offset + cw) >= BUFFER_LENGTH) {
                    offset = BUFFER_LENGTH - 1;
                } else {
                    offset += cw;
                }
            }
        }
    }

    fprintf(fp, "%s", nbuf);
}

void PrintRawUriBuf(char *retbuf, uint32_t *offset, uint32_t retbuflen,
                    uint8_t *buf, uint32_t buflen)
{
    uint32_t u = 0;

    for (u = 0; u < buflen; u++) {
        if (isprint(buf[u]) && buf[u] != '\"') {
            int cw = snprintf(retbuf + *offset, retbuflen - *offset, "%c",
                              buf[u]);
            if (cw >= 0) {
                if ((*offset + cw) >= retbuflen) {
                    *offset = retbuflen - 1;
                } else {
                    *offset += cw;
                }
            }
        } else {
            int cw = snprintf(retbuf + *offset, retbuflen - *offset, "\\x%02X",
                              buf[u]);
            if (cw >= 0) {
                if ((*offset + cw) >= retbuflen) {
                    *offset = retbuflen - 1;
                } else {
                    *offset += cw;
                }
            }
        }
    }

    return;
}

void PrintRawDataFp(FILE *fp, uint8_t *buf, uint32_t buflen) {
    int ch = 0;
    uint32_t u = 0;

    for (u = 0; u < buflen; u+=16) {
        fprintf(fp ," %04X  ", u);
        for (ch = 0; (u+ch) < buflen && ch < 16; ch++) {
             fprintf(fp, "%02X ", (uint8_t)buf[u+ch]);

             if (ch == 7) fprintf(fp, " ");
        }
        if (ch == 16) fprintf(fp, "  ");
        else if (ch < 8) {
            int spaces = (16 - ch) * 3 + 2 + 1;
            int s = 0;
            for ( ; s < spaces; s++) fprintf(fp, " ");
        } else if(ch < 16) {
            int spaces = (16 - ch) * 3 + 2;
            int s = 0;
            for ( ; s < spaces; s++) fprintf(fp, " ");
        }

        for (ch = 0; (u+ch) < buflen && ch < 16; ch++) {
             fprintf(fp, "%c", isprint((uint8_t)buf[u+ch]) ? (uint8_t)buf[u+ch] : '.');

             if (ch == 7)  fprintf(fp, " ");
             if (ch == 15) fprintf(fp, "\n");
        }
    }
    if (ch != 16)
        fprintf(fp, "\n");
}


#ifndef s6_addr16
# define s6_addr16 __u6_addr.__u6_addr16
#endif

static const char *PrintInetIPv6(const void *src, char *dst, socklen_t size)
{
    struct in6_addr * insrc = (struct in6_addr *) src;
    int i;
    char s_part[6];

    /* current IPv6 format is fixed size */
    if (size < 8 * 5) {
        SCLogWarning(SC_ERR_ARG_LEN_LONG, "Too small buffer to write IPv6 address");
        return NULL;
    }
    memset(dst, 0, size);
    for(i = 0; i < 8; i++) {
        snprintf(s_part, 6, "%04x:", htons(insrc->s6_addr16[i]));
        strlcat(dst, s_part, size);
    }
    /* suppress last ':' */
    dst[strlen(dst) - 1] = 0;

    return dst;
}

const char *PrintInet(int af, const void *src, char *dst, socklen_t size)
{
    switch (af) {
        case AF_INET:
            return inet_ntop(af, src, dst, size);
        case AF_INET6:
            /* Format IPv6 without deleting zeroes */
            return PrintInetIPv6(src, dst, size);
        default:
            SCLogError(SC_ERR_INVALID_VALUE, "Unsupported protocol: %d", af);
    }
    return NULL;
}

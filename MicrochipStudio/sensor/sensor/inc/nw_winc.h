/**
 * MIT License
 *
 * Copyright (c) 2021 Klatu Networks
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _NW_WINC_H_
#define _NW_WINC_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Includes

#include "socket/include/socket.h"


#include "osa.h"

// =============================================================================
// Public Types and Definitions
#define NW_WINC_MAX_SOCKETS  4


typedef enum 
{
    NW_WIFI_TCP_CLIENT,
    NW_WIFI_UDP_CLINET,
    NW_WIFI_SSL_TCP_CLIENT,
    NW_WIFI_TCP_SERVER,
    NW_WIFI_UDP_SERVER, 
    NW_WIFI_SSL_TCP_SERVER   
}NW_WINC_socket_type_t;


typedef struct 
{
    BOOLEAN is_socket_open;

    NW_WINC_socket_type_t  socket_type;

    SOCKET socket_id;

    EventGroupHandle_t  event_group; //freeRTOS Event

    unsigned char hostname[128];
    uint32_t addr;

    unsigned char* recv_buf;
    unsigned int recv_length;
    unsigned int prev_recv_length;
}NW_WINC_socket_t;


typedef struct NW_WINC
{  
    /* Wifi network parameters */
    unsigned char  ssid[32];
    unsigned char  pass_phrase[32];
    unsigned int   wifi_security;
    unsigned char  is_network_up;
    unsigned char  ip_addr[32];
    EventGroupHandle_t  wifi_event_group; //freeRTOS Event

    osa_thread_handle h_wifi_thread; 

    NW_WINC_socket_t  Sockets[NW_WINC_MAX_SOCKETS];
    unsigned char sys_time[40];  

} NW_WINC;

// =============================================================================
// Public Declarations

void NW_WINC_Init(NW_WINC*               pThis,
                      unsigned char*             pSSID,
                      unsigned char*             pPassPhrase,
                      unsigned int               wifi_security);


void NW_WINC_Term();

int NW_WINC_OpenSocket(NW_WINC_socket_type_t type,
                       unsigned char* ip_addr,
                       unsigned char* hostname, 
                       int port,
                       int* socket_id);

int NW_WINC_CloseSocket(int socket_id);

int NW_WINC_SendSocket(int socket_id,unsigned char* buffer, int size);

int NW_WINC_ReceiveSocket(int socket_id, unsigned char* buffer, int size);

void NW_WINC_GetSysTime(unsigned char* time);

void NW_WINC_GetMacAddr(unsigned char* mac_addr);

void NW_WINC_GetIpAddr(unsigned char* ip_addr);

#ifdef __cplusplus
}
#endif

#endif  
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

// =============================================================================
// Includes

#include <stdio.h>
#include <stddef.h>


#include "FreeRTOS.h"
#include "console.h"
#include "hal_rtos.h"
#include "string.h"
#include "task.h"
#include "winc_init.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "common/include/nm_common.h"
#include "driver/include/m2m_types.h"


#include "osa.h"
#include "trace.h"
#include "rtc.h"
#include "nw_winc.h"
// =============================================================================

// Private types and definitions

#ifndef DEF_SSID
#define DEF_SSID "wdbsystems"
#endif

#ifndef DEF_PASSWORD
#define DEF_PASSWORD "WDBNet123"
#endif
#ifndef DEF_SEC_TYPE
#define DEF_SEC_TYPE 2
#endif

static SOCKET s_socket = -1;




// =============================================================================
// Private (forward) declarations
static void test_open_socket();

// =============================================================================
// Private storage

// =============================================================================
// Public code

static void test_wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    printf("%s() Entry = %x\n",__FUNCTION__,u8MsgType);
    switch (u8MsgType)
    {
        case M2M_WIFI_RESP_CON_STATE_CHANGED:
        { 
            tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;           
            if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
                puts("m2m_wifi_state: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\n");
                m2m_wifi_get_connection_info();
                m2m_wifi_request_dhcp_client();
            }
            else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
                puts("m2m_wifi_state: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED");
            // Connect to the router using user supplied credentials
                m2m_wifi_connect(DEF_SSID, strlen(DEF_SSID), DEF_SEC_TYPE, DEF_PASSWORD, M2M_WIFI_CH_ALL);
            }
        }
        break;

        case M2M_WIFI_REQ_DHCP_CONF:
        {
            uint8 *pu8IPAddress = (uint8 *)pvMsg;
            printf("m2m_wifi_state: M2M_WIFI_REQ_DHCP_CONF: IP is %u.%u.%u.%u\n",
                    pu8IPAddress[0],
                    pu8IPAddress[1],
                    pu8IPAddress[2],
                    pu8IPAddress[3]);
        }  
		test_open_socket();
        break;

		default:
			break;
    }
}
static void test_socket_cb(SOCKET sock, uint8 u8Msg, void *pvMsg)
{
    printf("%s() Entry = %x\n", __FUNCTION__,u8Msg);

    switch (u8Msg) 
    {
        /* Socket connected */
        case SOCKET_MSG_CONNECT:
        {
            tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
            if (pstrConnect && pstrConnect->s8Error >= 0) 
            {
                TRACE_DBG("%s Socket Connected \n",__FUNCTION__);
            }
            else
            {
                TRACE_DBG("%s Socket Connected ERROR \n",__FUNCTION__);
            }
        }
            break; 

        /* Message send */
        case SOCKET_MSG_SEND: 
        {
            TRACE_DBG("%s Send ok\n",__FUNCTION__);
        }
            break;

        /* Message receive */
        case SOCKET_MSG_RECV: 
        {
            tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
            if (pstrRecv && pstrRecv->s16BufferSize > 0)
            {
                TRACE_DBG("%s Recv ok\n",__FUNCTION__);
            }
            else
            {
                TRACE_WARN("%s Recv ERROR \n",__FUNCTION__);
            }
        } 
            break;

        default:  
            break;
    }
}
  #if 0
int test_wifi2(void)
{
	unsigned char ret;
    tstrWifiInitParam param;
	static winc_mgr_params_t winc_params = {
      .wifi_ssid = "wdbsystems2",
      .wifi_sectype = M2M_WIFI_SEC_WPA_PSK,
      .wifi_auth = (void *)"WDBNet123",
      .wifi_channels = M2M_WIFI_CH_ALL,
      .host_port = _htons(443),
      .host_name = "api.traxxekg.com",
      .use_ssl = true,
  	};
	winc_mgr_params_t *s_params;


  	TRACE_DBG("%s() Entry\n", __FUNCTION__);
	
	s_params = &winc_params;

	set_winc_spi_descriptor(&SPI_INSTANCE);
    nm_bsp_init();

    m2m_memset((uint8*)&param, 0, sizeof(param));
    param.pfAppWifiCb = test_wifi_cb;

    /*intilize the WINC Driver*/
    ret = m2m_wifi_init(&param);
    if (M2M_SUCCESS != ret)
    {
        M2M_ERR("Driver Init Failed <%d>\n",ret);
        while(1);
    }
	TRACE_DBG("%s() Before Wifi Connect \n", __FUNCTION__);
	m2m_wifi_connect((char *)s_params->wifi_ssid,
                      strlen(s_params->wifi_ssid), // include null byte!
                      s_params->wifi_sectype,
                      s_params->wifi_auth,
                      s_params->wifi_channels);

	TRACE_DBG("%s() After Wifi Connect\n", __FUNCTION__);
    while(1){
        /* Handle the app state machine plus the WINC event handler */
        while(m2m_wifi_handle_events(NULL) != M2M_SUCCESS) { vTaskDelay(1000);
        }
    }
}

#endif
int test_wifi(void)
{
    tstrWifiInitParam  param;
    //sint8              ret;

    TRACE_DBG("%s() Entry\n", __FUNCTION__);

	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));
    param.pfAppWifiCb = test_wifi_cb;
    wifi_init(&param);

    /* Initialize Socket module */
    socketInit();
    registerSocketCallback(test_socket_cb, NULL);

    // Connect to the router using user supplied credentials
    m2m_wifi_connect(DEF_SSID, strlen(DEF_SSID), DEF_SEC_TYPE, DEF_PASSWORD, M2M_WIFI_CH_ALL);
    while(1)    
    {
	    while(m2m_wifi_handle_events(NULL) != M2M_SUCCESS)
        { 
            vTaskDelay(1000);
        }
    }
}

static void test_open_socket()
{

    struct sockaddr_in addr;

    TRACE_DBG("%s() Entry\n", __FUNCTION__);

    /* Initialize socket address structure. */
    addr.sin_family      = AF_INET;
    addr.sin_port        = _htons(5000);
    addr.sin_addr.s_addr = nmi_inet_addr("192.168.1.135");
	//SOCKET_FLAGS_SSL

	s_socket = socket(AF_INET, SOCK_STREAM, 0);

    TRACE_DBG(" Socket Allocated \n");

	if(s_socket >= 0)
	{
        TRACE_DBG("%s() Entry222222\n", __FUNCTION__);
		if (connect(s_socket,(struct sockaddr *)&addr,sizeof(struct sockaddr_in)) < 0)
		{
			TRACE_WARN("Socket Creation Error\n");
		}

	}
    TRACE_DBG("%s() Exit\n", __FUNCTION__);
}

int SENSOR_GetData(unsigned char* buf, int size)
{
    unsigned char* rsp;
    //unsigned int len;
    unsigned char mac_addr[6];
    unsigned char ip_addr[32];
    unsigned char* p; 
  

    TRACE_DBG("%s() Entry\n", __FUNCTION__);

    rsp = buf;
    memset(rsp,0x00,size);
    NW_WINC_GetMacAddr(mac_addr);
    NW_WINC_GetIpAddr( ip_addr);
    rsp +=sprintf((char *)rsp,"POST /api/devices/%.2X:%.2X:%.2X:%.2X:%.2X:%.2X/collect_and_check HTTP/1.1\r\n",
                  mac_addr[0],
                  mac_addr[1],
                  mac_addr[2],
                  mac_addr[3],
                  mac_addr[4],
                  mac_addr[5]);


    rsp +=sprintf((char *)rsp,"Host: api.traxxekg.com\r\n");

    rsp +=sprintf((char *)rsp,"User-Agent: curl/7.58.0\r\n");

    rsp +=sprintf((char *)rsp,"Accept: */*\r\n");

    rsp +=sprintf((char *)rsp,"Content-Type: application/json\r\n");

    rsp +=sprintf((char *)rsp,"Authorization: Basic ");

    rsp +=sprintf((char *)rsp,"TkZCVE02R1Y3QVVFMlNSREtNSjNENllVSUk6S1g0VFhRSlBFSEZDWUVBRDJGR0dTMlFGVUc2R1hHNlFHQzdQQ0ZZ\r\n"); 
 
    rsp +=sprintf((char *)rsp,"Content-Length: %d\r\n",887);

    rsp +=sprintf((char *)rsp,"\r\n");
 
    #if 1

    p = rsp;
    rsp +=sprintf((char *)rsp,"{\"macAddress\":\"%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\"", mac_addr[0],
			                                                           mac_addr[1],
			                                                           mac_addr[2],
			                                                           mac_addr[3],
			                                                           mac_addr[4],
			                                                           mac_addr[5]);
    rsp +=sprintf((char *)rsp,",\"timestamp\":%d",RTC_GetTimeStamp());

    rsp +=sprintf((char *)rsp,",\"attributes\":{\"model\":\"TRAXX Device de Oro\"");
    rsp +=sprintf((char *)rsp,",\"hwId\":2");
    rsp +=sprintf((char *)rsp,",\"version\":\"%s\"","1.4.6.6");
    rsp +=sprintf((char *)rsp,",\"registryVersion\":\"%s\"","02.00.00");

    rsp +=sprintf((char *)rsp,",\"wifi ip\":\"%s\"}",ip_addr);

    rsp +=sprintf((char *)rsp,",\"events\":[]");

    rsp +=sprintf((char *)rsp,",\"samples\":[{");

    rsp +=sprintf((char *)rsp,"\"index\":0");
    rsp +=sprintf((char *)rsp,",\"time\":%d",RTC_GetTimeStamp());

    rsp +=sprintf((char *)rsp,",\"sensors\":[");

    rsp +=sprintf((char *)rsp,"{\"id\":\"RTD-1\"");
    rsp +=sprintf((char *)rsp,",\"metric\":\"TEMPERATURE\"");
    rsp +=sprintf((char *)rsp,",\"value\":17.7088");
    rsp +=sprintf((char *)rsp,",\"valid\":\"true\"}");

    rsp +=sprintf((char *)rsp,",{\"id\":\"CT-1\"");
    rsp +=sprintf((char *)rsp,",\"metric\":\"AMPERAGE\"");
    rsp +=sprintf((char *)rsp,",\"value\":0.0000");
    rsp +=sprintf((char *)rsp,",\"valid\":\"true\"}]}]");



    rsp +=sprintf((char *)rsp,",\"metrics\":[{");
    rsp +=sprintf((char *)rsp,"\"timestamp\":%d",RTC_GetTimeStamp());
    rsp +=sprintf((char *)rsp,",\"metrics\":{");
    rsp +=sprintf((char *)rsp,"\"wifi.error1\":5");

    rsp +=sprintf((char *)rsp,",\"wifi.err1RadioTime\":33622");
    rsp +=sprintf((char *)rsp,",\"wifi.error2\":0");
    rsp +=sprintf((char *)rsp,",\"wifi.err2RadioTime\":0");
    rsp +=sprintf((char *)rsp,",\"wifi.error3\":0");
    rsp +=sprintf((char *)rsp,",\"wifi.err3RadioTime\":0");
    rsp +=sprintf((char *)rsp,",\"wifi.error4\":0");
    rsp +=sprintf((char *)rsp,",\"wifi.err4RadioTime\":0");

    rsp +=sprintf((char *)rsp,",\"wifi.bssid\":\"%s\"","78:24:af:7c:c6:d0"); //TBD
    rsp +=sprintf((char *)rsp,",\"sn.bssid\":\"%s\"","78:24:af:7c:c6:d0");  //TBD

    rsp +=sprintf((char *)rsp,",\"wifi_channel\":11");                //TBD
    rsp +=sprintf((char *)rsp,",\"sn.vbat\":7.065");  //TBD
    rsp +=sprintf((char *)rsp,",\"wifi_channel\":11");                //TBD
    rsp +=sprintf((char *)rsp,",\"sn.vbat\":7.065");                 //TBD

    rsp +=sprintf((char *)rsp,",\"wifi.rssi\":-51");                //TBD
    rsp +=sprintf((char *)rsp,",\"sn.rssi\":51");                 //TBD

    rsp +=sprintf((char *)rsp,",\"wifi.dhcp\":0");                //TBD
    rsp +=sprintf((char *)rsp,",\"wifi.setup\":0");                 //TBD


    rsp +=sprintf((char *)rsp,",\"wifi.radio\":0");                //TBD
    rsp +=sprintf((char *)rsp,",\"wifi.remain\":0");                 //TBD

    rsp +=sprintf((char *)rsp,",\"cloud.latency\":0");                //TBD
    rsp +=sprintf((char *)rsp,",\"sensor.alarm\":0");                 //TBD


    rsp +=sprintf((char *)rsp,",\"power.source\":1");                //TBD
    rsp +=sprintf((char *)rsp,",\"highpower.count\":0}}]}");                 //TBD
#endif

    TRACE_DBG("%s() Pkt Length = %d  content length = %d  \n", __FUNCTION__, rsp-buf, rsp-p);

    return rsp-buf;
}


// =============================================================================
// Private (static) code

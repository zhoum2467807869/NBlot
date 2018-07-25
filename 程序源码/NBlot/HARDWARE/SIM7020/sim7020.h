#ifndef _SIM7020_H
#define _SIM7020_H
#include "sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nblot_usart.h"
//////////////////////////////////////////////////////////////////////////////////    
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F429开发板
//串口1初始化         
//正点原子@ALIENTEK
//技术论坛:www.openedv.csom
//修改日期:2015/6/23
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved
//********************************************************************************
//V1.0 第一次应用 
//////////////////////////////////////////////////////////////////////////////////  

//初始化及网络注册相关命令
#define AT_SYNC        "AT"
#define AT_CMEE        "AT+CMEE"
#define AT_ATI         "ATI"
#define AT_CPIN        "AT+CPIN"
#define AT_CSQ         "AT+CSQ"
#define AT_CFUN        "AT+CFUN"
#define AT_CGREG       "AT+CGREG"
#define AT_CGACT       "AT+CGACT"
#define AT_CGATT       "AT+CGATT"
#define AT_COPS        "AT+COPS"
#define AT_CGCONTRDP   "AT+CGCONTRDP"


//信息查询相关命令
#define AT_CGMI        "AT+CGMI"        //获取产商ID命令
#define AT_CGMM        "AT+CGMM"        //获取模型ID命令
#define AT_CGMR        "AT+CGMR"        //获取软件版本命令 
#define AT_CIMI        "AT+CIMI"        //获取国际移动用户身份ID命令 
#define AT_CGSN        "AT+CGSN"        //获取产品序列号ID命令
#define AT_CBAND       "AT+CBAND"       //获取频率信息

//TCP/UDP相关命令
#define AT_CSOC        "AT+CSOC"
#define AT_CSOCON      "AT+CSOCON" 
#define AT_CSOSEND     "AT+CSOSEND"
#define AT_CSOCL       "AT+CSOCL" 


//CoAP相关命令                        
#define AT_CCOAPSTA    "AT+CCOAPSTA"   //连接远端 CoAP 服务
#define AT_CCOAPNEW    "AT+CCOAPNEW"   //创建客户端实例
#define AT_CCOAPSEND   "AT+CCOAPSEND"  //发送16进制数据
#define AT_CCOAPDEL     "AT+CCOAPDEL"  //释放并删除客户端实例


#define AT_CLAC        "AT+CLAC"
#define AT_CGPADDR     "AT+CGPADDR"
#define AT_CGDCONT     "AT+CGDCONT"
#define AT_CCLK        "AT+CCLK"
#define AT_CPSMS       "AT+CPSMS"
#define AT_CEDRXS      "AT+CEDRXS"
#define AT_CEER        "AT+CEER"
#define AT_CEDRXRDP    "AT+CEDRXRDP"
#define AT_CTZR        "AT+CTZR"


/* ETSI Commands* (127.005)  <Under development> */
/*
#define AT_CSMS       "AT+CSMS"
#define AT_CNMA       "AT+CNMA"
#define AT_CSCA       "AT+CSCA"
#define AT_CMGS       "AT+CMGS"
#define AT_CMGC       "AT+CMGC"
#define AT_CSODCP     "AT+CSODCP"
#define AT_CRTDCP     "AT+CRTDCP"
*/

#define AT_NMGS       "AT+NMGS"
#define AT_NMGR       "AT+NMGR"
#define AT_NNMI       "AT+NNMI"
#define AT_NSMI       "AT+NSMI"
#define AT_NQMGR      "AT+NQMGR"
#define AT_NQMGS      "AT+NQMGS"
#define AT_NMSTATUS   "AT+NMSTATUS"
#define AT_NRB        "AT+NRB"
#define AT_NCDP       "AT+NCDP"

#define AT_NUESTATS   "AT+NUESTATS"

#define AT_NEARFCN    "AT+NEARFCN"
#define AT_NSOCR      "AT+NSOCR"
#define AT_NSOST      "AT+NSOST"
#define AT_NSOSTF     "AT+NSOSTF"
#define AT_NSORF      "AT+NSORF"
#define AT_NSOCL      "AT+NSOCL"  
      
#define  AT_NPING          "AT+NPING"
#define  AT_NLOGLEVEL      "AT+NLOGLEVEL"
#define  AT_NCONFIG        "AT+NCONFIG"
#define  AT_NATSPEED       "AT+NATSPEED"
#define  AT_NCCID          "AT+NCCID"
#define  AT_NFWUPD         "AT+NFWUPD"
#define  AT_NRDCTRL        "AT+NRDCTRL"
#define  AT_NCHIPINFO      "AT+NCHIPINFO"
#define  AT_NTSETID        "AT+NTSETID"


#define CMD_TRY_TIMES           10
#define CMD_READ_ARGUMENT       "?"
#define CMD_TEST_ARGUMENT       "=?"

#define CMD_OK_RES              "OK"

//#define CTNB                    1

#ifdef CTNB
#define REMOTE_SERVER_IP        "\"180.101.147.115\""
#define REMOTE_COAP_PORT        "5683"
#define REMOTE_UDP_PORT         "6000"
#define REMOTE_TCP_PORT         "5683"

#define REMOTE_COAP_INFO        "\"180.101.147.115,5683\""

#else

#define REMOTE_SERVER_IP        "\"115.29.240.46\""
#define REMOTE_COAP_PORT        "5683"
#define REMOTE_UDP_PORT         "6000"
#define REMOTE_TCP_PORT         "9000"

#define REMOTE_COAP_INFO        "\"115.29.240.46,5683\""

#endif

#define BAND_850MHZ_ID           5
#define BAND_850MHZ_STR          "850"

#define BAND_900MHZ_ID           8
#define BAND_900MHZ_STR          "900"

#define BAND_800MHZ_ID           20
#define BAND_800MHZ_STR          "800"

#define BAND_700MHZ_ID           28
#define BAND_700MHZ_STR          "700"


//AT指令响应的最大参数个数
#define AT_CMD_RESPONSE_PAR_NUM_MAX   16


/*
 * AT指令属性枚举
 */
typedef enum
{
   CMD_TEST,         //命令TEST操作
   CMD_READ,         //命令READ操作
   CMD_SET,          //命令SET操作
   CMD_EXCUTE        //命令EXCUTE操作
}cmd_property_t;

/*
 * AT指令动作行为枚举
 */
typedef enum
{
    ACTION_OK_EXIT        = 0X01,              //命令执行成功后将退出 
    ACTION_OK_AND_NEXT    = 0X02,              //命令执行成功后将执行下一条指令      
    ACTION_ERROR_BUT_NEXT = 0X04,              //命令执行错误后跳过该命令执行下一条指令
    ACTION_ERROR_AND_TRY  = 0X08,              //命令执行错误后进行尝试
    ACTION_ERROR_EXIT     = 0X10               //命令执行错误后将退出    
}cmd_action_t;

//AT指令结构类型
typedef struct at_cmd_info
{
    const char*     p_atcmd;       // AT指令
    cmd_property_t  property;      // 指令当前属性(TEST,READ,SET,EXCUTE)
    char*           p_atcmd_arg;   // 指令参数
    char*           p_expectres;   // 期望得到回复
    unsigned char   cmd_try;       // 出错尝试次数
    unsigned char   have_tried;    // 已经出次尝试的次数
    uint8_t         cmd_action;    // AT指令行为
    uint32_t        max_timeout;   // 最大超时时间
}at_cmd_info_t;


//声明AT cmd结构指针类型
typedef at_cmd_info_t *at_cmdhandle;


//通用错误代码定义 
#define   SIM7020_OK                  0
#define   SIM7020_ERROR              -1
#define   SIM7020_NOTSUPPORT         -2

//指令执行结果
#define   AT_CMD_RESULT_OK            0
#define   AT_CMD_RESULT_ERROR         1
#define   AT_CMD_RESULT_CONTINUE     -5
#define   AT_CMD_RESULT_RANDOM_CODE  -6

//指令出错处理
#define   SIM7020_ERROR_TIMEOUT      -2
#define   SIM7020_ERROR_RETRY        -3
#define   SIM7020_ERROR_NEXT         -4
#define   SIM7020_ERROR_CONTINUE     -5



//sim7020主状态定义，app可以使用该信息
typedef enum sim7020_main_status
{
    SIM7020_NONE,
    SIM7020_NBLOT_INIT,   // 初始化操作
    SIM7020_NBLOT_INFO,   // 获取 NB 模块厂商及固件，频段等信息
    SIM7020_SIGNAL,       // 获取信号质量
    SIM7020_TCPUDP_CR,    // 创建 TCP/UDP
    SIM7020_TCPUDP_CL,    // 关闭 TCP/UDP
    SIM7020_TCPUDP_SEND,  // 利用已经创建的TCP/UDP发送数据
    SIM7020_TCPUDP_RECV,  // TCP/UDP接收信息 
    SIM7020_SOCKET_ERR,   // SOCKET连接发生错误
    SIM7020_CoAP_SEVER,   // CoAP远程服务器设置
    SIM7020_CoAP_CLIENT,  // CoAP客户端连接地址设置
    SIM7020_CoAP_SEND,    // 发送消息
    SIM7020_CoAP_RECV,    // CoAP返回信息
    SIM7020_CoAP_CL,      // 关闭CoAP
    SIM7020_RESET,        // 复位NB
    SIM7020_END    
}sim7020_main_status_t;


//sim7020子状态定义
typedef enum sim7020_sub_status
{
    SIM7020_SUB_NONE,
    SIM7020_SUB_SYNC,
    SIM7020_SUB_CMEE,    
    SIM7020_SUB_ATI,
    SIM7020_SUB_CPIN,
    SIM7020_SUB_CSQ,
    SIM7020_SUB_CFUN,
    SIM7020_SUB_CEREG,
//    SIM7020_SUB_CGACT_DISABLE,
//    SIM7020_SUB_CGACT,
    SIM7020_SUB_CGACT_QUERY,    
    SIM7020_SUB_CGATT,    
    SIM7020_SUB_CGATT_QUERY,
    SIM7020_SUB_COPS_QUERY,
    SIM7020_SUB_CGCONTRDP_QUERY,
  
    SIM7020_SUB_CEREG_QUERY,
  
    SIM7020_SUB_CGMI,
    SIM7020_SUB_CGMM,
    SIM7020_SUB_CGMR,
    SIM7020_SUB_CIMI,
    SIM7020_SUB_CGSN,  
    SIM7020_SUB_CBAND,
    SIM7020_SUB_NSMI,
    SIM7020_SUB_NNMI,

    SIM7020_SUB_TCPUDP_CR,
    SIM7020_SUB_TCPUDP_CONNECT, 
    SIM7020_SUB_TCPUDP_SEND,
    SIM7020_SUB_TCPUDP_RECV,  // TCP/UDP接收信息 
    SIM7020_SUB_SOCKET_ERR,   // SOCKET连接发生错误    
    SIM7020_SUB_TCPUDP_CL,
    
    SIM7020_SUB_CoAP_SEVER,   // CoAP远程服务器设置
    SIM7020_SUB_CoAP_CLIENT,  // CoAP客户端连接地址设置
    SIM7020_SUB_CoAP_SEND,    // 发送消息
    SIM7020_SUB_CoAP_RECV,    // CoAP返回信息
    SIM7020_SUB_CoAP_CL,      // 关闭CoAP    
    
    SIM7020_SUB_END   
    
}sim7020_sub_status_t;



//sim7020 状态信息,用于状态机
typedef struct sim7020_status_sm
{
    sim7020_main_status_t  main_status;         //命令运行主阶段
    int                    sub_status;          //命令运行子阶段，用于状态嵌套     
}sim7020_status_sm_t;

//sim7020 连接状态信息,用于指示使用哪种协议进行连接
typedef struct sim7020_status_connect {
    uint8_t                connect_status;      //连接的状态
    uint8_t                connect_type;        //连接的类型
    int8_t                 connect_id;          //连接的id
    int8_t                 rssi;                //信号的质量
    int8_t                 cid;                 //连接的cid
    uint8_t                register_status;     //网络注册
    uint16_t               data_len;            //提示数据长度  
    char                  *data_offest;         //数据缓冲区起始地址所在偏移地址  
}sim7020_status_connect_t;

typedef struct sim7020_socket_info {
    uint8_t                socket_type;         //指示socket_type的类型
    int8_t                 socket_id;           //指示相应的socket id
    int8_t                 socket_errcode;      //指示相应的socket错误码 
    char                  *data_offest;         //数据缓冲区起始地址所在偏移地址
    uint16_t               data_len;            //提示数据长度   
}sim7020_socket_info_t;

//连接的协议类型
typedef enum sim7020_connect_type {
   SIM7020_NOCON,
   SIM7020_TCP  = 1,         
   SIM7020_UDP,         
   SIM7020_HTTP,        
   SIM7020_COAP,        
   SIM7020_MQTT,            
}sim7020_connect_type_t;

//SIM7020G固件信息
typedef struct sim020_firmware_info
{   char         name[32];
    uint8_t      IMSI[16];
    uint8_t      IMEI[16];      
}sim020_firmware_info_t;


//定义收发数据缓冲区长度
#define SIM7020_RECV_BUF_MAX_LEN    (RING_BUF_LEN + 1)
#define SIM7020_SEND_BUF_MAX_LEN    (RING_BUF_LEN + 1)

//接收缓存空间
typedef struct sim7020_recv
{
    char      buf[SIM7020_RECV_BUF_MAX_LEN];    //接收数据缓冲区
    uint16_t  len;                              //有效数据长度
}sim7020_recv_t;

//接收缓存空间
typedef struct sim7020_send
{
    char      buf[SIM7020_SEND_BUF_MAX_LEN];    //发送数据缓冲区
    uint16_t  len;                              //有效数据长度
    
}sim7020_send_t;

//sim7020驱动函数结构体
struct sim7020_drv_funcs {
    
    //sim7020发送数据
    int (*sim7020_send_data) (void *p_arg, uint8_t *pData, uint16_t size, uint32_t Timeout);
    
    //sim7020接收数据
    int (*sim7020_recv_data) (void *p_arg, uint8_t *pData, uint16_t size, uint32_t Timeout);    
};

#define    SIM7020_NONE_EVENT          0x0000           //没有事件发生
#define    SIM7020_RECV_EVENT          0x0001           //收到回应数据事件
#define    SIM7020_TIMEOUT_EVENT       0x0002           //命令发送时出错产生超时事件或发送后得不到回应，产生超时事件
#define    SIM7020_REG_STA_EVENT       0x0004           //NBIOT网络附着事件
#define    SIM7020_TCP_RECV_EVENT      0x0008           //TCP接收事件
#define    SIM7020_UDP_RECV_EVENT      0x0010           //UDP接收事件
#define    SIM7020_COAP_RECV_EVENT     0X0020           //COAP接收事件
#define    SIM7020_MQTT_RECV_EVENT     0X0040           //MQTT接收事件
#define    SIM7020_LWM2M_RECV_EVENT    0X0080           //LWM2M接收事件
#define    SIM7020_SOCKET_ERR_EVENT    0x0100           //SOCKET发生错误事件

//sim7020消息id, 回调函数中使用
typedef enum sim7020_msg_id
{
    SIM7020_MSG_NONE,

    SIM7020_MSG_NBLOT_INIT,

    SIM7020_MSG_NBLOT_INFO,
    
    SIM7020_MSG_REG,
  
    SIM7020_MSG_IMEI,       //移动设备身份码   
    SIM7020_MSG_IMSI,
  
    SIM7020_MSG_MID,        //产商ID
    SIM7020_MSG_MMODEL,     //模块型号
    SIM7020_MSG_MREV,       //软件版本号
    SIM7020_MSG_BAND,       //工作频段
  
    SIM7020_MSG_CSQ,        
    SIM7020_MSG_SIGNAL,     //信号强度
    

    SIM7020_MSG_TCPUDP_CREATE,
    SIM7020_MSG_TCPUDP_CLOSE,
    SIM7020_MSG_TCPUDP_SEND,
    SIM7020_MSG_TCPUDP_RECV,
    
    SIM7020_MSG_SOCKET_ERROR, //socket错误    

    SIM7020_MSG_COAP_SERVER,
    SIM7020_MSG_COAP_CLIENT,
    SIM7020_MSG_COAP_SEND,
    SIM7020_MSG_COAP_RECV,
    SIM7020_MSG_COAP_CLOSE,
    
    SIM7020_MSG_CMD_RETRY,
    
    SIM7020_MSG_CMD_NEXT,
    
    SIM7020_MSG_CMD_FAIL,    

    SIM7020_MSG_END
  
}sim7020_msg_id_t;

//定义串口事件回调函数指针
typedef void (*sim7020_cb)(void *p_arg, sim7020_msg_id_t, int ,char*);


//sim7020设备结构体
typedef struct sim7020_dev
{     
    struct sim7020_drv_funcs *p_drv_funcs;
    
    uart_dev_t               *p_uart_dev;
    
    //sim7020设备事件回调函数
    sim7020_cb                sim7020_cb;  
    
    void                     *p_arg;

    //事件标记
    volatile int              sim7020_event;
    
    //sim7020指令执行状态信息
    at_cmd_info_t            *p_sim7020_cmd; 

    //sim7020指令执行状态信息
    sim7020_socket_info_t    *p_socket_info;    

    //sim7020 sm状态信息
    sim7020_status_sm_t      *sim7020_sm_status;

    //sim7020 连接状态信息
    sim7020_status_connect_t *sim7020_connect_status;
  
    //sim7020固件信息
    sim020_firmware_info_t   *firmware_info; 

    //帧格式，超时成帧或者ILDE成帧
    uint8_t                   frame_format;  
               
}sim7020_dev_t;

//sim7020设备句柄
typedef sim7020_dev_t *sim7020_handle_t;

//设置sim7020事件
void sim7020_event_set (sim7020_handle_t sim7020_handle, int sim7020_event);

//获取sim7020事件
int sim7020_event_get (sim7020_handle_t sim7020_handle, int sim7020_event);

//清除sim7020事件
void sim7020_event_clr (sim7020_handle_t sim7020_handle, int sim7020_event);

//sim7020初始化 
sim7020_handle_t sim7020_init(uart_handle_t lpuart_handle);

//注册sim7020事件回调处理函数
void sim7020_event_registercb (sim7020_handle_t sim7020_handle, sim7020_cb cb, void *p_arg);

//sim7020事件处理函数
int sim7020_event_poll (sim7020_handle_t sim7020_handle);

//sim7020 nblot初始化及完成网络注册
int sim7020_nblot_init (sim7020_handle_t sim7020_handle);

//获取NB模块的信息
int sim7020_nblot_info_get(sim7020_handle_t sim7020_handle);

//获取NB模块的信号质量
int sim7020_nblot_signal_get(sim7020_handle_t sim7020_handle);


//创建tcpudp 
int sim7020_nblot_tcpudp_create(sim7020_handle_t sim7020_handle, sim7020_connect_type_t type);


//关闭tcpudp
int sim7020_nblot_tcpudp_close(sim7020_handle_t sim7020_handle, sim7020_connect_type_t type);


//以hex数据格式发送数据
int sim7020_nblot_tcpudp_send_hex(sim7020_handle_t sim7020_handle, 
                                  int len, 
                                  char *msg, 
                                  sim7020_connect_type_t type);

//以字符串格式发送数据
int sim7020_nblot_tcpudp_send_str(sim7020_handle_t sim7020_handle, 
                                  int len, 
                                  char *msg, 
                                  sim7020_connect_type_t type);
                                  
//创建coap服务器 
int sim7020_nblot_coap_server_create(sim7020_handle_t sim7020_handle, sim7020_connect_type_t type);

//创建coap客户端
int sim7020_nblot_coap_client_create(sim7020_handle_t sim7020_handle, sim7020_connect_type_t type);


//关闭coap
int sim7020_nblot_coap_close(sim7020_handle_t sim7020_handle, sim7020_connect_type_t type);


//以hex数据格式发送coap协议数据,必须是偶数个长度
int sim7020_nblot_coap_send_hex(sim7020_handle_t sim7020_handle, int len, char *msg, sim7020_connect_type_t type);

//以字符串格式发送数据coap协议数据
int sim7020_nblot_coap_send_str(sim7020_handle_t sim7020_handle, int len, char *msg, sim7020_connect_type_t type);                                                                  

#endif

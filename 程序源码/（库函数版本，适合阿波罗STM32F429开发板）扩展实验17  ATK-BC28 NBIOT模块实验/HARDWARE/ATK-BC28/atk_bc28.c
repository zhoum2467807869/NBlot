#include "atk_bc28.h"
#include "atk_delay.h"
#include "atk_bc28_nbiot.h"

////////////////////////////////////////////////////////////////////////////////// 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F429开发板
//串口1初始化           
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2015/9/7
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************


/**
  * @brief 调试宏开关
  */
#define NBIOT_DEBUG                   
#ifdef NBIOT_DEBUG
#define NBIOT_DEBUG_INFO(...)    (int)printf(__VA_ARGS__)
#else
#define NBIOT_DEBUG_INFO(...)
#endif


//简要逻辑
//对命令的响应后的解析会调用at_cmd_result_parse这个函数
//如果正确，会根据命令的属性来决定是否需要执行下一条命令
//发送消息时，调用nbiot_msg_send，子状态的命令此时并没有发生改变
//直到调用at_cmd_next构造好下一条指令的时候才会改变子状态
//当at_cmd_next返回FALSE时，表明所有的子状态都已经结束了，此时会复用状态机的逻辑


/**
  * @brief 定义nbiot数据收发描述结构体变量
  */
static struct nbiot_recv  g_nbiot_recv_desc;
static struct nbiot_send  g_nbiot_send_desc;

/**
  * @brief 定义用于保存当前正在执行的AT指令的述结构变量
  */
static at_cmd_info_t g_at_cmd; 

/**
  * @brief 定义nbiot设备结构体变量
  */
static struct nbiot_dev       g_nbiot_dev;

/**
  * @brief 定义nbiot指令状态流程信息结构体变量
  */
static nbiot_status_sm_t      g_nbiot_sm_status;

/**
  * @brief 连接状态信息结构体变量
  */
static nbiot_status_connect_t  g_nbiot_connect_status;     

/**
  * @brief 定义nbiot固件信息结构体变量
  */
static nbiot_firmware_info_t   g_firmware_info; 


/**
  * @brief 定义socket信息结构体变量
  */
static nbiot_socket_info_t    g_socket_info[5];


/**
  * @brief 相关函数提前声明
  */
static int  __nbiot_uart_data_tx (void *p_arg, uint8_t *pData, uint16_t size, uint32_t Timeout);
static int  __nbiot_uart_data_rx (void *p_arg, uint8_t *pData, uint16_t size, uint32_t Timeout);
static void __uart_event_cb_handle(void *p_arg);
static int8_t at_cmd_result_parse(char *buf);
static uint8_t nbiot_response_handle (nbiot_handle_t nbiot_handle, uint8_t cmd_response);
static uint8_t at_cmd_next (void);   
static uint8_t nbiot_event_notify (nbiot_handle_t nbiot_handle, char *buf);
static void nbiot_msg_send (nbiot_handle_t nbiot_handle, char**buf, int8_t is_ok);
static int nbiot_data_recv(nbiot_handle_t nbiot_handle, uint8_t *pData, uint16_t size, uint32_t Timeout);


/**
  * @brief 定义NBIoT模块驱动函数结体变量 
  */
static struct nbiot_drv_funcs drv_funcs = {    
    __nbiot_uart_data_tx,
    __nbiot_uart_data_rx,        
};

/**
  * @brief 清空接收缓存  
  * @param  None 
  * @retval None  
  */
static void nbiot_recv_buf_reset(void)
{   
    memset(&g_nbiot_recv_desc, 0, sizeof(struct nbiot_recv));
}


/**
  * @brief 复位AT指令执行的流程  
  * @param  None 
  * @retval None  
  */
static void nbiot_status_reset(void)
{
    g_nbiot_sm_status.main_status  = NBIOT_NONE;
    g_nbiot_sm_status.sub_status   = NBIOT_SUB_NONE;
}

/**
  * @brief 设置AT指令执行的流程  
  * @param  None 
  * @retval None  
  */
static void nbiot_status_set (nbiot_main_status_t  main_status, nbiot_sub_status_t  sub_status)
{
    g_nbiot_sm_status.main_status  = main_status;
    g_nbiot_sm_status.sub_status   = sub_status;
}

/**
  * @brief 获取AT指令执行的流程  
  * @param  None 
  * @retval None  
  */
static void nbiot_status_get (nbiot_status_sm_t *p_sm_status)
{
    p_sm_status->main_status  = g_nbiot_sm_status.main_status ;
    p_sm_status->sub_status   = g_nbiot_sm_status.sub_status;
}

/**
  * @brief 设置nbiot事件  
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  nbiot_event   : 事件类型.
  * @retval None  
  */
void nbiot_event_set (nbiot_handle_t nbiot_handle, int nbiot_event)
{ 
    nbiot_handle->nbiot_event |= nbiot_event;   
}

/**
  * @brief  判断当前nbiot事件是否发生  
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  nbiot_event   : 事件类型.
  * @retval 非0代表当前事件已经发生  
  */
int nbiot_event_get (nbiot_handle_t nbiot_handle,  int nbiot_event)
{ 
    return (nbiot_handle->nbiot_event & nbiot_event); 
}

/**
  * @brief 清除nbiot事件  
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  nbiot_event   : 事件类型.
  * @retval None  
  */
void nbiot_event_clr (nbiot_handle_t nbiot_handle, int nbiot_event)
{ 
    nbiot_handle->nbiot_event &= ~nbiot_event;
}


/**
  * @brief 串口事件回调处理函数
  * @param  p_arg  : 空类型指针.
  * @retval None  
  */
static void __uart_event_cb_handle (void *p_arg)
{    
    nbiot_handle_t  nbiot_handle = (nbiot_handle_t)p_arg; 
    
    uart_dev_t       *p_uart_dev     = nbiot_handle->p_uart_dev;
  
    int size = g_nbiot_recv_desc.len;
    
    if (p_uart_dev->uart_event & UART_TX_EVENT)
    {
               
        NBIOT_DEBUG_INFO("atk_nbiot uart tx ok %s", g_nbiot_send_desc.buf);  

        uart_event_clr(p_uart_dev, UART_TX_EVENT);                 
    }

    if (p_uart_dev->uart_event & UART_RX_EVENT)
    {               
        size = uart_ring_buf_avail_len(p_uart_dev);
        
        //从缓冲区当中读取数据
        if (size > 0)
        {                                  
            nbiot_data_recv(nbiot_handle, (uint8_t*)(&g_nbiot_recv_desc.buf[g_nbiot_recv_desc.len]), size, 0);
                     
            //产生异步事件等待处理
            nbiot_event_notify(nbiot_handle, g_nbiot_recv_desc.buf);
                        
            NBIOT_DEBUG_INFO("atk_nbiot uart rx ok %s\r\n", g_nbiot_recv_desc.buf);
          
            //改变接收数据的缓冲区位置，确保一个命令回响的数据能完全收到
            g_nbiot_recv_desc.len = g_nbiot_recv_desc.len + size;
            
        }
                                                                                 
        uart_event_clr(p_uart_dev, UART_RX_EVENT); 
    } 

    //发生发送超时事件，说明指令有可能没有发送出去，或者发送过程中出错
    //或者模块工作异常，没有回应命令的数据
    if (p_uart_dev->uart_event & UART_TX_TIMEOUT_EVENT) 
    {        
        NBIOT_DEBUG_INFO("atk_nbiot uart tx timeout %s", g_nbiot_send_desc.buf);  

        nbiot_event_set(nbiot_handle, NBIOT_TIMEOUT_EVENT);
      
        uart_event_clr(p_uart_dev, UART_TX_TIMEOUT_EVENT); 
    } 

    //如果使用非超时成帧，此事件理论上不会发生
    if (p_uart_dev->uart_event & UART_RX_TIMEOUT_EVENT) 
    {       
        size = uart_ring_buf_avail_len(p_uart_dev);
        
        //超时成帧      
        if (g_nbiot_dev.frame_format == 1) 
        {
            if (size > 0)
            {               
                nbiot_data_recv(nbiot_handle, (uint8_t*)(&g_nbiot_recv_desc.buf[g_nbiot_recv_desc.len]), size, 0);
                                             
                //产生异步事件等待处理
                nbiot_event_notify(nbiot_handle, g_nbiot_recv_desc.buf);
              
                g_nbiot_recv_desc.len = g_nbiot_recv_desc.len + size;
            }
                                        
       //不在超时成帧的状态下，代表的确发生了超时事件      
       } else {
         
            if (size > 0)
            {               
                nbiot_data_recv(nbiot_handle, (uint8_t*)(&g_nbiot_recv_desc.buf[g_nbiot_recv_desc.len]), size, 0);
                                             
                //产生异步事件等待处理
                nbiot_event_notify(nbiot_handle, g_nbiot_recv_desc.buf);
              
                //改变接收数据的缓冲区位置，确保一个命令回响的数据能完全收到
                g_nbiot_recv_desc.len = g_nbiot_recv_desc.len + size;
            }          
         
            //产生超时事件，超时事件是否执行得看命令解析结果
            nbiot_event_set(nbiot_handle, NBIOT_TIMEOUT_EVENT);
       }
       
       NBIOT_DEBUG_INFO("atk_nbiot uart rx timeout %s\r\n", g_nbiot_recv_desc.buf);  
       
       uart_event_clr(p_uart_dev, UART_RX_TIMEOUT_EVENT);
    }            
}

/**
  * @brief nbiot事件处理函数
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @retval NBIOT_OK 成功  
  */
int nbiot_event_poll(nbiot_handle_t nbiot_handle)
{
    char *at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX] = {0};
    
    char *p_revc_buf_tmp = g_nbiot_recv_desc.buf;
        
    uint8_t index = 0;
        
    int8_t next_cmd = 0;
    
    int8_t cmd_is_pass = 0;
    
    static int8_t recv_cnt = 0;
            
    if (nbiot_event_get(nbiot_handle, NBIOT_RECV_EVENT)) 
    {       
        //接收在超时时间内正常完成，停止接收超时  
        atk_soft_timer_stop(&nbiot_handle->p_uart_dev->uart_rx_timer); 
        //解析到错误，在错误即可进行命令的外理，避免进入超时事件
        nbiot_event_clr(nbiot_handle, NBIOT_TIMEOUT_EVENT); 
        
        NBIOT_DEBUG_INFO("%s recv event\r\n", g_at_cmd.p_atcmd);
      
        cmd_is_pass = at_cmd_result_parse(g_nbiot_recv_desc.buf);
        
        //命令响应结果     
        if (cmd_is_pass == AT_CMD_RESULT_OK) 
        {
            NBIOT_DEBUG_INFO("RESULT OK\r\n");            
                             
            recv_cnt=0; 
                              
            //提取AT指令返回的参数,在使用strok期间，不允许改变缓冲区的内容，中间出现再多的\r\n，也只会当做一个来处理
            while((at_response_par[index] = strtok(p_revc_buf_tmp,"\r\n")) != NULL)
            {
                index++;
                p_revc_buf_tmp = NULL;
                
                if (index >= (AT_CMD_RESPONSE_PAR_NUM_MAX - 1))
                {
                    break;
                }  
            }

            if (index != 0)
            {               
                NBIOT_DEBUG_INFO("%s cmd excute ok\r\n\r\n\r\n", g_at_cmd.p_atcmd);
              
                //代表命令发送成功了并得到正确的响应
                nbiot_msg_send(nbiot_handle, at_response_par, TRUE);
                next_cmd = nbiot_response_handle(nbiot_handle, TRUE);                
                                                                    
            } 
            else 
            {
                
                //未收到符合\r\n...\r\n正确的数据帧，根据结果来重发命令或者数据
                next_cmd = nbiot_response_handle(nbiot_handle, FALSE); 
            }                
                                                                                      
            //清缓存            
            nbiot_recv_buf_reset();                        
       
        }       
        else if(cmd_is_pass == AT_CMD_RESULT_ERROR)
        { 
            NBIOT_DEBUG_INFO("RESULT ERROR\r\n");           

            recv_cnt=0;
          
            //错误后根据命令属性尝试重发命令或者跳过该命令执行下一条命令            
            next_cmd = nbiot_response_handle(nbiot_handle, FALSE);     
        
            if (g_at_cmd.cmd_action & ACTION_ERROR_AND_TRY)
            {                                   
                NBIOT_DEBUG_INFO("%s cmd is failed and try\r\n", g_at_cmd.p_atcmd);
                
                at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1] = (char*)g_at_cmd.p_atcmd;
                
                //通知上层应用，此动作执行失败,但会重试
                nbiot_msg_send(nbiot_handle, &at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1], NBIOT_ERROR_RETRY);
                
            } 
            else if (g_at_cmd.cmd_action & ACTION_ERROR_BUT_NEXT)
            {               
                at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1] = (char*)g_at_cmd.p_atcmd;
               
                NBIOT_DEBUG_INFO("%s cmd is failed and next\r\n", g_at_cmd.p_atcmd);
                
                //通知上层应用，此动作执行失败后跳过该命令执行
                nbiot_msg_send(nbiot_handle, &at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1], NBIOT_ERROR_NEXT);
                
            }
            else 
            {                
                NBIOT_DEBUG_INFO("%s cmd is failed and exit\r\n", g_at_cmd.p_atcmd);        
                at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1] = (char*)g_at_cmd.p_atcmd;
                
                //通知上层应用，此动作执行失败后跳过该命令执行
                nbiot_msg_send(nbiot_handle, &at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1], FALSE);  

                //复位状态标志
                nbiot_status_reset();              

            } 
                    
            //清缓存            
            nbiot_recv_buf_reset();            

        } 

        else if (cmd_is_pass == AT_CMD_RESULT_CONTINUE)
        {
          
            NBIOT_DEBUG_INFO("RESULT CONTINUE\r\n");
            recv_cnt=0;
            
            //命令未执行完成，正常情况下，收到的的是命令回显, 接下来的还是当前命令响应数据的接收, 重新启动接收超时               
            atk_soft_timer_timeout_change(&nbiot_handle->p_uart_dev->uart_rx_timer, 8000);
                             
             
            //命令未完成            
        }       
        else 
        {
         
            recv_cnt++;

            NBIOT_DEBUG_INFO("recv_cn =%d\r\n", recv_cnt);            
          
            //理论上不会运行到这里, 运行到这里，有两种情况，
            //一种是表示传输出错，收到的数据都是乱码        
            //第二种是IDLE成帧判断不是那么准确，没有收完也以为一帧结束了
            if (recv_cnt > (AT_CMD_RESPONSE_PAR_NUM_MAX + 20))                         
            {  
               //接收在超时时间内未正常完成，停止接收超时  
               atk_soft_timer_stop(&nbiot_handle->p_uart_dev->uart_rx_timer);   
              
               //收到的是乱码,强制接收结束，同时根据命令的属性判断命令是否重发或跳过该命令执行下一条命令
               next_cmd = nbiot_response_handle(nbiot_handle, FALSE);
              
               recv_cnt = 0;
               //清缓存            
               nbiot_recv_buf_reset();                
            }
            else 
            {
              
               //命令未完成,收到的的是命令回显当中其中的一部分                
               atk_soft_timer_timeout_change(&nbiot_handle->p_uart_dev->uart_rx_timer, 10000);
             
            }              
                               
        } 
            
        nbiot_event_clr(nbiot_handle, NBIOT_RECV_EVENT); 
    }
       
    if (nbiot_event_get(nbiot_handle, NBIOT_REG_STA_EVENT)) 
    {      
        NBIOT_DEBUG_INFO("reg event\r\n");

                               
        //通知上层应用网络注册结果
        nbiot_msg_send(nbiot_handle, NULL, TRUE); 

        next_cmd = AT_CMD_NEXT;
        nbiot_event_clr(nbiot_handle,   NBIOT_REG_STA_EVENT); 
      
        //清除缓存数据    
        nbiot_recv_buf_reset();   
    }
    
        
    if (nbiot_event_get(nbiot_handle, NBIOT_COAP_RECV_EVENT)) 
    {
        
        NBIOT_DEBUG_INFO("coap recv event ok\r\n");
      
        //通知上层应用接收到CoAP数据
        nbiot_msg_send(nbiot_handle, NULL, TRUE);          
             
        nbiot_event_clr(nbiot_handle, NBIOT_COAP_RECV_EVENT);

         //复位状态标志
         nbiot_status_reset();         
      
        //清除缓存数据    
        nbiot_recv_buf_reset();  
    }
    
    if (nbiot_event_get(nbiot_handle, NBIOT_NCDP_RECV_EVENT)) 
    {
        
        NBIOT_DEBUG_INFO("ncdp recv event ok\r\n");
      
        //通知上层应用接收到NCDP数据
        nbiot_msg_send(nbiot_handle, NULL, TRUE);          
             
        nbiot_event_clr(nbiot_handle, NBIOT_NCDP_RECV_EVENT); 
      
        //复位状态标志
        nbiot_status_reset();   
      
        //清除缓存数据    
        nbiot_recv_buf_reset();  
    }

    if (nbiot_event_get(nbiot_handle, NBIOT_NCDP_STATUS_EVENT)) 
    {
        
        NBIOT_DEBUG_INFO("ncdp status event ok\r\n");
      
        //通知上层应用NCDP状态连接发生变化
        nbiot_msg_send(nbiot_handle, NULL, TRUE);          
             
        nbiot_event_clr(nbiot_handle, NBIOT_NCDP_STATUS_EVENT); 
      
        //复位状态标志
        nbiot_status_reset();   
      
        //清除缓存数据    
        nbiot_recv_buf_reset();  
    }


    if (nbiot_event_get(nbiot_handle, NBIOT_CSON_STATUS_EVENT)) 
    {
        
        NBIOT_DEBUG_INFO("cson status event ok\r\n");
      
        //通知上层应用NCDP状态连接发生变化
        nbiot_msg_send(nbiot_handle, NULL, TRUE);          
             
        nbiot_event_clr(nbiot_handle, NBIOT_CSON_STATUS_EVENT); 
      
             
        //清除缓存数据    
        nbiot_recv_buf_reset();  
    }     
    
    
    if (nbiot_event_get(nbiot_handle, NBIOT_SOCKET_ERR_EVENT)) 
    {       
        NBIOT_DEBUG_INFO("socket err event\r\n");
      
        //通知上层应用SOCKET失败了
        nbiot_msg_send(nbiot_handle, NULL, TRUE);        
              
        nbiot_event_clr(nbiot_handle, NBIOT_SOCKET_ERR_EVENT); 
      
        //复位状态标志
        nbiot_status_reset();   
      
        //清除缓存数据    
        nbiot_recv_buf_reset();  
    }
    
    if (nbiot_event_get(nbiot_handle, NBIOT_OTHER_EVENT)) 
    {       
        NBIOT_DEBUG_INFO("other event\r\n");
         
        //清除缓存数据    
        nbiot_recv_buf_reset();        
        nbiot_event_clr(nbiot_handle, NBIOT_OTHER_EVENT); 
      
    }    

    if (nbiot_event_get(nbiot_handle, NBIOT_TIMEOUT_EVENT)) 
    {        
        //超时处理，根据命令的属性尝试重发命令或者跳过该命令执行下一条命令        
        next_cmd = nbiot_response_handle(nbiot_handle, FALSE);
      
        //通知上层应用，此动作执行超时
        if (g_at_cmd.cmd_action & ACTION_ERROR_BUT_NEXT) 
        {           
            at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1] = (char *)g_at_cmd.p_atcmd;
           
            NBIOT_DEBUG_INFO("%s cmd not repsonse or send failed\r\n", g_at_cmd.p_atcmd);
                               
            //通知上层应用，此动作因超时执行失败后跳过该命令执行
            nbiot_msg_send(nbiot_handle, &at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1], NBIOT_ERROR_NEXT);            
        }        
        else if (g_at_cmd.cmd_action & ACTION_ERROR_AND_TRY) 
        {           
            at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1] = (char *)g_at_cmd.p_atcmd;
           
            NBIOT_DEBUG_INFO("%s cmd not repsonse or send failed\r\n", g_at_cmd.p_atcmd);
                               
            //通知上层应用，此动作因超时执行失败后跳过该命令执行
            nbiot_msg_send(nbiot_handle, &at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1], NBIOT_ERROR_RETRY);            
        }
        else        
        {            
            NBIOT_DEBUG_INFO("%s cmd is failed and exit\r\n", g_at_cmd.p_atcmd);        
            at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1] = (char*)g_at_cmd.p_atcmd;
            
            //通知上层应用，此动作因超时执行失败后结束命令执行
            nbiot_msg_send(nbiot_handle, &at_response_par[AT_CMD_RESPONSE_PAR_NUM_MAX - 1], FALSE);  

            //复位状态标志
            nbiot_status_reset();              
        } 
        
        //清除垃圾数据      
        nbiot_recv_buf_reset(); 
        recv_cnt = 0;        
                                                    
        nbiot_event_clr(nbiot_handle, NBIOT_TIMEOUT_EVENT); 
    } 
    
    //根据事件及状态判断是否需要执行下一条命令
    if(next_cmd == AT_CMD_NEXT)
    {
        //执行下一条命令
        if (at_cmd_next())
        {
            nbiot_at_cmd_send(nbiot_handle, &g_at_cmd);
        }
        
        //返回FALSE表示子进程已经结束了
        else
        {
            //代表该主状态下所有的子状态命令已经成功执行完成了
            nbiot_msg_send(nbiot_handle, NULL,TRUE);

            //复位sm状态标志
            nbiot_status_reset();
        }     
    }
   
    return NBIOT_OK;    
}

/**
  * @brief  nbiot数据发送  
  */
static int  __nbiot_uart_data_tx (void *p_arg, uint8_t *pData, uint16_t size, uint32_t Timeout)
{  
    int ret = 0;
    
    nbiot_handle_t  nbiot_handle = (nbiot_handle_t)p_arg;
    
    uart_handle_t uart_handle = nbiot_handle->p_uart_dev; 
    
    ret = uart_data_tx_poll(uart_handle, pData, size, Timeout); 

    return ret;    
}

/**
  * @brief  nbiot数据接收   
  */
static int  __nbiot_uart_data_rx (void *p_arg, uint8_t *pData, uint16_t size, uint32_t Timeout)
{  
    int ret = 0;
    
    //超时参数保留使用
    (void)Timeout;
    
    nbiot_handle_t  nbiot_handle = (nbiot_handle_t)p_arg;
    
    uart_handle_t uart_handle = nbiot_handle->p_uart_dev;

    uart_ring_buf_read(uart_handle, pData, size);    
    
    return ret;    
}


/**
  * @brief nbiot at指令初始化
  * @param  at_cmd           : 具体AT指令
  * @param  argument         : 指令对应的参数
  * @param  property         : 指令参数
  * @param  at_cmd_time_out  : 指令的超时时间
  * @retval None
  */
void nbiot_at_cmd_param_init (at_cmdhandle cmd_handle,
                              const char *at_cmd,
                              char *argument,
                              cmd_property_t property,
                              uint32_t at_cmd_time_out)
{
    if (cmd_handle == NULL)
    {
        return;
    }
    cmd_handle->cmd_try     = CMD_TRY_TIMES;
    cmd_handle->property    = property;
    cmd_handle->cmd_action  = ACTION_OK_AND_NEXT | ACTION_ERROR_AND_TRY;
    cmd_handle->p_atcmd_arg = argument;
    cmd_handle->p_expectres = NULL;
    cmd_handle->have_tried  = 0;
    cmd_handle->max_timeout = at_cmd_time_out;
    cmd_handle->p_atcmd     = at_cmd;
}

/**
  * @brief 生成nbiot at指令的字符串及其对应的长度
  * @param  cmd_handle  : AT指令的句柄
  * @retval 生成后，AT总共的有效长度  
  */
static int cmd_generate(at_cmdhandle cmd_handle)
{
    int cmdLen = 0;
    
    if (cmd_handle == NULL)
    {
        return cmdLen;
    }
    memset(g_nbiot_send_desc.buf,0,NBIOT_SEND_BUF_MAX_LEN);
    g_nbiot_send_desc.len = 0;

    if(cmd_handle->property == CMD_TEST)
    {
        cmdLen = snprintf(g_nbiot_send_desc.buf,NBIOT_SEND_BUF_MAX_LEN,
                          "%s=?\r\n",
                          cmd_handle->p_atcmd);
    }    
    else if(cmd_handle->property == CMD_READ)
    {
        cmdLen = snprintf(g_nbiot_send_desc.buf,NBIOT_SEND_BUF_MAX_LEN,
                          "%s?\r\n",
                          cmd_handle->p_atcmd);
    }
    else if(cmd_handle->property == CMD_EXCUTE)
    {
        cmdLen = snprintf(g_nbiot_send_desc.buf,NBIOT_SEND_BUF_MAX_LEN,
                          "%s\r\n",
                          cmd_handle->p_atcmd);    
    }

    else if(cmd_handle->property == CMD_SET)
    {
        cmdLen = snprintf(g_nbiot_send_desc.buf,NBIOT_SEND_BUF_MAX_LEN,
                          "%s=%s\r\n",
                          cmd_handle->p_atcmd,cmd_handle->p_atcmd_arg);    
    }
    
    //cmdlen是有效的数据长度，不包括字符串结束标记符
    g_nbiot_send_desc.len = cmdLen;
    
    return cmdLen;
}

/**
  * @brief 通过AT指令的响应内容，判断at指令是否发送成功
  * @param  buf  :  指令的回响数据的缓存
  * @retval 返回值为 ACTION_OK_EXIT 这一类枚举类型
  */
static int8_t at_cmd_result_parse (char *buf)
{
    int8_t result = -1;
  
  
    //确认收到的数据是否为命令相应的参数(+) 
    char *p_colon = strchr(buf,'+');
    
    //确认收到的数据是否为命令相应的参数(:)
    char *p_colon_temp = strchr(buf,':');
       
    if(g_at_cmd.p_expectres == NULL)
    {
        if (strstr(buf,"OK"))
        {
            result = AT_CMD_RESULT_OK;
        }
        else if (strstr(buf,"ERROR"))
        {
            result = AT_CMD_RESULT_ERROR;
          
        } else if ((strstr(buf,g_at_cmd.p_atcmd)) || ((p_colon && p_colon_temp))) {
          
            NBIOT_DEBUG_INFO("p_colon %s  p_colon_temp %s \r\n", p_colon, p_colon_temp);
            //前者成者是命令的回显 || 后者成立OK之前的命令参数
            result = AT_CMD_RESULT_CONTINUE;
          
        } else {
          
           //乱码
           result = AT_CMD_RESULT_RANDOM_CODE;
        }
    }
    else
    {
        if (strstr(buf,"OK"))
        {
            //与得到的期望值一致
            if(strstr(buf,g_at_cmd.p_expectres))
            {
                result = AT_CMD_RESULT_OK;
            }
            else
            {
                result = AT_CMD_RESULT_ERROR;
            }
            
        }
        else if(strstr(buf,"ERROR"))
        {
            //ERROR
            result = AT_CMD_RESULT_ERROR;
        }
         
        else if (strstr(buf, g_at_cmd.p_atcmd) || ((p_colon && p_colon_temp))) {
          
             //前者成者是命令的回显 || 后者成立OK之前的命令参数
             result = AT_CMD_RESULT_CONTINUE;
          
        } else {
          
             //乱码
             result = AT_CMD_RESULT_RANDOM_CODE;      
          
        }    
    }

    return result;
}



/**
  * @brief 根据接收的数产生nbiot决定产生对应的一步事件通知
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  buf           : 接收数据的缓存
  * @retval NBIOT_OK 成功  
  */
static uint8_t nbiot_event_notify (nbiot_handle_t nbiot_handle, char *buf)
{
    char *target_pos_start = NULL;
         
    if((target_pos_start = strstr(buf, "+CEREG:")) != NULL)
    {
        char *p_colon = strchr(target_pos_start,',');
      
        if (p_colon)
        {
            p_colon = p_colon + 1;
            
            //该命令后面直接跟的网络注册状态的信息，用字符来表示的，要转换成数字        
            g_nbiot_connect_status.register_status = (*p_colon - '0');
            p_colon  = strstr(buf, "OK");
        }        
        else 
        {
           p_colon = strchr(target_pos_start,':');
          
            if (p_colon)
            {
                p_colon = p_colon + 1;
                
                //该命令后面直接跟的网络注册状态的信息，用字符来表示的，要转换成数字        
                g_nbiot_connect_status.register_status = (*p_colon - '0');
            }
        }
                       
        if (p_colon !=NULL)
        {                      
            nbiot_event_set(nbiot_handle, NBIOT_REG_STA_EVENT);  
        } 
        else
        {
             //如果收到回复，其它是AT命令响应的数据
             nbiot_event_set(nbiot_handle, NBIOT_RECV_EVENT);           
        }          
    }    
    else if ((target_pos_start = strstr(buf,"+QLWEVTIND:")) != NULL)
    {
        //收到服务器端发来NCDP状态数据
        char *p_colon = strchr(target_pos_start, ':');
              
        //得到NCDP当前的状态
        if (p_colon)
        {
            p_colon++;
            g_nbiot_connect_status.m2m_status = strtoul(p_colon,0,10);
        } 
                              
        nbiot_event_set(nbiot_handle, NBIOT_NCDP_STATUS_EVENT);  
        nbiot_status_set(NBIOT_NCDP_STATUS, NBIOT_SUB_NCDP_STATUS);          
    }
    
    else if ((target_pos_start = strstr(buf,"+CSCON:")) != NULL)
    {
        //收到服务器端发来NCDP状态数据
        char *p_colon = strchr(target_pos_start, ':');
        nbiot_status_sm_t sm_status = {0,0};
              
        //得到NCDP当前的状态
        if (p_colon)
        {
            p_colon++;
            g_nbiot_connect_status.cscon_status = strtoul(p_colon,0,10);
        } 
                              
        nbiot_event_set(nbiot_handle, NBIOT_CSON_STATUS_EVENT);
        nbiot_status_get(&sm_status);

        if ((sm_status.main_status != NBIOT_INIT) && (sm_status.main_status != NBIOT_INFO)) 
        {          
             nbiot_status_set(NBIOT_CSCON_STATUS, NBIOT_SUB_CSCON_STATUS);       
        }          
    } 

    else if ((target_pos_start = strstr(buf,"REGISTERNOTIFY")) != NULL)
    {
                              
        nbiot_event_set(nbiot_handle, NBIOT_OTHER_EVENT);      
    }     

    else if ((target_pos_start = strstr(buf,"+NNMI:")) != NULL)
    {
        //收到服务器端发来NCDP数据
        char *p_colon = strchr(target_pos_start, ':');
             
        //得到有效数据的起始地址
        if (p_colon)
        {
            p_colon =  p_colon + 3;
            g_nbiot_connect_status.data_offest = p_colon;
                     
        } 
                              
        nbiot_event_set(nbiot_handle, NBIOT_NCDP_RECV_EVENT);  
        nbiot_status_set(NBIOT_NCDP_RECV, NBIOT_SUB_NCDP_RECV);           
    }    
    else if((target_pos_start = strstr(buf,"+CLMOBSERVE")) != NULL)
    {
        nbiot_event_set(nbiot_handle, NBIOT_LWM2M_RECV_EVENT);          
    } 

    //收到MQTT数据包    
    else if ((target_pos_start = strstr(buf,"+CMQPUB")) != NULL)
    {        
        nbiot_event_set(nbiot_handle, NBIOT_MQTT_RECV_EVENT);  
    }    
    else 
    {
        //如果收到回复，其它是AT命令响应的数据
        nbiot_event_set(nbiot_handle, NBIOT_RECV_EVENT);  
    }
    
    return 0;
}

/**
  * @brief 判断是否产生下一条AT指令
  * @param  type          : 连接类型，值仅为枚举当中的值
  * @retval FALSE 指令流程结束  NBIOT_ERROR 设置失败  
  */
static uint8_t at_cmd_next (void)
{ 
    if (g_nbiot_sm_status.main_status == NBIOT_INIT)
    {
        g_nbiot_sm_status.sub_status++;
      
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        {
            return FALSE;
        }

        switch(g_nbiot_sm_status.sub_status)
        {
          
        case NBIOT_SUB_SYNC:
            
            break;
        
         case NBIOT_SUB_QLED:
          
            nbiot_at_cmd_param_init(&g_at_cmd, AT_QLEDMODE, "1", CMD_SET, 500);
          
            break;       
        
        case NBIOT_SUB_CMEE:
          
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CMEE, "1", CMD_SET, 500);
            break;        
        
        case NBIOT_SUB_BAND:
          
            nbiot_at_cmd_param_init(&g_at_cmd, AT_NBAND, "5", CMD_SET, 500);
            g_at_cmd.cmd_action  = ACTION_OK_AND_NEXT | ACTION_ERROR_BUT_NEXT;
          
            break;
        
        case NBIOT_SUB_QREGSWT:
          
            nbiot_at_cmd_param_init(&g_at_cmd, AT_QREGSWT, "0", CMD_SET, 500);
          
            break;        
        
        case NBIOT_SUB_ATI:
          
            nbiot_at_cmd_param_init(&g_at_cmd, AT_ATI, NULL, CMD_EXCUTE, 3000);
            g_at_cmd.p_expectres = "Quectel";     //设置期望回复消息，如果指令执行完成
                                                  //没有与期望的消息匹配，则认为出错
                                                  //并进行出错尝试           
            break;
                      
        //查询NB卡状态是否准备好    
        case NBIOT_SUB_CPIN:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_NCCID, NULL, CMD_READ, 3000);
            g_at_cmd.p_expectres = "+NCCID:"; //设置期望回复消息，如果指令执行完成
                                              //没有与期望的消息匹配，则认为出错
                                              //并进行出错尝试              
          }
          break;
                    
          
        //查询射频模块信号质量   
        case NBIOT_SUB_CSQ:
          {
             nbiot_at_cmd_param_init(&g_at_cmd, AT_CSQ, NULL, CMD_EXCUTE, 3000);
          }
          break;

        //使能模块射频信号,响应等待的最长时间为6S      
        case NBIOT_SUB_CFUN:                                   
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CFUN,"1",CMD_SET, 6000);
          }
          
          break;
          
        // 使能nbiot网络注册   
        case NBIOT_SUB_CEREG:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CEREG, "1", CMD_SET, 500);
            g_at_cmd.cmd_action  = ACTION_OK_WAIT | ACTION_ERROR_AND_TRY;  
              
          }
          break; 

        // 使能CSCONT提示 
        case NBIOT_SUB_CSCON:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CSCON, "1", CMD_SET, 500); 
              
          }
          break;            
          
                 
        //查询PDP激活信息          
        case NBIOT_SUB_CIPCA_QUERY:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CIPCA, NULL, CMD_READ, 500);

          }
          break;          
               
          
        //使能网络附着,最大响应时间为1s,留余量这里设为3s     
        case NBIOT_SUB_CGATT:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CGATT, "1", CMD_SET, 3000);            
          }
          break;
          
          
        //查询模块的网络状态信息     
        case NBIOT_SUB_NUESTATS:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_NUESTATS, NULL, CMD_EXCUTE, 500);
          }
          break;
          
        //查询分配的APN信息及IP地址     
        case NBIOT_SUB_CGPADDR:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CGPADDR, NULL, CMD_EXCUTE, 500);
               
          }
          break; 

        case NBIOT_SUB_NNMI:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_NNMI, "1", CMD_SET, 500);
               
          }            
          break;  


//        case NBIOT_SUB_NSMI:
//          {
//            nbiot_at_cmd_param_init(&g_at_cmd, AT_NSMI, "1", CMD_SET, 500);
//               
//          }            
//          break;          
          
          
        //查询网络附着信息,最大响应时间不详       
        case NBIOT_SUB_CGATT_QUERY:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CGATT, NULL, CMD_READ, 3000);
            
            //设置期望回复消息，如果指令执行完成
            //没有与期望的消息匹配，则认为出错                                             
            //并进行出错尝试               
            g_at_cmd.p_expectres = "CGATT:1";     
          }
          break;
          
                                      
        //查询nbiot网络是否注册,最大响应时间不详       
        case NBIOT_SUB_CEREG_QUERY:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CEREG, NULL, CMD_READ, 500);             

          }
          break;
                    
        default: 
          
          //强制表示子进程结束
          g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        
          return FALSE;
                   
         }
    }
    
    else if (g_nbiot_sm_status.main_status == NBIOT_INFO)
    {
        g_nbiot_sm_status.sub_status++;
      
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        {
            return FALSE;
        }
        
        switch(g_nbiot_sm_status.sub_status)
        {
          
        case  NBIOT_SUB_CGMI:
          
          {
            nbiot_at_cmd_param_init(&g_at_cmd,AT_CGMI,NULL,CMD_EXCUTE,3000);
          }
          break;

                   
        case NBIOT_SUB_CGMM:
          {
            nbiot_at_cmd_param_init(&g_at_cmd,AT_CGMM,NULL,CMD_EXCUTE,3000);
          }
          break;
          
        case NBIOT_SUB_CGMR:
          {
            nbiot_at_cmd_param_init(&g_at_cmd,AT_CGMR,NULL,CMD_EXCUTE,3000);
          }
          break;
          
        case NBIOT_SUB_CIMI:
          {
            nbiot_at_cmd_param_init(&g_at_cmd,AT_CIMI,NULL,CMD_EXCUTE,3000);
          }
          break;

        case NBIOT_SUB_CGSN:
          {
            nbiot_at_cmd_param_init(&g_at_cmd, AT_CGSN, "1", CMD_SET, 3000);
            
            //设置期望回复消息为络
            //如果指令执行完成,没有与期望的消息匹配                                            
            //则认为出错，并进行出错尝试               
            g_at_cmd.p_expectres = "CGSN";
          }
          break;             
          
          
        case NBIOT_SUB_NBAND:
          {
            nbiot_at_cmd_param_init(&g_at_cmd,AT_NBAND,NULL,CMD_READ,3000);
            //设置期望回复消息为络
            //如果指令执行完成,没有与期望的消息匹配                                            
            //则认为出错，并进行出错尝试               
            g_at_cmd.p_expectres = "NBAND";
          }
          break;
          
        default: 
          
          //强制表示子进程结束
          g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        
          return FALSE;          
        
        }
    }
    
    else if (g_nbiot_sm_status.main_status == NBIOT_SIGNAL)
    {
        
        g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        return FALSE;
    }
    
    else if (g_nbiot_sm_status.main_status == NBIOT_CSCON)
    {
        
        g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        return FALSE;
    }    
    else if (g_nbiot_sm_status.main_status == NBIOT_RESET)
    {
        
        g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        return FALSE;
    }
    else if (g_nbiot_sm_status.main_status == NBIOT_NCONFIG)
    {
        
        g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        return FALSE;
    }     
    

    else if (g_nbiot_sm_status.main_status == NBIOT_NCDP_SERVER) 
    {
        g_nbiot_sm_status.sub_status++;
        
        switch (g_nbiot_sm_status.sub_status)
        {
                        
        case NBIOT_SUB_NCDP_CONNECT:          
          {
                                              
            //最大响应时间为300ms,这里留余量设为500ms                                    
            nbiot_at_cmd_param_init(&g_at_cmd, AT_QLWSREGIND, "0", CMD_SET, 500);
                                        
          }
          break;

                                     
        default: 
          
          //强制表示子进程结束
          g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
        
          return FALSE;          
        
        }      
          
    }
   
     else if (g_nbiot_sm_status.main_status == NBIOT_NCDP_SEND) 
    {
      
        g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
      
        return FALSE;    
      
    } 
    
    else if (g_nbiot_sm_status.main_status == NBIOT_NCDP_CL) 
    {
      
        g_nbiot_sm_status.sub_status = NBIOT_SUB_END;
      
        return FALSE;     
    } 
       
    else if (g_nbiot_sm_status.main_status == NBIOT_NONE)   
    {  //防止意外重发
       return FALSE; 
    }   
    else 
    {
      
    }
    
    return TRUE;
}


/**
  * @brief 发送消息,回调用户注册进来的回调函数
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  buf           : 指令响应数据的二维指针，其元素的值为一个一维的字符串数据
  * @retval NBIOT_OK 成功  
  */
static void nbiot_msg_send (nbiot_handle_t nbiot_handle, char**buf, int8_t is_ok)
{
    if (nbiot_handle == NULL)
    {
        return;
    }

     //出错或超时，尝试重试
    if ((is_ok == NBIOT_ERROR_RETRY) || 
        (is_ok == NBIOT_ERROR_TIMEOUT)) {
        
         nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_CMD_RETRY, strlen(buf[0]), buf[0]);  
        
         return;      
    }

    //出错及跳过该指令执行  
    else if (is_ok == NBIOT_ERROR_NEXT)
    {

        nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_CMD_NEXT, strlen(buf[0]), buf[0]);  
        
        return;
    }    
           
    //出错，则上报此流程执行失败
    else if(is_ok == FALSE)
    {
        nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_CMD_FAIL, strlen(buf[0]), buf[0]);
        //复位状态标志
        nbiot_status_reset(); 
        return;
    }

    else if (g_nbiot_sm_status.main_status == NBIOT_INIT)
    {
        switch(g_nbiot_sm_status.sub_status)
        {
            
        case NBIOT_SUB_SYNC:
            
            break;
        
         case NBIOT_SUB_QLED:
                   
          
            break;          

        case NBIOT_SUB_CMEE:
          
            break;    

        case NBIOT_SUB_ATI:
          
            //得到模块的名字
            memcpy(g_firmware_info.name, buf[1], strlen(buf[1])); 
            
            break;

        //查询NB卡状态是否准备好    
        case NBIOT_SUB_CPIN:
        {
            break; 
        }
                         
        case NBIOT_SUB_CSQ:
        {
            char *p_colon = strchr(buf[0],':');
          
            if (p_colon != NULL) 
            {            
                p_colon++;
                //转换成10进制数字
                uint8_t lqi =strtoul(p_colon,0, 10);
                //运算取得每个数值对应的dbm范围
                int8_t rssi = -110 + (lqi << 1);
                uint8_t len = snprintf(buf[0],10,"%d",rssi);
                *(buf[0]+len) = 0;
                
                g_nbiot_connect_status.rssi = rssi;
                
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_CSQ,len,buf[0]);
              
            }           
          
            break;
        }

        case NBIOT_SUB_QREGSWT:
            
            break;        
                  
        case NBIOT_SUB_CFUN:
            
            break;
        
        case NBIOT_SUB_CSCON:
            
            break;        


        case NBIOT_SUB_CEREG:
           
            NBIOT_DEBUG_INFO("reg status=%d\r\n", g_nbiot_connect_status.register_status);           
            break;
                
        case NBIOT_SUB_CIPCA_QUERY:
        {
            char *p_colon = strchr(buf[0],':');
                        
            if (p_colon != NULL) 
            {                
                p_colon++;
                
                //转换成10进制数字,得到当前创建的cid
                g_nbiot_connect_status.cid =strtoul(p_colon,0, 10);
            }
            
            break;
         }
               
        case NBIOT_SUB_CGATT:
            
            break;


        case NBIOT_SUB_CGATT_QUERY:
            
            break;

        case NBIOT_SUB_CGPADDR:
            
            break;  


//        case NBIOT_SUB_NNMI:
//            
//            break;
//        
//            
//        case NBIOT_SUB_NSMI:

//            break;
        
           
        case NBIOT_SUB_END:
            
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_INIT,1,"OK");

            break;

        default:
          
            break;
        }
    }
    else if(g_nbiot_sm_status.main_status == NBIOT_NCONFIG)  
    {          
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        {
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_NCONFIG,1,"OK"); 
        }        

    }

    else if(g_nbiot_sm_status.main_status == NBIOT_RESET)  
    {
         
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        {
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_RESET,1,"OK"); 
        }        
    }    
    
    else if(g_nbiot_sm_status.main_status == NBIOT_INFO)
    {
        switch(g_nbiot_sm_status.sub_status)
        {
            
            //查询网络注册状态    
            case NBIOT_SUB_CEREG_QUERY:

                nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_REG, 1, (char *)&g_nbiot_connect_status.register_status);

                break;        
                
                         
            case NBIOT_SUB_CGMI:
            {
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_MID, strlen(buf[0]), buf[0]); 
                break;
            }
                        
            case NBIOT_SUB_CGMM:
            {
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_MMODEL,strlen(buf[0]),buf[0]);  
                break;
            }
              
              
            case NBIOT_SUB_CGMR:
            {

                nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_MREV,strlen(buf[0]),buf[0]);  
                break;            
            }
                                 
            case NBIOT_SUB_CIMI:
            {
                memcpy(g_firmware_info.IMSI,buf[0],15);
                g_firmware_info.IMSI[15] = 0;
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_IMSI,strlen(buf[0]),buf[0]);
                break;
            }
                        
            case NBIOT_SUB_CGSN:
            {
                char *p_colon = strchr(buf[0],':');
                
                if (p_colon)
                {
                    p_colon = p_colon + 1;
                    memcpy(g_firmware_info.IMEI ,p_colon,15);
                    g_firmware_info.IMEI[15] = 0;
                    nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_IMEI,15,(char*)g_firmware_info.IMEI);
                } 
                break;
            }
                                       
            case NBIOT_SUB_NBAND:
            {
                char *p_colon = strchr(buf[0],':');
                char *pFreq = NULL;
                if  (p_colon)
                {
                    p_colon++;
                    uint8_t hz_id = strtoul(p_colon,0,10);
                    if(hz_id == BAND_850MHZ_ID)
                    {
                        //850MHZ
                        pFreq = BAND_850MHZ_STR;
                    }
                    else if(hz_id == BAND_900MHZ_ID)
                    {
                        //900MHZ
                        pFreq = BAND_900MHZ_STR;
                    }
                    else if(hz_id == BAND_800MHZ_ID)
                    {
                        //800MHZ 
                        pFreq = BAND_800MHZ_STR;
                    }
                    else 
                    {
                        //700MHZ
                        pFreq = BAND_700MHZ_STR;
                    }
                    
                    nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_BAND,strlen(pFreq),pFreq);
                }
                
                break;
            }
              
            case NBIOT_SUB_END:
            {
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_INFO, 1, "OK");
                break;
            }
            
            default:
               break;
              
        }
    }
    else if(g_nbiot_sm_status.main_status == NBIOT_SIGNAL)
    {
        switch(g_nbiot_sm_status.sub_status) 
        { 
            case NBIOT_SUB_CSQ:
            {
                char *p_colon = strchr(buf[0],':');
              
                if (p_colon != NULL) 
                {            
                    p_colon++;
                    //转换成10进制数字
                    uint8_t lqi =strtoul(p_colon,0, 10);
                    //运算取得每个数值对应的dbm范围
                    int8_t rssi = -110 + (lqi << 1);
                    uint8_t len = snprintf(buf[0],10,"%d",rssi);
                    *(buf[0]+len) = 0;
                    
                    g_nbiot_connect_status.rssi = rssi;
                    
                    nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_CSQ,len,buf[0]);
                  
                }  
              
                break;
            } 
              
            case NBIOT_SUB_END:
            {
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_SIGNAL,1,"OK");
                break;
            }

            default:
              
                break;
        }

    }
    
    else if(g_nbiot_sm_status.main_status == NBIOT_CSCON)
    {
        switch(g_nbiot_sm_status.sub_status) 
        { 
            case NBIOT_SUB_CSCON_QUERY:
            {
                char *p_colon = strchr(buf[0],',');
              
                if (p_colon != NULL) 
                {            
                    p_colon++;
                    //转换成10进制数字
                    uint8_t active_status =strtoul(p_colon,0, 10);
     
                    
                    g_nbiot_connect_status.cscon_status = active_status;
                    
                    nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_CSCON, 1, (char *)&g_nbiot_connect_status.cscon_status);
                  
                }  
              
                break;
            } 
              
            case NBIOT_SUB_END:
            {
                nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_CSCON, 1, (char *)&g_nbiot_connect_status.cscon_status);
                break;
            }

            default:
              
                break;
        }

    }    
    
    else if(g_nbiot_sm_status.main_status == NBIOT_NCDP_SERVER)
    {
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        { 
            g_nbiot_connect_status.connect_status = 1;         
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_NCDP_SERVER, 1, "OK");
        }
    }  

    else if(g_nbiot_sm_status.main_status == NBIOT_NCDP_SEND)
    {
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        {
            char *p_buf_tmep = g_nbiot_send_desc.buf;
          
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_NCDP_SEND, strlen(p_buf_tmep), p_buf_tmep);
        }
    }
    else if(g_nbiot_sm_status.main_status == NBIOT_NCDP_RECV)
    {
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_NCDP_RECV)
        {
            char *data_buf = g_nbiot_connect_status.data_offest; 
                   
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_NCDP_RECV,strlen(data_buf),data_buf);
                       
        }
    }

    else if(g_nbiot_sm_status.main_status == NBIOT_NCDP_STATUS)
    {
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_NCDP_STATUS)
        {                      
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_NCDP_STATUS, 1, (char *)&g_nbiot_connect_status.m2m_status);
          

                   
        }
    }

    else if(g_nbiot_sm_status.main_status == NBIOT_CSCON_STATUS)
    {
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_CSCON_STATUS)
        {                      
            nbiot_handle->nbiot_cb(nbiot_handle->p_arg, (nbiot_msg_id_t)NBIOT_MSG_CSCON_STATUS, 1, (char *)&g_nbiot_connect_status.cscon_status);
            //复位状态标志
            nbiot_status_reset();        
        }
    }    

    else if(g_nbiot_sm_status.main_status == NBIOT_NCDP_CL)
    {
        if (g_nbiot_sm_status.sub_status == NBIOT_SUB_END)
        {
             g_nbiot_connect_status.connect_status = 0;  
          
             char *p_buf_tmep = NULL;
                                        
             if (g_nbiot_connect_status.connect_type == NBIOT_NCDP)
             {
                 p_buf_tmep = "ncdp close";
             }         
            
             nbiot_handle->nbiot_cb(nbiot_handle->p_arg,(nbiot_msg_id_t)NBIOT_MSG_NCDP_CLOSE, strlen(p_buf_tmep), p_buf_tmep);
        }
    }
    else
    {

    }    
}

/**
  * @brief  指令响应结果处理，这个函数利用命令的状态属性，来结束状态的执行.
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  cmd_response  : 指令响应的结果，指令响应成功，传入的值为True
  * @retval 返回 AT_CMD_OK 这一类的宏值
  */
static uint8_t nbiot_response_handle (nbiot_handle_t nbiot_handle, uint8_t cmd_response)
{
    uint8_t next_cmd = AT_CMD_OK;
      
    if (cmd_response)
    {
        if (g_at_cmd.cmd_action & ACTION_OK_AND_NEXT)
        {
            next_cmd = AT_CMD_NEXT;
        }        
        else if (g_at_cmd.cmd_action & ACTION_OK_WAIT)   
        {
            //代表命令执行成功将等待
            next_cmd = AT_CMD_WAIT;             
        }
        else  
        {
            //代表命令执行成功后退出该次状态机所包括所有命令的流程
            g_at_cmd.cmd_action = ACTION_OK_EXIT;             
        }
    }
    else
    {
        if(g_at_cmd.cmd_action & ACTION_ERROR_AND_TRY)
        {
            g_at_cmd.have_tried++;

            if (g_at_cmd.have_tried < g_at_cmd.cmd_try)
            {
                //出错重发该命令
                nbiot_at_cmd_send(nbiot_handle, &g_at_cmd);
            }
            else
            {
                  
               //重试达到最大次数，该变命令属性，代表命令执行错误后退出该次状态机所包括所有命令的流程
                g_at_cmd.cmd_action = ACTION_ERROR_EXIT;
                
            }
        }        
        else if (g_at_cmd.cmd_action & ACTION_OK_WAIT)   
        {
            //代表命令执行错误将等待
            next_cmd = AT_CMD_WAIT;             
        }
                
        else if (!(g_at_cmd.cmd_action & ACTION_ERROR_EXIT))  
        {
            //命令执行错误跳过该命令执行下一条命令
            next_cmd = TRUE;
        }
        else 
        {
            //nerver reach here  
        }
    }
    
    return next_cmd;
}

/**
  * @brief  给nbiot模块发送AT指令.
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  cmd_handle    : 将要发送指令信息句柄
  * @retval 返回 0 代表发送成功
  */
int nbiot_at_cmd_send(nbiot_handle_t nbiot_handle, at_cmdhandle cmd_handle)
{
    int strLen = 0;
    
    int ret = 0;
        
    if (nbiot_handle == NULL || cmd_handle == NULL)
    {
       return NBIOT_ERROR;
    }
    
    //生成命令的数据及长度    
    strLen = cmd_generate(cmd_handle);

    //回调命令发送函数
    ret = nbiot_handle->p_drv_funcs->nbiot_send_data(nbiot_handle, 
                                                     (uint8_t*)g_nbiot_send_desc.buf, 
                                                     strLen,                                                    
                                                     cmd_handle->max_timeout);   
    return ret;
}

/**
  * @brief  接收来自nbiot模块的数据.
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  pData    : 将要发送消息缓冲区指针
  * @param  size     : 将要发送消息缓冲区的长度
  * @param  Timeout  : 设置接收的超时值
  * @retval 返回 0 代表发送成功
  */
static int nbiot_data_recv(nbiot_handle_t nbiot_handle, uint8_t *pData, uint16_t size, uint32_t Timeout)
{   
    int ret = 0;
        
    if (nbiot_handle == NULL)
    {
       return NBIOT_ERROR;
    }
    
    //回调接收数据函数 
    ret = nbiot_handle->p_drv_funcs->nbiot_recv_data(nbiot_handle, pData, size, Timeout);      
    
    return ret;
}


/**
  * @brief  nbiot模块设备实例初始化 .
  * @param  nbiot_handle  : 指向nbiot设备句柄的指针.
  * @param  cmd_handle    : 将要发送指令信息句柄
  * @retval 返回 nbiot模块设备句柄的指针 
  */
nbiot_handle_t nbiot_dev_init(uart_handle_t nbiot_handle)
{
     //填充设备结构体
     g_nbiot_dev.p_uart_dev       = nbiot_handle;
     g_nbiot_dev.p_drv_funcs      = &drv_funcs; 

     g_nbiot_dev.p_nbiot_cmd      = &g_at_cmd;    
     g_nbiot_dev.p_socket_info    = g_socket_info;
     g_nbiot_dev.p_firmware_info  = &g_firmware_info;
     g_nbiot_dev.p_sm_status      = &g_nbiot_sm_status;
     g_nbiot_dev.p_connect_status = &g_nbiot_connect_status;
  
     g_nbiot_dev.frame_format     = 0;  
    
     //注册串口收发事件回调函数
     uart_event_registercb(nbiot_handle, __uart_event_cb_handle, &g_nbiot_dev);     
    
     return &g_nbiot_dev;    
}

/**
  * @brief  注册nbiot模块事件回调函数.
  * @param  cb     : 模块设备回调.
  * @param  p_arg  : 模块设备回调函数参数
  * @retval 返回 nbiot模块设备句柄的指针 
  */
void nbiot_event_registercb (nbiot_handle_t nbiot_handle, nbiot_cb cb, void *p_arg)
{  
    if(cb != 0)
    {
        nbiot_handle->nbiot_cb  = (nbiot_cb)cb;
        nbiot_handle->p_arg     = p_arg;
    }
}









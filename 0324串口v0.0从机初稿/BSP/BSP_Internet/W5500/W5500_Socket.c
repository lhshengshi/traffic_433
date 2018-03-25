/******************** (C) COPYRIGHT DIY嵌入式开发工作室*******************************
 *
 *                              ARM Cortex-M4 Port
 *
 * 文件名称     : BSP.h (  )
 * 作    者     :    
 * 环    境	 	: MDK5.10a-->固件库V1.3.0版本
 * 开发平台     : STM32F407ZGT6-25MHz->168MHz
 * 修改时间     : 2014-10-20
 * 版    本     : V1.0.0
 * 硬件连接     : STM32F40xx-STM32开发板
 * 说    明     : Socket编程相关函数
 *
 *
**************************************************************************************/
/* Includes---------------------------------------------*/
#include "./BSP_Internet/w5500/W5500.h"
#include "./BSP_Internet/w5500/W5500_SPI.h"
#include "./BSP_Internet/w5500/W5500_Conf.h"
#include "./BSP_Internet/w5500/W5500_Socket.h"
/* Private defines--------------------------------------*/
#define waittime  5
#define waitnum   200
/* Private typedef -------------------------------------*/
/* Private macro ---------------------------------------*/
/* Private variables -----------------------------------*/
static uint16_t local_port;
/* Private function prototypes -------------------------*/
/* Private functions -----------------------------------*/
/**
*@brief		This Socket function initialize the channel in perticular mode,
*			    and set the port and wait for W5200 done it.
*@param		s: socket number.
*@param		protocol: The socket to chose.
*@param		port:The port to bind.
*@param		flag: Set some bit of MR,such as **< No Delayed Ack(TCP) flag.
*@return  	1 for sucess else 0.
*/
uint8_t socket(SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag)
{
    uint8_t ret;
    uint16_t wtime;
    if (((protocol&0x0F) == Sn_MR_TCP)||((protocol&0x0F) == Sn_MR_UDP)||((protocol&0x0F) == Sn_MR_IPRAW)|| ((protocol&0x0F) == Sn_MR_MACRAW)||((protocol&0x0F) == Sn_MR_PPPOE))
    {
        close(s);
        IINCHIP_WRITE(Sn_MR(s) ,protocol | flag);
        if (port != 0) {
            IINCHIP_WRITE( Sn_PORT0(s) ,(uint8_t)((port & 0xff00) >> 8));
            IINCHIP_WRITE( Sn_PORT1(s) ,(uint8_t)(port & 0x00ff));
        } else {
            local_port++; 									/*If don't set the source port, set local_port number*/
            IINCHIP_WRITE(Sn_PORT0(s) ,(uint8_t)((local_port & 0xff00) >> 8));
            IINCHIP_WRITE(Sn_PORT1(s) ,(uint8_t)(local_port & 0x00ff));
        }
        IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_OPEN); 			/*Run sockinit Sn_CR*/

        while( IINCHIP_READ(Sn_CR(s)) )					/*Wait to process the command*/
        {
            Delay_us(waittime);
            if(wtime++ > waitnum)
                goto  breakwhile;
        }
        ret = 1;
    }
    else
    {
breakwhile:
        ret = 0;
    }
    return ret;
}

/**
*@brief		This function close the socket and parameter is "s" which represent the socket number
*@param		s: socket number.
*@return	None
*/
void close(SOCKET s)
{
    uint16_t wtime;
    IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_CLOSE);

    while( IINCHIP_READ(Sn_CR(s) ) )						/*Wait to process the command*/
    {
        Delay_us(waittime);
        if(wtime++ > waitnum)
            goto  breakwhile;
    }
breakwhile:
    IINCHIP_WRITE( Sn_IR(s) , 0xFF);					/*All clear*/
}


/**
*@brief		This function established  the connection for the channel in passive (server) mode.
*			This function waits for the request from the peer.
*@param		s: socket number.
*@return 	1 for success else 0.
*/
uint8_t listen(SOCKET s)
{
    uint8_t ret;
    uint16_t wtime;
    if (IINCHIP_READ( Sn_SR(s) ) == SOCK_INIT)
    {
        IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_LISTEN);

        while( IINCHIP_READ(Sn_CR(s) ) )				 	/*Wait to process the command*/
        {
            Delay_us(waittime);
            if(wtime++ > waitnum)
                goto  breakwhile;
        }
        ret = 1;
    }
    else
    {
breakwhile:
        ret = 0;
    }
    return ret;
}

/**
*@brief		This function established  the connection for the channel in Active (client) mode.
*			This function waits for the untill the connection is established.
*@param		s: socket number.
*@param		addr: The server IP address to connect.
*@param		port: The server IP port to connect.
*@return  	1 for success else 0.
*/
uint8_t connect(SOCKET s, uint8_t * addr, uint16_t port)
{
    uint8_t ret;
    uint8_t wtime;
    if(((addr[0] == 0xFF)  && (addr[1] == 0xFF) && (addr[2] == 0xFF) && (addr[3] == 0xFF)) || ((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) ||(port == 0x00))
    {
        ret = 0;
    } else
    {
        ret = 1;
        IINCHIP_WRITE( Sn_DIPR0(s), addr[0]);
        IINCHIP_WRITE( Sn_DIPR1(s), addr[1]);
        IINCHIP_WRITE( Sn_DIPR2(s), addr[2]);
        IINCHIP_WRITE( Sn_DIPR3(s), addr[3]);
        IINCHIP_WRITE( Sn_DPORT0(s), (uint8_t)((port & 0xff00) >> 8));
        IINCHIP_WRITE( Sn_DPORT1(s), (uint8_t)(port & 0x00ff));
        IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_CONNECT);

        while ( IINCHIP_READ(Sn_CR(s) ) ) 							/*Wait for completion*/
        {
            Delay_us(waittime);
            if(wtime++ > waitnum)
            {
                ret = 0;
                goto  breakwhile;
            }
        }
        wtime = 0;

        while ( IINCHIP_READ(Sn_SR(s)) != SOCK_SYNSENT )
        {
            if(IINCHIP_READ(Sn_SR(s)) == SOCK_ESTABLISHED)
            {
                break;
            }
            if (getSn_IR(s) & Sn_IR_TIMEOUT)
            {
                IINCHIP_WRITE(Sn_IR(s), (Sn_IR_TIMEOUT));  			/*Clear TIMEOUT Interrupt*/
                ret = 0;
                break;
            }
            Delay_us(waittime);
            if(wtime++ > waitnum)
            {
                ret = 0;
                goto  breakwhile;
            }
        }
    }
breakwhile:
    return ret;
}

/**
*@brief		This function used for disconnect the socket s.
*@param		s: socket number.
*@return  	1 for success else 0.
*/
void disconnect(SOCKET s)
{
    uint8_t wtime=0;
    IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_DISCON);

    while( IINCHIP_READ(Sn_CR(s) ) )									 /*Wait to process the command*/
    {
        Delay_us(waittime);
        if(wtime++ > waitnum)
        {
            goto  breakwhile;
        }
    }
breakwhile:;
}

/**
*@brief   This function used to send the data in TCP mode
*@param		s: socket number.
*@param		buf: data buffer to send.
*@param		len: data length.
*@return  1 for success else 0.
*/
uint16_t send(SOCKET s, const uint8_t * buf, uint16_t len)
{
    uint8_t status=0;
    uint16_t ret=0;
    uint16_t freesize=0;
    uint8_t wtime=0;
    if (len > getIINCHIP_TxMAX(s)) ret = getIINCHIP_TxMAX(s);	 		/*Check size not to exceed MAX size*/
    else ret = len;

    do
    {
        freesize = getSn_TX_FSR(s);
        status = IINCHIP_READ(Sn_SR(s));
        if ((status != SOCK_ESTABLISHED) && (status != SOCK_CLOSE_WAIT))
        {
            ret = 0;
            break;
        }
		if(wtime++ > waitnum)
        {
            goto  breakwhile;
        }
    } while (freesize < ret);
     wtime=0;
    send_data_processing(s, (uint8_t *)buf, ret);						 /*Copy data*/
    IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_SEND);
    /*Wait to process the command*/
    while( IINCHIP_READ(Sn_CR(s) ) )
    {
        Delay_us(waittime);
        if(wtime++ > waitnum)
        {
            goto  breakwhile;
        }
    }


    while ( (IINCHIP_READ(Sn_IR(s) ) & Sn_IR_SEND_OK) != Sn_IR_SEND_OK )
    {
        status = IINCHIP_READ(Sn_SR(s));
        if ((status != SOCK_ESTABLISHED) && (status != SOCK_CLOSE_WAIT) )
        {
            close(s);
            return 0;
        }
        Delay_us(waittime);
        if(wtime++ > waitnum)
        {
            goto  breakwhile;
        }
    }
    IINCHIP_WRITE( Sn_IR(s) , Sn_IR_SEND_OK);

#ifdef __DEF_IINCHIP_INT__
    putISR(s, getISR(s) & (~Sn_IR_SEND_OK));
#else
    IINCHIP_WRITE( Sn_IR(s) , Sn_IR_SEND_OK);
#endif
breakwhile:
    return ret;
}

/**
*@brief		This function is an application I/F function which is used to receive the data in TCP mode.
*			It continues to wait for data as much as the application wants to receive.
*@param		s: socket number.
*@param		buf: data buffer to receive.
*@param		len: data length.
*@return  	received data size for success else 0.
*/
uint16_t recv(SOCKET s, uint8_t * buf, uint16_t len)
{
    uint16_t ret=0;
    uint8_t wtime;
    if ( len > 0 )
    {
        recv_data_processing(s, buf, len);
        IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_RECV);

        while( IINCHIP_READ(Sn_CR(s) ))								 /*Wait to process the command*/
        {
            Delay_us(waittime);
            if(wtime++ > waitnum)
            {
                goto  breakwhile;
            }
        }
breakwhile:
        ret = len;
    }
    return ret;
}

/**
*@brief		This function is an application I/F function which is used to send the data for other then TCP mode.
*			Unlike TCP transmission, The peer's destination address and the port is needed.
*@param		s: socket number.
*@param		buf: data buffer to send.
*@param		len: data length.
*@param		addr: IP address to send.
*@param		port: IP port to send.
*@return  	This function return send data size for success else 0.
*/
uint16_t sendto(SOCKET s, const uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t port)
{
    uint16_t ret=0;
    uint8_t wtime;
    if (len > getIINCHIP_TxMAX(s)) ret = getIINCHIP_TxMAX(s); 										/*Check size not to exceed MAX size*/
    else ret = len;

    if( ((addr[0] == 0x00) && (addr[1] == 0x00) && (addr[2] == 0x00) && (addr[3] == 0x00)) || ((port == 0x00)) )//||(ret == 0) )
    {
        ret = 0;
    }else
    {
        IINCHIP_WRITE( Sn_DIPR0(s), addr[0]);
        IINCHIP_WRITE( Sn_DIPR1(s), addr[1]);
        IINCHIP_WRITE( Sn_DIPR2(s), addr[2]);
        IINCHIP_WRITE( Sn_DIPR3(s), addr[3]);
        IINCHIP_WRITE( Sn_DPORT0(s),(uint8_t)((port & 0xff00) >> 8));
        IINCHIP_WRITE( Sn_DPORT1(s),(uint8_t)(port & 0x00ff));
        send_data_processing(s, (uint8_t *)buf, ret);				   	/*Copy data*/
        IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_SEND);

        while( IINCHIP_READ( Sn_CR(s) ) )							   	/*Wait to process the command*/
        {
            Delay_us(waittime);
            if(wtime++ > waitnum)
            {
                goto  breakwhile;
            }
        }
        wtime=0;
        while( (IINCHIP_READ( Sn_IR(s) ) & Sn_IR_SEND_OK) != Sn_IR_SEND_OK )
        {
            if (IINCHIP_READ( Sn_IR(s) ) & Sn_IR_TIMEOUT)
            {
                IINCHIP_WRITE( Sn_IR(s) , (Sn_IR_SEND_OK | Sn_IR_TIMEOUT)); 	/*Clear SEND_OK & TIMEOUT*/
                return 0;
            }
            Delay_us(waittime);
            if(wtime++ > waitnum)
            {
                goto  breakwhile;
            }
        }
        IINCHIP_WRITE( Sn_IR(s) , Sn_IR_SEND_OK);
    }
breakwhile:
    return ret;
}

/**
*@brief		This function is an application I/F function which is used to receive the data in other then
*			TCP mode. This function is used to receive UDP, IP_RAW and MAC_RAW mode, and handle the header as well.
*@param		s: socket number.
*@param		buf: data buffer to receive.
*@param		len: data length.
*@param		addr: IP address to receive.
*@param		port: IP port to receive.
*@return	This function return received data size for success else 0.
*/
uint16_t recvfrom(SOCKET s, uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t *port)
{
    uint8_t head[8];
    uint16_t data_len=0;
    uint16_t ptr=0;
    uint32_t addrbsb =0;
    uint8_t wtime;
    if ( len > 0 )
    {
        ptr     = IINCHIP_READ(Sn_RX_RD0(s) );
        ptr     = ((ptr & 0x00ff) << 8) + IINCHIP_READ(Sn_RX_RD1(s));
        addrbsb = (uint32_t)(ptr<<8) +  (s<<5) + 0x18;

        switch (IINCHIP_READ(Sn_MR(s) ) & 0x07)
        {
        case Sn_MR_UDP :
            wiz_read_buf(addrbsb, head, 0x08);
            ptr += 8;
            /*Read peer's IP address, port number*/
            addr[0]  = head[0];
            addr[1]  = head[1];
            addr[2]  = head[2];
            addr[3]  = head[3];
            *port    = head[4];
            *port    = (*port << 8) + head[5];
            data_len = head[6];
            data_len = (data_len << 8) + head[7];

            addrbsb = (uint32_t)(ptr<<8) +  (s<<5) + 0x18;
            wiz_read_buf(addrbsb, buf, data_len);
            ptr += data_len;

            IINCHIP_WRITE( Sn_RX_RD0(s), (uint8_t)((ptr & 0xff00) >> 8));
            IINCHIP_WRITE( Sn_RX_RD1(s), (uint8_t)(ptr & 0x00ff));
            break;

        case Sn_MR_IPRAW :
            wiz_read_buf(addrbsb, head, 0x06);
            ptr += 6;
            addr[0]  = head[0];
            addr[1]  = head[1];
            addr[2]  = head[2];
            addr[3]  = head[3];
            data_len = head[4];
            data_len = (data_len << 8) + head[5];

            addrbsb  = (uint32_t)(ptr<<8) +  (s<<5) + 0x18;
            wiz_read_buf(addrbsb, buf, data_len);
            ptr += data_len;

            IINCHIP_WRITE( Sn_RX_RD0(s), (uint8_t)((ptr & 0xff00) >> 8));
            IINCHIP_WRITE( Sn_RX_RD1(s), (uint8_t)(ptr & 0x00ff));
            break;

        case Sn_MR_MACRAW :
            wiz_read_buf(addrbsb, head, 0x02);
            ptr+=2;
            data_len = head[0];
            data_len = (data_len<<8) + head[1] - 2;
            if(data_len > 1514)
            {
                goto  breakwhile;
            }
            addrbsb  = (uint32_t)(ptr<<8) +  (s<<5) + 0x18;
            wiz_read_buf(addrbsb, buf, data_len);
            ptr += data_len;

            IINCHIP_WRITE( Sn_RX_RD0(s), (uint8_t)((ptr & 0xff00) >> 8));
            IINCHIP_WRITE( Sn_RX_RD1(s), (uint8_t)(ptr & 0x00ff));
            break;

        default :
            break;
        }
        IINCHIP_WRITE( Sn_CR(s) ,Sn_CR_RECV);

        while( IINCHIP_READ( Sn_CR(s)) ) 	/*Wait to process the command*/
        {
            Delay_us(waittime);
            if(wtime++ > waitnum)
            {
                goto  breakwhile;
            }
        }

    }
breakwhile:
    return data_len;
}

/********** (C) COPYRIGHT DIY嵌入式开发工作室 （END OF FILE ）************************/







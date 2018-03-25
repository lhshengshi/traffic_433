/**
******************************************************************************
* @file    			HTTPS.c
* @author  			WIZnet Software Team
* @version 			V1.0
* @date    			2015-02-14
* @brief   			heeputil.c/.h webpge.h 配合使用 完成Web Server功能
* @attention		选用IE浏览器进行测试
*
******************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "W5500.h"
#include "W5500_Socket.h"

#include "http_server.h"

extern char  homepage_default[];
char tx_buf[MAX_URI_SIZE];
char rx_buf[MAX_URI_SIZE];
uint8_t pub_buf[1024];

/**
*@brief		  转化转义字符为ascii charater
*@param		  url：需要转化网页地址
*@return	  无
*/
void unescape_http_url(char * url)
{
    int x, y;

    for (x = 0, y = 0; url[y]; ++x, ++y)
    {
        if ((url[x] = url[y]) == '%')
        {
            url[x] = C2D(url[y+1])*0x10+C2D(url[y+2]);
            y+=2;
        }
    }
    url[x] = '\0';
}

/**
*@brief		  执行一个答复，如 html, gif, jpeg,etc.
*@param		  buf- 答复数据
*@param			type- 答复数据类型
*@param			len-  答复数据长度
*@return	  无
*/
void make_http_response_head(unsigned char * buf,char type,unsigned long len)
{
    char * head;
    char tmp[10];
    memset(buf,0x00,MAX_URI_SIZE);
    /* 文件类型*/
    if 	(type == PTYPE_HTML) head = RES_HTMLHEAD_OK;
    else if (type == PTYPE_GIF)	head = RES_GIFHEAD_OK;
    else if (type == PTYPE_TEXT)	head = RES_TEXTHEAD_OK;
    else if (type == PTYPE_JPEG)	head = RES_JPEGHEAD_OK;
    else if (type == PTYPE_FLASH)	head = RES_FLASHHEAD_OK;
    else if (type == PTYPE_MPEG)	head = RES_MPEGHEAD_OK;
    else if (type == PTYPE_PDF)	head = RES_PDFHEAD_OK;

    sprintf(tmp,"%ld", len);
    strcpy((char*)buf, head);
    strcat((char*)buf, tmp);
    strcat((char*)buf, "\r\n\r\n");
}

/**
*@brief		  寻找一个MIME类型文件
*@param		  buf- MIME型文件
*@param		  type-数据类型
*@return	  无
*/
void find_http_uri_type(unsigned char * type, char * buf)
{
    /* Decide type according to extention*/
    if 	(strstr(buf, ".pl"))				*type = PTYPE_PL;
    else if (strstr(buf, ".html") || strstr(buf,".htm"))	*type = PTYPE_HTML;
    else if (strstr(buf, ".gif"))				*type = PTYPE_GIF;
    else if (strstr(buf, ".text") || strstr(buf,".txt"))	*type = PTYPE_TEXT;
    else if (strstr(buf, ".jpeg") || strstr(buf,".jpg"))	*type = PTYPE_JPEG;
    else if (strstr(buf, ".swf")) 				*type = PTYPE_FLASH;
    else if (strstr(buf, ".mpeg") || strstr(buf,".mpg"))	*type = PTYPE_MPEG;
    else if (strstr(buf, ".pdf")) 				*type = PTYPE_PDF;
    else if (strstr(buf, ".cgi") || strstr(buf,".CGI"))	*type = PTYPE_CGI;
    else if (strstr(buf, ".js")  || strstr(buf,".JS"))	*type = PTYPE_TEXT;
    else if (strstr(buf, ".xml") || strstr(buf,".XML"))	*type = PTYPE_HTML;
    else 							*type = PTYPE_ERR;
}

/**
*@brief		  解析每一个http响应
*@param		  request： 定义一个指针
*@return	  无
*/
void parse_http_request(st_http_request * request,unsigned char* buf)
{
    char * nexttok;
    nexttok = strtok((char*)buf," ");
    if(!nexttok)
    {
        request->METHOD = METHOD_ERR;
        return;
    }
    if(!strcmp(nexttok, "GET") || !strcmp(nexttok,"get"))
    {
        request->METHOD = METHOD_GET;
        nexttok = strtok(NULL," ");

    }
    else if (!strcmp(nexttok, "HEAD") || !strcmp(nexttok,"head"))
    {
        request->METHOD = METHOD_HEAD;
        nexttok = strtok(NULL," ");

    }
    else if (!strcmp(nexttok, "POST") || !strcmp(nexttok,"post"))
    {
        nexttok = strtok(NULL,"\0");//20120316
        //nexttok = strtok(NULL," ");
        request->METHOD = METHOD_POST;

    }
    else
    {
        request->METHOD = METHOD_ERR;
    }
    if(!nexttok)
    {
        request->METHOD = METHOD_ERR;
        return;
    }
    strcpy(request->URI,nexttok);
}

/**
*@brief		  得到响应过程中的下一个参数
*@param		  url：需要转化网页地址
*@param		  param_name：
*@return	  返回一个数据
*/
uint8_t* get_http_param_value(char* uri, char* param_name)
{
    uint16_t len;
    uint8_t* pos2;
    uint8_t* name=0;
    uint8_t *ret=pub_buf;
    uint16_t content_len=0;
    char tmp_buf[10]= {0x00,};
    if(!uri || !param_name) return 0;
    /***************/
    mid(uri,"Content-Length: ","\r\n",tmp_buf);
    content_len=atoi16(tmp_buf,10);
    //printf("content len=%d\r\n",content_len);
    uri = (char*)strstr(uri,"\r\n\r\n");
    uri+=4;
    //printf("uri=%s\r\n",uri);
    uri[content_len]=0;
    /***************/
    name= (uint8_t*)strstr(uri,param_name);
    if(name)
    {
        name += strlen(param_name) + 1;
        pos2=(uint8_t*)strstr((char*)name,"&");
        if(!pos2) pos2=name+strlen((char*)name);
        len=0;
        len = pos2-name;

        if(len)
        {
            ret[len]=0;
            strncpy((char*)ret,(char*)name,len);
            unescape_http_url((char *)ret);
            replacetochar((char *)ret,'+',' ');
        } else ret[0]=0;
    } else return 0;
    return ret;
}





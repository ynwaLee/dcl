#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asn.h"

#define PKT_SIZE    10485760

int get_tlc(unsigned char * pkt, int len, unsigned int * ptag, int * ptaglen, int * pconstructed, unsigned char ** pcontent, int * pcontentlen);

//计算长度为0x80的数据长度 = 内容长度 + 2字节00
int calc_unsure_len(unsigned char * pkt, int maxlen)
{
	int len = 0;
	unsigned int tag;
	int constructed;
	unsigned char * content;
	int taglen, contentlen;
	while(maxlen-len >= 2)
	{
		if(pkt[len]==0 && pkt[len+1]==0)
		{
			return len+2;
		}
		
		if(!get_tlc(pkt+len, maxlen-len, &tag, &taglen, &constructed, &content, &contentlen))
			break;
		
		len += taglen + contentlen;
	}
	
	return 0;
}

int get_tlc(unsigned char * pkt, int len, unsigned int * ptag, int * ptaglen, int * pconstructed, unsigned char ** pcontent, int * pcontentlen)
{
	if(len < 2)
		return 0;
	*ptaglen = 0;
	*pconstructed = (pkt[*ptaglen] & 0x20) >> 5;
	if((pkt[*ptaglen]&0x1F) == 0x1F)	//tag>30
	{
		(*ptaglen) ++;
		*ptag = 0;
		while(*ptaglen<len && (pkt[*ptaglen]&0x80))
		{
			*ptag <<= 7;
			*ptag += pkt[*ptaglen] & 0x7F;
			(*ptaglen) ++;
		}
		if(*ptaglen >= len)
			return 0;
		*ptag <<= 7;
		*ptag += pkt[*ptaglen] & 0x7F;
		(*ptaglen) ++;
	}
	else
	{
		*ptag = pkt[(*ptaglen) ++];
	}
	if(*ptaglen >= len)
		return 0;
	if(pkt[*ptaglen] == 0x80)
	{
		(*ptaglen) ++;
		*pcontentlen = calc_unsure_len(pkt+*ptaglen, len-*ptaglen);
		if(*pcontentlen == 0)
			return 0;
	}
	else if(pkt[*ptaglen] > 0x80)
	{
		int lenlen = pkt[*ptaglen] & 0x7f;
		(*ptaglen) ++;
		if(*ptaglen+lenlen >= len)
			return 0;
		int i;
		*pcontentlen = 0;
		for(i=0; i<lenlen; i++)
		{
			*pcontentlen *= 256;
			*pcontentlen += pkt[(*ptaglen) ++];
		}
	}
	else
	{
		*pcontentlen = pkt[(*ptaglen) ++];
	}
	
	if(*pcontentlen<0 || *ptaglen+*pcontentlen>len)
		return 0;
	
	*pcontent = pkt + *ptaglen;
	return 1;
}

//layer			: 每一层的标签，由调用者分配空间
//cur_layer		: 当前层，从1开始
//max_layers	: 最多解析层数
//asn_callback	: 回调函数
//para			: 用户定义参数
//返回值		: 这一层的总字节数
int deasn(unsigned char * pkt, int len, void * para, unsigned int * layer, int cur_layer, int max_layers, ASN_CALLBACK asn_callback)
{
	unsigned int tag;
	int constructed;
	unsigned char * content;
	int taglen, contentlen;
	if(!get_tlc(pkt, len, &tag, &taglen, &constructed, &content, &contentlen))
		return 0;
	layer[cur_layer-1] = tag;
	
	asn_callback(tag, constructed, content, contentlen, layer, cur_layer, para);
	
	if(constructed)	//包含下一层
	{
		if(cur_layer >= max_layers)
			return taglen+contentlen;
		int offset = taglen;
		int retlen;
		while(contentlen+taglen-offset >= 2)
		{
			retlen = deasn(pkt+offset, len-offset, para, layer, cur_layer+1, max_layers, asn_callback);
			if(!retlen)
				return 0;
			offset += retlen;
		}
	}
	
	return taglen+contentlen;
}

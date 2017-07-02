#include "amrcode.h"
#include "mfa.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>
#include "g729_decoder.h"
#include "ftransfer.h"

amrnb_code_t amrnb_code[MAX_AMRNB_CODE] = {
	{0, "AMR475",	4.75*1000,	13, 0x04},
	{1, "AMR515",	5.15*1000,	14, 0x0c},
	{2, "AMR590",	5.90*1000,	16, 0x14},
	{3, "AMR670",	6.70*1000,	18, 0x1c},
	{4, "AMR740",	7.40*1000,	20, 0x24},
	{5, "AMR795",	7.95*1000,	21, 0x2c},
	{6, "AMR1020",	10.20*1000, 27, 0x34},
	{7, "AMR1220",	12.20*1000, 32, 0x3c},
};

int bps2framesize(double bps)
{
	return (int)((bps/50/8) + 0.5) + 1;
}

int amrnb_code_index(int frameheader)
{
	int i = 0;
	
	for (i = 0; i < MAX_AMRNB_CODE; i++) {
		if (amrnb_code[i].frame_header == frameheader) {
			return i;
		}
	}

	return -1;
}


static int write_buff(int fd, void *buff, int len)
{
	int	n = 0;
	int ret = 0;

	while (n < len) {
		ret = write(fd, buff + n, len - n);
		if (ret == -1) {
			return -1;
		}
		n += ret;
	}

	return 0;
}

static int amrnbsc2wave(int amrfd, int wavefd)
{
	unsigned char amrbuff[1024];
	short wavebuff[256];
	short buff[1024];
	unsigned char frameheader = 0;
	int index = 0;
	int frame_data_size = 0;
	int *destate = NULL;
	int datasize = 0;
	int wavelen = 160;
	int i = 0;
	/*if amrfd is a file, skip the file header*/
	if (read(amrfd, &frameheader, 1) != 1) {
		return -1;
	}

	if (frameheader == '#') {
		int len = sizeof(AMRNB_SC_FILE_HEADER) - 1 - 1;
		if (read(amrfd, amrbuff, len) != len) {
			return -1;
		}
		if (read(amrfd, &frameheader, 1) != 1) {
			return -1;
		}
	}

	destate = Decoder_Interface_init();
	while (1) {
		index = amrnb_code_index(frameheader);
		if (index == -1) {
			amrbuff[0] = frameheader;
			memset(amrbuff+1, 0, sizeof(amrbuff)-1);
		}else {
			amrbuff[0] = frameheader;
			frame_data_size = amrnb_code[index].frame_size - 1;
			if (read(amrfd, amrbuff + 1, frame_data_size) == 0) {
				datasize = -1;
				break;
			}
		}
		
		Decoder_Interface_Decode(destate, amrbuff, wavebuff, 0);
		
		for (i = 0; i < wavelen; i++) {
			buff[i*2] = wavebuff[i];
			buff[i*2 + 1] = wavebuff[i];
		}
		datasize += 2*wavelen*sizeof(short);
		encrypt_wave_data((unsigned char *)buff, datasize);
		if (write_buff(wavefd, buff, datasize) == -1) {
			datasize = -1;
			break;
		}

		if (read(amrfd, &frameheader, 1) != 1) {
			datasize = -1;
			break;
		}
	}

	Decoder_Interface_exit(destate);

	return datasize;
}

static int amrnbmc2wave(int amrfd, int wavefd)
{
	/*if amrfd is a file, skip the file header*/
	unsigned char amrbuff[1024];
	short wavebuff[2][256];
	short buff[1024];
	int frame_index = 0;
	unsigned char frameheader = 0;
	int index = 0;
	int frame_data_size = 0;
	int data_size = 0;
	int *destate[2] = {NULL, NULL};
	int nwave = 160;
	int i = 0;

	/*if amrfd is a file, skip the file header*/
	if (read(amrfd, &frameheader, 1) != 1) {
		return -1;
	}

	if (frameheader == '#') {
		int len = sizeof(AMRNB_MC_FILE_HEADER) - 1 - 1 + 4;
		if (read(amrfd, amrbuff, len) != len) {
			return -1;
		}
		if (read(amrfd, &frameheader, 1) != 1) {
			return -1;
		}
	}

	destate[0] = Decoder_Interface_init();
	destate[1] = Decoder_Interface_init();
	for (; 1; frame_index++) {
		index = amrnb_code_index(frameheader);
		if (index == -1) {
			amrbuff[0] = frameheader;
			memset(amrbuff+1, 0, sizeof(amrbuff)-1);
		}else {
			amrbuff[0] = frameheader;
			frame_data_size = amrnb_code[index].frame_size - 1;
			if (read(amrfd, amrbuff + 1, frame_data_size) == 0) {
				break;
			}
		}
		
		Decoder_Interface_Decode(destate[frame_index % 2], amrbuff, wavebuff[frame_index % 2], 0);
		
		if ((frame_index % 2) == 1) {
			for (i = 0; i < nwave; i++) {
				buff[2*i] = wavebuff[0][i];	
				buff[2*i+1] = wavebuff[1][i];	
			}

			encrypt_wave_data((unsigned char *)buff, nwave*sizeof(short)*2);
			if (write_buff(wavefd, buff, nwave*sizeof(short)*2) == -1) {
				data_size = -1;	
				break;
			}

			data_size += nwave*sizeof(short)*2;
		}

		if (read(amrfd, &frameheader, 1) != 1) {
			break;
		}
	}

	Decoder_Interface_exit(destate[0]);
	Decoder_Interface_exit(destate[1]);

	return data_size;
}

static int amrwbsc2wave(int amrfd, int wavefd)
{
	return 0;
}
static int amrwbmc2wave(int amrfd, int wavefd)
{
	return 0;
}

int amr2wave(int amrfd, int wavefd, int amrcode)
{
	switch (amrcode) {
		case AMR_CODE_NBSC:
			return amrnbsc2wave(amrfd, wavefd);
			break;
		case AMR_CODE_NBMC:
			return amrnbmc2wave(amrfd, wavefd);
			break;
		case AMR_CODE_WBSC:
			return amrwbsc2wave(amrfd, wavefd);
			break;
		case AMR_CODE_WBMC:
			return amrwbmc2wave(amrfd, wavefd);
			break;
		default:
			return -1;
	}

	return 0;
}

int g7292wave(int g729fd, int wavefd,unsigned int filesize)
{
		unsigned char buf_up[20]; 
		unsigned char buf_down[20];   
		unsigned char vocdata[filesize];
		unsigned char len_up;
		unsigned char len_down;   
		G729DECODER *decoder_up;  
		G729DECODER *decoder_down;	 
		short buf_mix[320];
		short *pcm_up; 
		short *pcm_down;
		unsigned int data_len;
		unsigned int offset = 0;
		unsigned int len = 0;
		unsigned int n = 0;
		int i; 
		decoder_up = G729DecInit();
		decoder_down = G729DecInit(); 
		data_len = 0;	
		lseek(g729fd,17,SEEK_SET);
		read(g729fd,vocdata,filesize);
		while (1) 
		{		
			if(len >= filesize)
				break;
			memcpy(&len_up, vocdata + n +offset, 1); 
			if(len_up == 0 || len_up == '\0')
			{
				n += 1;
				continue;
			}
			n +=1;
	 
			memcpy(buf_up, vocdata + n + offset, 20); 
			pcm_up = G729Decode(decoder_up, buf_up);
					
			offset += 20;
	
			memcpy(&len_down, vocdata + n + offset, 1); 
			if(len_down == 0 || len_down == '\0')
			{
				n += 1;
				continue;
			}
			n +=1;
	
			memcpy(buf_down, vocdata + n + offset, 20);
			pcm_down = G729Decode(decoder_down, buf_down);
	
			offset += 20;
			len = n + offset;
			for (i = 0; i < 160; i++) 
			{			 
				buf_mix[i * 2] = pcm_up[i]; 		  
				buf_mix[i * 2 + 1] = pcm_down[i];		 
			}
	
			encrypt_wave_data((unsigned char *)buf_mix, 160*sizeof(short)*2);
			write(wavefd, buf_mix, 320*2);
			data_len += (320 * sizeof(short));
		}

	close(g729fd);    
	close(wavefd); 
	G729DecClose(decoder_up); 
	G729DecClose(decoder_down);
	
	return data_len;
}

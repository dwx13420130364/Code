#include "app_md5.h"
#include "math.h"
#include "string.h"
#include "stmflash.h"
#include "rtthread.h"

unsigned char PADDING[]={0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  

												                           
void MD5Init(md5Ctx_t *context)  
{  
     context->count[0] = 0;  
     context->count[1] = 0;  
     context->state[0] = 0x67452301;  
     context->state[1] = 0xEFCDAB89;  
     context->state[2] = 0x98BADCFE;  
     context->state[3] = 0x10325476;  
}  
void MD5Update(md5Ctx_t *context,unsigned char *input,unsigned int inputlen)  
{  
    unsigned int i = 0,index = 0,partlen = 0;  
    index = (context->count[0] >> 3) & 0x3F;  
    partlen = 64 - index;  
    context->count[0] += inputlen << 3;  
    if(context->count[0] < (inputlen << 3))  
       context->count[1]++;  
    context->count[1] += inputlen >> 29;  
      
    if(inputlen >= partlen)  
    {  
       memcpy(&context->buffer[index],input,partlen);  
       MD5Transform(context->state,context->buffer);  
		for(i = partlen;i+64 <= inputlen;i+=64)  
           MD5Transform(context->state,&input[i]);  
		
		index = 0;          
	}    
	else  
	{  
			i = 0;  
	}  
	memcpy(&context->buffer[index],&input[i],inputlen-i);  
}  


void MD5Final(md5Ctx_t *context, unsigned char digest[16])  
{  
    unsigned int index = 0, padlen = 0;  
    unsigned char bits[8];  
    index = (context->count[0] >> 3) & 0x3F;  
    padlen = (index < 56) ? (56 - index) : (120 - index);  
    MD5EncodeIntToChar(bits, context->count, 8);  
    MD5Update(context, PADDING, padlen);  
    MD5Update(context, bits, 8);  
    MD5EncodeIntToChar(digest, context->state, 16);  
}  


void MD5EncodeIntToChar(unsigned char *output, unsigned int *input, unsigned int len)  
{  
    unsigned int i = 0, j = 0;  
    while (j < len)  
    {  
        output[j] = input[i] & 0xFF;  
        output[j + 1] = (input[i] >> 8) & 0xFF;  
        output[j + 2] = (input[i] >> 16) & 0xFF;  
        output[j + 3] = (input[i] >> 24) & 0xFF;  
        i++;  
        j += 4;  
    }  
}

void MD5DecodeCharToInt(unsigned int *output, unsigned char *input, unsigned int len)  
{  
    unsigned int i = 0, j = 0;  
    while (j < len)  
    {  
        output[i] = (input[j]) |  
            (input[j + 1] << 8) |  
            (input[j + 2] << 16) |  
            (input[j + 3] << 24);  
        i++;  
        j += 4;  
    }  
}  

void MD5Transform(unsigned int state[4], unsigned char block[64])  
{  
    unsigned int a = state[0];  
    unsigned int b = state[1];  
    unsigned int c = state[2];  
    unsigned int d = state[3];  
    unsigned int x[64];  
  
    MD5DecodeCharToInt(x, block, 64);  
    FF(a, b, c, d, x[0], 7, 0xd76aa478);  
    FF(d, a, b, c, x[1], 12, 0xe8c7b756);  
    FF(c, d, a, b, x[2], 17, 0x242070db);  
    FF(b, c, d, a, x[3], 22, 0xc1bdceee);  
    FF(a, b, c, d, x[4], 7, 0xf57c0faf);  
    FF(d, a, b, c, x[5], 12, 0x4787c62a);  
    FF(c, d, a, b, x[6], 17, 0xa8304613);  
    FF(b, c, d, a, x[7], 22, 0xfd469501);  
    FF(a, b, c, d, x[8], 7, 0x698098d8);  
    FF(d, a, b, c, x[9], 12, 0x8b44f7af);  
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);  
    FF(b, c, d, a, x[11], 22, 0x895cd7be);  
    FF(a, b, c, d, x[12], 7, 0x6b901122);  
    FF(d, a, b, c, x[13], 12, 0xfd987193);  
    FF(c, d, a, b, x[14], 17, 0xa679438e);  
    FF(b, c, d, a, x[15], 22, 0x49b40821);  
  
  
    GG(a, b, c, d, x[1], 5, 0xf61e2562);  
    GG(d, a, b, c, x[6], 9, 0xc040b340);  
    GG(c, d, a, b, x[11], 14, 0x265e5a51);  
    GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);  
    GG(a, b, c, d, x[5], 5, 0xd62f105d);  
		
		GG(d, a, b, c, x[10], 9, 0x2441453);  
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);  
    GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);  
    GG(a, b, c, d, x[9], 5, 0x21e1cde6);  
    GG(d, a, b, c, x[14], 9, 0xc33707d6);  
    GG(c, d, a, b, x[3], 14, 0xf4d50d87);  
    GG(b, c, d, a, x[8], 20, 0x455a14ed);  
    GG(a, b, c, d, x[13], 5, 0xa9e3e905);  
    GG(d, a, b, c, x[2], 9, 0xfcefa3f8);  
    GG(c, d, a, b, x[7], 14, 0x676f02d9);  
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a); 
		
		HH(a, b, c, d, x[5], 4, 0xfffa3942);  
    HH(d, a, b, c, x[8], 11, 0x8771f681);  
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);  
    HH(b, c, d, a, x[14], 23, 0xfde5380c);  
    HH(a, b, c, d, x[1], 4, 0xa4beea44);  
    HH(d, a, b, c, x[4], 11, 0x4bdecfa9);  
    HH(c, d, a, b, x[7], 16, 0xf6bb4b60);  
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);  
    HH(a, b, c, d, x[13], 4, 0x289b7ec6);  
    HH(d, a, b, c, x[0], 11, 0xeaa127fa);  
    HH(c, d, a, b, x[3], 16, 0xd4ef3085);  
    HH(b, c, d, a, x[6], 23, 0x4881d05);  
    HH(a, b, c, d, x[9], 4, 0xd9d4d039);  
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);  
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);  
    HH(b, c, d, a, x[2], 23, 0xc4ac5665); 
		
		
		II(a, b, c, d, x[0], 6, 0xf4292244);  
    II(d, a, b, c, x[7], 10, 0x432aff97);  
    II(c, d, a, b, x[14], 15, 0xab9423a7);  
    II(b, c, d, a, x[5], 21, 0xfc93a039);  
    II(a, b, c, d, x[12], 6, 0x655b59c3);  
    II(d, a, b, c, x[3], 10, 0x8f0ccc92);  
    II(c, d, a, b, x[10], 15, 0xffeff47d);  
    II(b, c, d, a, x[1], 21, 0x85845dd1);  
    II(a, b, c, d, x[8], 6, 0x6fa87e4f);  
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);  
    II(c, d, a, b, x[6], 15, 0xa3014314);  
    II(b, c, d, a, x[13], 21, 0x4e0811a1);  
    II(a, b, c, d, x[4], 6, 0xf7537e82);  
    II(d, a, b, c, x[11], 10, 0xbd3af235);  
    II(c, d, a, b, x[2], 15, 0x2ad7d2bb);  
    II(b, c, d, a, x[9], 21, 0xeb86d391);  
    state[0] += a;  
    state[1] += b;  
    state[2] += c;  
    state[3] += d;  
} 




unsigned int A=0x67452301,B=0xefcdab89,C=0x98badcfe,D=0x10325476,a,b,c,d,flen[2],x[16]; 
uint16_t i = 0;
uint16_t j = 0;
uint16_t k = 0;

uint8_t Temp_Ex_Flash[1025];//定义暂时存储1K外部flash内bin文件

void md5(void){                 //MD5主要计算

  a=A,b=B,c=C,d=D;

  FF (a, b, c, d, x[ 0],  7, 0xd76aa478); 
  FF (d, a, b, c, x[ 1], 12, 0xe8c7b756); 
  FF (c, d, a, b, x[ 2], 17, 0x242070db); 
  FF (b, c, d, a, x[ 3], 22, 0xc1bdceee); 
  FF (a, b, c, d, x[ 4],  7, 0xf57c0faf); 
  FF (d, a, b, c, x[ 5], 12, 0x4787c62a); 
  FF (c, d, a, b, x[ 6], 17, 0xa8304613); 
  FF (b, c, d, a, x[ 7], 22, 0xfd469501); 
  FF (a, b, c, d, x[ 8],  7, 0x698098d8); 
  FF (d, a, b, c, x[ 9], 12, 0x8b44f7af); 
  FF (c, d, a, b, x[10], 17, 0xffff5bb1); 
  FF (b, c, d, a, x[11], 22, 0x895cd7be); 
  FF (a, b, c, d, x[12],  7, 0x6b901122); 
  FF (d, a, b, c, x[13], 12, 0xfd987193); 
  FF (c, d, a, b, x[14], 17, 0xa679438e); 
  FF (b, c, d, a, x[15], 22, 0x49b40821); 


  GG (a, b, c, d, x[ 1],  5, 0xf61e2562); 
  GG (d, a, b, c, x[ 6],  9, 0xc040b340); 
  GG (c, d, a, b, x[11], 14, 0x265e5a51); 
  GG (b, c, d, a, x[ 0], 20, 0xe9b6c7aa); 
  GG (a, b, c, d, x[ 5],  5, 0xd62f105d); 
  GG (d, a, b, c, x[10],  9, 0x02441453); 
  GG (c, d, a, b, x[15], 14, 0xd8a1e681); 
  GG (b, c, d, a, x[ 4], 20, 0xe7d3fbc8); 
  GG (a, b, c, d, x[ 9],  5, 0x21e1cde6); 
  GG (d, a, b, c, x[14],  9, 0xc33707d6); 
  GG (c, d, a, b, x[ 3], 14, 0xf4d50d87); 
  GG (b, c, d, a, x[ 8], 20, 0x455a14ed); 
  GG (a, b, c, d, x[13],  5, 0xa9e3e905); 
  GG (d, a, b, c, x[ 2],  9, 0xfcefa3f8); 
  GG (c, d, a, b, x[ 7], 14, 0x676f02d9); 
  GG (b, c, d, a, x[12], 20, 0x8d2a4c8a); 


  HH (a, b, c, d, x[ 5],  4, 0xfffa3942); 
  HH (d, a, b, c, x[ 8], 11, 0x8771f681); 
  HH (c, d, a, b, x[11], 16, 0x6d9d6122); 
  HH (b, c, d, a, x[14], 23, 0xfde5380c); 
  HH (a, b, c, d, x[ 1],  4, 0xa4beea44); 
  HH (d, a, b, c, x[ 4], 11, 0x4bdecfa9); 
  HH (c, d, a, b, x[ 7], 16, 0xf6bb4b60); 
  HH (b, c, d, a, x[10], 23, 0xbebfbc70); 
  HH (a, b, c, d, x[13],  4, 0x289b7ec6); 
  HH (d, a, b, c, x[ 0], 11, 0xeaa127fa); 
  HH (c, d, a, b, x[ 3], 16, 0xd4ef3085); 
  HH (b, c, d, a, x[ 6], 23, 0x04881d05); 
  HH (a, b, c, d, x[ 9],  4, 0xd9d4d039); 
  HH (d, a, b, c, x[12], 11, 0xe6db99e5); 
  HH (c, d, a, b, x[15], 16, 0x1fa27cf8); 
  HH (b, c, d, a, x[ 2], 23, 0xc4ac5665); 


  II (a, b, c, d, x[ 0],  6, 0xf4292244); 
  II (d, a, b, c, x[ 7], 10, 0x432aff97); 
  II (c, d, a, b, x[14], 15, 0xab9423a7); 
  II (b, c, d, a, x[ 5], 21, 0xfc93a039); 
  II (a, b, c, d, x[12],  6, 0x655b59c3); 
  II (d, a, b, c, x[ 3], 10, 0x8f0ccc92); 
  II (c, d, a, b, x[10], 15, 0xffeff47d); 
  II (b, c, d, a, x[ 1], 21, 0x85845dd1); 
  II (a, b, c, d, x[ 8],  6, 0x6fa87e4f); 
  II (d, a, b, c, x[15], 10, 0xfe2ce6e0); 
  II (c, d, a, b, x[ 6], 15, 0xa3014314); 
  II (b, c, d, a, x[13], 21, 0x4e0811a1); 
  II (a, b, c, d, x[ 4],  6, 0xf7537e82); 
  II (d, a, b, c, x[11], 10, 0xbd3af235); 
  II (c, d, a, b, x[ 2], 15, 0x2ad7d2bb); 
  II (b, c, d, a, x[ 9], 21, 0xeb86d391); 

  A += a;
  B += b;
  C += c;
  D += d;

}

unsigned int sample_index = 0; //从TEST_data采集64位个字节
unsigned int data_index = 0;   //bin文件字符索引,固件bin文件大小不要超过65K
unsigned int read_times = 0;
unsigned int BIN_location = 0;

void data_reset()
{
	sample_index = 0;
	data_index = 0;
	read_times = 0;
	BIN_location = 0;
	A=0x67452301;
	B=0xefcdab89;
	C=0x98badcfe;
	D=0x10325476;
	a=0;
	b=0;
	c=0;
	d=0;
	rt_memset(flen,0x00,sizeof(flen));
	rt_memset(x,0x00,sizeof(x));
}

void read_group_tempbuf(unsigned long fileSize)
{
	rt_memset(x,0,64);	
	sample_index = 0;
	for(j = 0;j < 16;j++)//fread(&x,4,16,fp) 以4字节为一组，共16组往x写入数据
	{
		for(k = 0;k < 4;k++)
		{
			if((read_times >= fileSize/1024)&&(data_index >= fileSize%1024)) break;//当对最后一部分小于1K的bin文件处理
			
			((char*)x)[sample_index] = Temp_Ex_Flash[data_index];
			data_index++;
			sample_index++;
		}
	}
}



void get_bin_md5(uint32_t startAddr,unsigned long fileSize,char result[33])
{
	unsigned short i = 0;
	
	data_reset();
	BIN_location=startAddr;
	
	for(read_times = 0;read_times < fileSize/1024;read_times++)//对整K进行md5计算
	{
		Flash_Read8BitDatas(BIN_location,1024,(int8_t *)&Temp_Ex_Flash)	;
		BIN_location += 1024;
		
		for(i = 0;i < 16;i++)//刚好对读出的1Kbin文件进行md5计算 
		{
			read_group_tempbuf(fileSize);
			md5();	
		}
		data_index = 0;//最大只到1023
	}
	memset(Temp_Ex_Flash,0,1025);
	Flash_Read8BitDatas(BIN_location,fileSize%1024,(int8_t *)&Temp_Ex_Flash)	;
	
	
	read_group_tempbuf(fileSize);
	for(i = 0;i < (fileSize%1024)/64;i++)
	{
		md5();
		read_group_tempbuf(fileSize);
	}
	((char*)x)[fileSize%64]=128;//文件结束补1,0操作 10000000
	
	flen[1]=fileSize/0x20000000;   //转换二进制文件大小（byte->bit）
	flen[0]=(fileSize%0x20000000)*8;
	
	if(fileSize%64>55) md5(),memset(x,0,64);
	memcpy(x+14,flen,8);
	md5();
	sprintf(result,"%08X%08X%08X%08X",PP(A),PP(B),PP(C),PP(D));
	
}















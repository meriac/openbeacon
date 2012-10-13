#include <stdio.h>
#include <stdint.h>

#define FILTER_SIZE 5
#define MIDDLE 127
#define NOISE 2

int main( int argc, const char* argv[] )
{
	int c,pos,count,i;
	uint8_t buffer[FILTER_SIZE];

	pos =0;
	while((c=getchar())>=0)
	{
		if((c<=(MIDDLE+NOISE)) && (c>=(MIDDLE-NOISE)))
		{
			count++;
			buffer[pos++]=(uint8_t)c;
			if(pos>=FILTER_SIZE)
				pos=0;
		}
		else
		{
			if(count<FILTER_SIZE)
				pos=0;
			else
			{
				i=(count-FILTER_SIZE)*2;
				while(i--)
					putchar(MIDDLE);
				count=FILTER_SIZE;
			}

			while(count)
			{
				count--;
				putchar(buffer[pos++]);
				if(pos>=FILTER_SIZE)
					pos=0;
			}
			putchar(c);
			count=0;
		}
	}
	return 0;
}

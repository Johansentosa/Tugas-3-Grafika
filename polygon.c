#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

typedef struct{
	int x;
	int y;
} point;

struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
int fb_fd;
long screensize;
uint8_t *fbp;

//algo solid fill tapi warna ny masi biru doang
//gw loop sampe dia ketemu warna bukan item di sblhny, jadi harus ad batesannya
//input firepoint awal
void solidFill(point firepoint){
	point newfp;
	
	newfp.x = firepoint.x+1;
	newfp.y = firepoint.y;
	long location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
	if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
		//ganti jadi rgb yang di mau buat gnti warna
		*(fbp + location) =255;
		*(fbp + location +1) = 0;
		*(fbp + location +2) = 0;
		*(fbp + location + 3) = 50;
		solidFill(newfp);
	}
	
	newfp.x = firepoint.x-1;
	newfp.y = firepoint.y;
	location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
	if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
		*(fbp + location) =255;
		*(fbp + location +1) = 0;
		*(fbp + location +2) = 0;
		*(fbp + location + 3) = 50;
		solidFill(newfp);
	}
	
	newfp.x = firepoint.x;
	newfp.y = firepoint.y+1;
	location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
	if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
		*(fbp + location) =255;
		*(fbp + location +1) = 0;
		*(fbp + location +2) = 0;
		*(fbp + location + 3) = 50;
		solidFill(newfp);
	}
	
	newfp.x = firepoint.x;
	newfp.y = firepoint.y-1;
	location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
	if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
		*(fbp + location) =255;
		*(fbp + location +1) = 0;
		*(fbp + location +2) = 0;
		*(fbp + location + 3) = 50;
		solidFill(newfp);
	}
}

//gambar garis turun start.y < finish.y 100 50, 150 100
void drawLine(point start, point finish, int deltax, int deltay, int p){
	int p1;
	if(start.y == finish.y){
		if(start.x<=finish.x){
			//printf("babx\n");
			while(start.x <= finish.x){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				//tempat bug
				//printf("%d-%d\n",start.x,finish.x);
				*(fbp + location) = 255;
				//printf("%d-%d\n",start.x,start.y);
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 50;
				start.x++;
			}
		}
	}
	else if(start.x == finish.x){
		if(start.y<=finish.y){
			while(start.y <= finish.y){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = 255;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 50;
				start.y++;
			}
		}
	}
	else{
		long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
		*(fbp + location) = 255;
		*(fbp + location +1) = 0;
		*(fbp + location +2) = 0;
		*(fbp + location + 3) = 50;
		if(p>=0){
			start.x++;
			start.y++;
			p1 = p + 2*deltay - 2*deltax;
		}
		else{
			start.x++;
			p1 = p + 2*deltay;
		}
		drawLine(start,finish,deltax,deltay,p1);
	}
}

//gambar garis naik start.y > finish.y
void drawLine2(point start, point finish, int deltax, int deltay, int p){
	int p1;
	if(start.y == finish.y){
		if(start.x<=finish.x){
			while(start.x <= finish.x){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = 255;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 50;
				start.x++;
			}
		}
	}
	else if(start.x == finish.x){
		if(start.y>=finish.y){
			while(start.y >= finish.y){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = 255;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 50;
				start.y--;
			}
		}
	}
	else{
		long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
		*(fbp + location) = 255;
		*(fbp + location +1) = 0;
		*(fbp + location +2) = 0;
		*(fbp + location + 3) = 50;
		if(p>=0){
			start.x++;
			start.y--;
			p1 = p + 2*deltay - 2*deltax;
		}
		else{
			start.x++;
			p1 = p + 2*deltay;
		}
		drawLine2(start,finish,deltax,deltay,p1);
	}
}

int main()
{
	fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	screensize = vinfo.yres_virtual * finfo.line_length;

	fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
	
	int x,y;
	
	for (y=0;y<vinfo.yres;y++)
	{
		for (x=0;x<vinfo.xres;x++){
			long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 0;
			*(fbp + location +1) = 0;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 50;
		}
	}

	//array isinya titik2 polygon
	int size = 11;
	
	point p[size];
	int k = 80;
	p[0].x = 25+k;
	p[0].y = 50+k;
	
	p[1].x = 50+k;
	p[1].y = 50+k;
	
	p[2].x = 50+k;
	p[2].y = 25+k;
	
	p[3].x = 75+k;
	p[3].y = 50+k;
	
	p[4].x = 125+k;
	p[4].y = 50+k;
	
	p[5].x = 150+k;
	p[5].y = 75+k;
	
	p[6].x = 75+k;
	p[6].y = 75+k;
	
	p[7].x = 50+k;
	p[7].y = 100+k;
	
	p[8].x = 50+k;
	p[8].y = 75+k;
	
	p[9].x = 25+k;
	p[9].y = 75+k;
	
	p[10].x = 25+k;
	p[10].y = 50+k;
	
	point fp;
	fp.x = 75+k;
	fp.y = 60+k;
	
	
	//n faktor penambah buat bikin animasi
	for(int n=0; n<400; n+=20){
		point temp;
		temp.x = fp.x;
		temp.y = fp.y;
		temp.x+=n;
		for(int i=0; i<size-1; i++){
			point start, finish;
			//kalo start x nya dikanan, di swap dlu
			if(p[i].x>p[i+1].x){
				start = p[i+1];
				finish = p[i];
			}
			else{
				start = p[i];
				finish = p[i+1];
			}
			
			start.x+=n;
			finish.x+=n;
			
			//rumus bersenham itu
			int deltay = finish.y - start.y;
			int deltax = finish.x - start.x;
			int px = 2*deltay -deltax;
			
			//kalo start y > finish y pake drawline2
			if(deltay < 0){
				deltay = deltay * -1;
				drawLine2(start,finish,deltax,deltay,px);
			}
			else{
				drawLine(start,finish,deltax,deltay,px);
			}
		}
		solidFill(temp);
		sleep(1);
		for (y=0;y<vinfo.yres;y++)
		{
			for (x=0;x<vinfo.xres;x++){
				long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = 0;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 50;
			}
		}
	}
	
	return 0;
}

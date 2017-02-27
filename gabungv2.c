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


//bikin backgroundnya item semua
struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo *vinfo);
inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo *vinfo)
{
    return (r<<vinfo->red.offset) | (g<<vinfo->green.offset) | (b<<vinfo->blue.offset);
}

//gambar garis turun start.y < finish.y 100 50, 150 100
void drawLine(point start, point finish, int deltax, int deltay, int p){
	 

	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);


	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

	int p1;
	while( (start.x != finish.x) || (start.y != finish.y)){
		if(start.y == finish.y){
		while(start.x < finish.x){
			long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 255;
			*(fbp + location +1) = 0;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 0;
			start.x++;
		}
		}
		else if(start.x == finish.x){
			while(start.y < finish.y){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = 255;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 0;
				start.y++;
			}
		}
		else{
			long long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 255;
			*(fbp + location +1) = 0;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 0;

			if(p>=0){
				start.x++;
				start.y++;
				p1 = p + 2*deltay - 2*deltax;
			}
			else{
				start.x++;
				p1 = p + 2*deltay;
			}
		}
	}
	
}

void drawLinex(point start, point finish, int deltax, int deltay, int p){
	 

	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);


	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

	int p1;
	while( (start.x != finish.x) || (start.y != finish.y)){
		if(start.y == finish.y){
		while(start.x < finish.x){
			long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 0;
			*(fbp + location +1) = 255;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 50;
			start.x++;
		}
		}
		else if(start.x == finish.x){
			while(start.y < finish.y){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) = 0;
				*(fbp + location +1) = 255;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 50;
				start.y++;
			}
		}
		else{
			long long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 0;
			*(fbp + location +1) = 255;
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
		}
	}
	
}


//gambar garis naik start.y > finish.y
void drawLine2(point start, point finish, int deltax, int deltay, int p){
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
	
	int p1;
	if(start.y == finish.y){
		while(start.x <= finish.x){
			long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 255;
			*(fbp + location +1) = 0;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 50;
			start.x++;
		}
	}
	else if(start.x == finish.x){
		while(start.y >= finish.y){
			long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 255;
			*(fbp + location +1) = 0;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 50;
			start.y--;
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

void drawLine2x(point start, point finish, int deltax, int deltay, int p){
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
	
	int p1;
	if(start.y == finish.y){
		while(start.x <= finish.x){
			long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 0;
			*(fbp + location +1) = 255;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 50;
			start.x++;
		}
	}
	else if(start.x == finish.x){
		while(start.y >= finish.y){
			long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 0;
			*(fbp + location +1) = 255;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 50;
			start.y--;
		}
	}
	else{
		long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
		*(fbp + location) = 0;
		*(fbp + location +1) = 255;
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
		drawLine2x(start,finish,deltax,deltay,p1);
	}
}

/****************************************************************************
						FROM PELURU BEAM
***************************************************************************/
void cetakHitam(int x, int y){
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
    long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
	*(fbp + location) = 0;
	*(fbp + location +1) = 0;
	*(fbp + location +2) = 0;
	*(fbp + location + 3) = 0;
}

void cetakPutih(int x, int y){
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
    long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
	*(fbp + location) = 255;
	*(fbp + location +1) = 255;
	*(fbp + location +2) = 255;
	*(fbp + location + 3) = 0;
}

void cetakBlank(){
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);

	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
    int x,y;
	for (y=0;y<vinfo.yres;y++)
	{
		for (x=0;x<vinfo.xres;x++){
			long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
			*(fbp + location) = 0;
			*(fbp + location +1) = 0;
			*(fbp + location +2) = 0;
			*(fbp + location + 3) = 0;
		}
	}
}

void drawBeam(point start, point finish, int deltax, int deltay, int p){	
	int p1;
	if(start.y == finish.y) { // garis ke kiri - BELUM BISA
		while(start.x <= finish.x) {
			cetakPutih(start.x, start.y);
			start.x += 10;
		}
	}
	else if(start.x == finish.x) { // garis lurus keatas
		while(start.y >= finish.y) {
			// Beam-like shooter - not using draw_circle()
			int savey = start.y;
			for(int i = 0; i < 10 && start.y>=finish.y ; i++) { // cetak 10 pixel biar seperti beam
				cetakPutih(start.x, start.y);
				start.y -= 1;
			}

			usleep(5000);
			for(int i = 0; i < 10 && savey>=finish.y ; i++) { // timpa beam yang sudah dicetak dengan hitam
				cetakHitam(start.x, savey);
				savey -= 1;
			}

		}
		
	}
	else {
		if(p>=0) { // garis miring gradien positif
			// Beam-like shooter - not using draw_circle()
			int savex = start.x;
			int savey = start.y;
			for(int i = 0; i < 10; i++) { // cetak 10 pixel biar seperti beam
				cetakPutih(start.x, start.y);
				start.x += 1;
				start.y -= 1;
			}
			usleep(5000);
			for(int i = 0; i < 10; i++) { // timpa beam yang sudah dicetak dengan hitam

				cetakHitam(savex, savey);
				savex += 1;
				savey -= 1;
			}
			p1 = p + 2*deltay - 2*deltax;
		}
		else { // ini ngga tau buat apa, ngga pernah masuk sini
			start.x++;
			p1 = p + 2*deltay;
		}
		drawBeam(start,finish,deltax,deltay,p1);
	}
}



void solidFill(point firepoint){
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	int fb_fd = open("/dev/fb0",O_RDWR);
	point newfp;
	
	//Get variable screen information
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	long screensize = vinfo.yres_virtual * finfo.line_length;

	uint8_t *fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
	
	newfp.x = firepoint.x+1;
	newfp.y = firepoint.y;
	long location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
	if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
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
}

int main()
{
	int x,y;

	cetakBlank();

	//array isinya titik2 polygon
	int size = 11;
	
	point p[size];
	int k = 180;
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
	fp.x = 75;
	fp.y = 60;
	
	//n faktor penambah buat bikin animasi
	for(int n=0; n<120; n+=20){
		cetakBlank();
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
		if(n+20 < 120)
			usleep(50000);

		
	}	
	//solidFill(fp);
	/**/

	/********************************************
				Draw Beam
	*******************************************/
	point p1[2];

	// Lokasi tembakan meriam
	p1[0].x = 350;
	p1[0].y = 500;
	
	//Lokasi target tembakan	
	p1[1].x = 350;
	p1[1].y = 250;
	
	
	/* Rumus Bresenham */
	int deltay = p1[1].y - p1[0].y;
	if(deltay < 0){
		deltay = deltay * -1;
	}
	int deltax = p1[1].x - p1[0].x;
	int px = 2*deltay -deltax;

	drawBeam(p1[0],p1[1],deltax,deltay,px);
	
	usleep(10000);
	

	/****************************************
				Draw Crash
	*****************************************/
	
	point b[9];
	//int k = 180;
	b[0].x = 170+k;
	b[0].y = 20+k;

	b[1].x = 180+k;
	b[1].y = 60+k;
	
	b[2].x = 220+k;
	b[2].y = 70+k;

	b[3].x = 180+k;
	b[3].y = 80+k;
	
	b[4].x = 170+k;
	b[4].y = 120+k;

	b[5].x = 160+k;
	b[5].y = 80+k;	

	b[6].x = 120+k;
	b[6].y = 70+k;

	b[7].x = 160+k;
	b[7].y = 60+k;	

	b[8].x = 170+k;
	b[8].y = 20+k;

	size = 9;
	

		for(int i=0; i<size-1; i++){
			point start, finish;
			//kalo start x nya dikanan, di swap dlu
			if(b[i].x>b[i+1].x){
				start = b[i+1];
				finish = b[i];
			}
			else{
				start = b[i];
				finish = b[i+1];
			}
			
			//rumus bersenham itu
			int deltay = finish.y - start.y;
			int deltax = finish.x - start.x;
			int px = 2*deltay -deltax;
			//kalo start y > finish y pake drawline2
			if(deltay < 0){
				deltay = deltay * -1;
				
				drawLine2x(start,finish,deltax,deltay,px);
			}
			else{
				drawLinex(start,finish,deltax,deltay,px);
			}
		}
	usleep(300000);
	cetakBlank();
	return 0;
}

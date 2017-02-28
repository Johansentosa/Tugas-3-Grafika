#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <math.h>
#include "event.c"

typedef struct Points {
    int x;
    int y;
} Point;
typedef struct Colors {
    int a;
    int r;
    int g;
    int b;
} Color;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

Color bg; // warna background
char* fbp; // memory map ke fb0
int fbfd; // file fb0
int screensize; // length nya si fbp
Point * window_center;
int bytePerPixel;
int overPixel = 10;
Point * window;
Color c2;

int i;
int window_width = 100;
int window_height = 100;

void setPoint(Point* p, int x, int y) {
    p -> x = x;
    p -> y = y;
}

void swapPoint(Point* p1, Point* p2) {
    int x = p1 -> x;
    int y = p1 -> y;
    setPoint(p1, p2 -> x, p2 -> y);
    setPoint(p2, x, y);
}

void setColor(Color* c, char r, char g, char b) {
    c -> a = 0;
    c -> r = r;
    c -> g = g;
    c -> b = b;
}

void changeARGB(int location, Color* c) {
    *(fbp + location) = c -> a;
    *(fbp + location + 1) = c -> r;
    *(fbp + location + 2) = c -> g;
    *(fbp + location + 3) = c -> b;
}

void clearScreen(Color* c) {
    int i, j;
    // iterasi pixel setiap baris dan setiap kolom
    for (j = 0; j < vinfo.yres; j++) {
        for (i = 0; i < vinfo.xres; i++) {
			long location = (i+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (j+vinfo.yoffset) * finfo.line_length;
            changeARGB(location, c);
            //location += bytePerPixel; // pixel sebelah kanannya berjarak sekian byte
        }
        //location += (bytePerPixel * overPixel); // pixel pertama pada baris selanjutnya berjarak sekian byte
    }
}

void drawLineX(Point* p1, Point* p2, Color* c, int positif) {
	// algoritma Bresenham
    int dx = p2 -> x - p1 -> x;
    int dy = (p2 -> y - p1 -> y) * positif;
    int d = dy + dy - dx;
    int j = p1 -> y;
    int i, location;
    // lebih panjang horizontal, maka iterasi berdasarkan x
    for (i = p1 -> x; i <= p2 -> x; i++) {
		if(i<0 || i>vinfo.xres-1 || j<0 || j>vinfo.yres-1){
		}
		else{
			if(i<=window[0].x || i>=window[2].x || j<=window[0].y || j>=window[2].y){
				location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
				changeARGB(location, c);
			}
			else{
				Color * temp = &c2;
				location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
				changeARGB(location, temp);
			}
		}
        if (d > 0) {
            // positif = 1 berarti y makin lama makin bertambah
            j += positif;
            d -= dx;
        }
        d += dy;
    }
}

void drawLineY(Point* p1, Point* p2, Color* c, int positif) {
	// algoritma Bresenham
    int dx = (p2 -> x - p1 -> x) * positif;
    int dy = p2 -> y - p1 -> y;
    int d = dx + dx - dy;
    int i = p1 -> x;
    int j, location;
    // lebih panjang vertikal, maka iterasi berdasarkan y
    for (j = p1 -> y; j <= p2 -> y; j++) {
		if(i<0 || i>vinfo.xres-1 || j<0 || j>vinfo.yres-1){
		}
		else{
			if(i<=window[0].x || i>=window[2].x || j<=window[0].y || j>=window[2].y){
				location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
				changeARGB(location, c);
			}
			else{
				Color * temp = &c2;
				location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
				changeARGB(location, temp);
			}
		}
        if (d > 0) {
            // positif = 1 berarti x makin lama makin bertambah
            i += positif;
            d -= dy;
        }
        d += dx;
    }
}

int getOctant (int dx, int dy) {
    /*  peta octant:
        \2|1/
        3\|/0
        --+--
        4/|\7
        /5|6\
    */

    if (dy >= 0 && dy < dx) {
        return 0;
    } else if (dx > 0 && dx <= dy) {
        return 1;
    } else if (dx <= 0 && dx > -dy) {
        return 2;
    } else if (dy > 0 && dy <= -dx) {
        return 3;
    } else if (dy <= 0 && dy > dx) {
        return 4;
    } else if (dx < 0 && dx >= dy) {
        return 5;
    } else if (dx >= 0 && dx < -dy) {
        return 6;
    } else { // dy < 0 && dy >= -dx
        return 7;
    }
}

void drawLine(Point* p1, Point* p2, Color* c) {
    int dx = p2 -> x - p1 -> x;
    int dy = p2 -> y - p1 -> y;
    int octant = getOctant(dx, dy);
    // algoritma bresenham sebenernya hanya bisa menggambar di octant 0,
    // atur sedemikian rupa supaya 7 octant lainnya bisa masuk ke algoritma
    switch (octant) {
        case 0: drawLineX(p1, p2, c, 1); break;
        case 1: drawLineY(p1, p2, c, 1); break;
        case 2: drawLineY(p1, p2, c, -1); break;
        case 3: swapPoint(p1, p2); drawLineX(p1, p2, c, -1); swapPoint(p1, p2); break;
        case 4: swapPoint(p1, p2); drawLineX(p1, p2, c, 1); swapPoint(p1, p2); break;
        case 5: swapPoint(p1, p2); drawLineY(p1, p2, c, 1); swapPoint(p1, p2); break;
        case 6: swapPoint(p1, p2); drawLineY(p1, p2, c, -1); swapPoint(p1, p2); break;
        case 7: drawLineX(p1, p2, c, -1); break;
    }
}

void connectBuffer() {
    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }
    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }
    // Figure out the size of the screen in bytes
    bytePerPixel = vinfo.bits_per_pixel / 8;
    screensize = (vinfo.xres + overPixel) * vinfo.yres * bytePerPixel;
    // Map the device to memory
    fbp = (char*) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (*fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
}

Point * initWindow(Point * window_center){
	Point * window = (Point*) malloc(5 * sizeof(Point));
	int x = window_center->x;
	int y = window_center->y;
	setPoint(&window[0], x-window_width/2, y-window_height/2);
	setPoint(&window[1], x+window_width/2, y-window_height/2);
	setPoint(&window[2], x+window_width/2, y+window_height/2);
	setPoint(&window[3], x-window_width/2, y+window_height/2);
	setPoint(&window[4], x-window_width/2, y-window_height/2);
	return window;
}

void zoom(float zoom, Point * zoomPoint, Point * plane, int i){
	Point * temp;
	Point * zoomtemp;
	zoomtemp = (Point*) malloc(1 * sizeof(Point));
	zoomtemp[0].x = zoomPoint->x * zoom;
	zoomtemp[0].y = zoomPoint->y * zoom;
	int deltax = zoomtemp[0].x - window_center->x;
	deltax=deltax*-1;
	int deltay = zoomtemp[0].y - window_center->y;
	deltay=deltay*-1;
	temp = (Point*) malloc(i * sizeof(Point));
	int j;
	for(j = 0; j < i; j++) {
		temp[j].x = (plane[j].x * zoom) + deltax;
		temp[j].y = (plane[j].y * zoom) + deltay;
		printf("%d\n",temp[j].x);
		printf("%d\n",temp[j].y);
	}
	
	Color c;
	setColor(&c, 255, 0, 0);
	int k=0;
	for(k = 0; k < zoom; k++) {
		for(j = 0; j < i-1; j++) {
			drawLine(&temp[j], &temp[j + 1], &c);
			temp[j].y++;
		}
	}
	free(temp);
	
}

int main() {
    setColor(&bg, 0, 0, 255);
    connectBuffer();
    clearScreen(&bg);
    Color c, cDel;
    setColor(&c, 255, 0, 0);
    setColor(&c2, 0, 255, 255);
    int j;
    i = 10;
    Point* plane;
    plane = (Point*) malloc(i * sizeof(Point));
    
    //BAGIAN window
    Point * zoomPoint = (Point*) malloc(1 * sizeof(Point));
    Point * zoom2 = (Point*) malloc(4 * sizeof(Point));
    window_center = (Point*) malloc(1 * sizeof(Point));
	setPoint(&window_center[0], 400, 400);
	setPoint(&zoomPoint[0], 200, 200);
	window = initWindow(window_center);
	zoom2 = initWindow(zoomPoint);
	
	
    int n = 0;
    setPoint(&plane[0], n, 90+n);
	setPoint(&plane[1], 40+n, 50+n);
	setPoint(&plane[2], 190+n, 50+n);
	setPoint(&plane[3], 220+n, 20+n);
	setPoint(&plane[4], 260+n, 20+n);
	setPoint(&plane[5], 190+n, 90+n);
	setPoint(&plane[6], 260+n, 150+n);
	setPoint(&plane[7], 300+n, 200+n);
	setPoint(&plane[8], 80+n, 200+n);
	setPoint(&plane[9], 100+n, 70+n);
	// gambar
	for(j = 0; j < i-1; j++) {
		drawLine(&plane[j], &plane[j + 1], &c);
	}
	
	//drawLine(&window[0], &window[2], &c);
	for(j = 0; j < 4; j++) {
		drawLine(&window[j], &window[j + 1], &c);
	}
	
	for(j = 0; j < 4; j++) {
		drawLine(&zoom2[j], &zoom2[j + 1], &c);
	}
	sleep(2);
	clearScreen(&bg);
	zoom(0.25,zoomPoint,plane,i);
	//drawLine(&window[0], &window[2], &c);
	for(j = 0; j < 4; j++) {
		drawLine(&window[j], &window[j + 1], &c);
	}
	sleep(2);
	//drawLine(&window_center[0], &window_center[1], &c);
	//clearScreen(&bg);
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

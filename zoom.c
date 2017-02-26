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

struct readFallDownParams {
	Point* p;
	Point firepoint;
	Color c;
};

struct readFallSpinParams {
	Point* p;
	Point firepoint;
	Color c;
	Point pivot;
};

Color bg, cBorder; // warna background
char* fbp; // memory map ke fb0
int fbfd; // file fb0
int screensize; // length nya si fbp
int overPixel = 10; // intinya satu baris ada 1376 pixel = 1366 pixel + 10 pixel "ngumpet"
int bytePerPixel;
int xSlide = 0;
int ySlide = 0;
double scale = 100.0;
int zoomin = 0;
int zoomout = 0;
char stroke;

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
	printf("drawLineX\n");
    int dx = p2 -> x - p1 -> x;
    int dy = (p2 -> y - p1 -> y) * positif;
    int d = dy + dy - dx;
    int j = p1 -> y;
    int i, location;
    // lebih panjang horizontal, maka iterasi berdasarkan x
    for (i = p1 -> x; i <= p2 -> x; i++) {
        if (i> 0 && i < vinfo.xres && j > 0 && j < vinfo.yres) {
            location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
            changeARGB(location, c);
            if (d > 0) {
                // positif = 1 berarti y makin lama makin bertambah
                j += positif;
                d -= dx;
            }
            d += dy;
        }
    }
}

void drawLineY(Point* p1, Point* p2, Color* c, int positif) {
	// algoritma Bresenham
	printf("drawLineY\n");
    int dx = (p2 -> x - p1 -> x) * positif;
    int dy = p2 -> y - p1 -> y;
    int d = dx + dx - dy;
    int i = p1 -> x;
    int j, location;
    // lebih panjang vertikal, maka iterasi berdasarkan y
    for (j = p1 -> y; j <= p2 -> y; j++) {
        if (j > 0 && j < vinfo.yres && i > 0 && i < vinfo.xres) {
            location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
            changeARGB(location, c);
            if (d > 0) {
                // positif = 1 berarti x makin lama makin bertambah
                i += positif;
                d -= dy;
            }
            d += dx;
        }
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

void setPointScale(Point *p, double scale) {
	Point pNew;
  	pNew.x = ceil((scale/100)*p->x+xSlide);
  	pNew.y = ceil((scale/100)*p->y+ySlide);
  	setPoint(p, pNew.x, pNew.y);
  	printf("%d %d\n", pNew.x, pNew.y);
}

void zoom(Point*p, int numPoints, double scale) {
    Color c; setColor(&c, 255, 255, 0);
    int i;
	for (i = 0; i<numPoints; i++) {
		setPointScale(&p[i], scale);
	}
	clearScreen(&bg);
	for (i = 0; i <numPoints-1; i++) {
		drawLine(&p[i], &p[i+1], &c);
	}
	drawLine(&p[numPoints-1], &p[0], &c);
	zoomin = 0;
	zoomout = 0;
}

void* drawBox() {
	Color c; setColor(&c, 255, 255, 0);
    int i;
    Point* p;
    p = (Point*) malloc(6*sizeof(Point));
    setPoint(&p[0], 300, 400);
    setPoint(&p[1], 400, 400);
    setPoint(&p[2], 400, 600);
    setPoint(&p[3], 300, 600);
    for (i = 0; i <3; i++) {
		drawLine(&p[i], &p[i+1], &c);
	}
	drawLine(&p[3], &p[0], &c);
    
    while (1) {
		if (zoomin || zoomout) {
			zoom(p, 4, scale);
		}
	}
}

void* captureKey() {
	int i;
  	while (1) {
  		stroke = getch();
    	if (stroke ==  'i') {
    		scale += 10;
    		zoomin = 1;
    	} else
    	if (stroke == 'o') {
			scale -= 10;
    		zoomout = 1;
    	}
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

int main(){
	Color bg;
	setColor(&bg, 0, 0, 0);
    connectBuffer();
	pthread_t thrBox, thrKey;
    pthread_create(&thrBox, NULL, drawBox, "thrBox");
    pthread_create(&thrKey, NULL, captureKey, "thrKey");
    pthread_join(thrBox, NULL);
    pthread_join(thrKey, NULL);
    return 0;
}

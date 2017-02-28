#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <math.h>
// #include "event.c"

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

typedef struct lines {
	Point* p1;
	Point* p2;
} Line;

typedef struct clipWindows {
	double l; //batas kiri window, sb. x
	double r; //batas kanan window, sb.x
	double t; //batas atas window, sb. y
	double b; //batas bawah window. sb. y
} Clipwindow;

typedef struct pointCodes {
	//buat posisi garis thd clipwindow, didalem diluar apa terpotong clipwindow
	//nilai 1 dan 0
	int l;
	int r;
	int t;
	int b;
} Pointcode;

typedef struct Buildings {

    int neff;
    Point * P;
} Building;

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



Color bg; // warna background
char* fbp; // memory map ke fb0
int fbfd; // file fb0
int screensize; // length nya si fbp
Point * window_center;
int bytePerPixel;
int overPixel = 10;
Point * window;
Color c2;
int nBuilding = 0;
Building * building;

int i;
int window_width = 200;
int window_height = 200;

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

void loadBuildings4() {
    printf("TES");
    char c;
    int valx, valy;
    Point pTemp;
    FILE *file;
    file = fopen("buildings.txt", "r");
    int i, j;
    building = malloc(500 * sizeof(Building));
    for (i = 0; i < 500; i++) {
        building[i].P = (Point *) malloc(500 * sizeof(Point));
    }
    int itbuilding;
    int itpoint;
    i = 0;

    while (!feof(file)) {
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='#');
        j = 0;
        if (!feof(file)) {
            // printf("building %d\n", i);
            while (c = getc(file) != '#'){
                fscanf (file, "%d", &valx);
                fscanf (file, "%d", &valy);
                //printf("%d %d\n",valx , valy);
                setPoint(&building[i].P[j], valx, valy);
                //printf("%d %d\n", building[i].P[j].x, building[i].P[j].y);
                // building.push_back(pTemp);
                j++;
            }
            building[i].neff = j;
            // printf("%d\n", building[i].neff);
            i++;
        }
    };
    nBuilding = i;
    fclose(file);
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


void zoom(int zoom, Point * zoomPoint, Point * plane, int i){
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

    window_center = (Point*) malloc(1 * sizeof(Point));
	setPoint(&window_center[0], 650, 200);
	window = initWindow(window_center);
    clearScreen(&bg);
    int i,j;
    char ch;
    loadBuildings4();
    while(1){
		clearScreen(&bg);
		//draw map
		for(i = 0; i < nBuilding; i++) {
			for(j = 0; j < building[i].neff-1; j++) {
				drawLine(&building[i].P[j], &building[i].P[j+1], &c);
			}
			drawLine(&building[i].P[j], &building[i].P[0], &c);
		}
		for(j = 0; j < 4; j++) {
			drawLine(&window[j], &window[j + 1], &c);
		}
		scanf("%c",&ch);
	}
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

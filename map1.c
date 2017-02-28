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

typedef struct Trees {
	int neff;
	Point *P;
} Tree;

typedef struct Streets {
	int neff;
	Point *P;
} Street;

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
int nBuilding2 = 0;
int nTree = 0;
int nStreet = 0;
Building * building;
Building * tree;
Building * street;

int i;
int window_width = 200;
int window_height = 200;

int zoom_width = 100;
int zoom_height = 100;

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
    *(fbp + location) = c -> r;
    *(fbp + location + 1) = c -> g;
    *(fbp + location + 2) = c -> b;
    *(fbp + location + 3) = c -> a;
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
    //printf("TES");
    char c;
    int valx, valy;
    Point pTemp;
    FILE *file;
    file = fopen("buildings.txt", "r");
    int i, j;
    building = malloc(200 * sizeof(Building));
    for (i = 0; i < 200; i++) {
        building[i].P = (Point *) malloc(50 * sizeof(Point));
    }
    int itbuilding;
    int itpoint;
    i = 0;

    while (!feof(file)) {
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='#');
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='\n');
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
    nBuilding2 = i;
    fclose(file);
}
void loadTree() {
    //printf("TES");
    char c;
    int valx, valy;
    Point pTemp;
    FILE *file;
    file = fopen("pohon.txt", "r");
    int i, j;
    tree = malloc(200 * sizeof(Tree));
    for (i = 0; i < 200; i++) {
        tree[i].P = (Point *) malloc(50 * sizeof(Point));
    }
    int itbuilding;
    int itpoint;
    i = 0;

    while (!feof(file)) {
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='#');
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='\n');
        j = 0;
        if (!feof(file)) {
            // printf("building %d\n", i);
            while (c = getc(file) != '#'){
                fscanf (file, "%d", &valx);
                fscanf (file, "%d", &valy);
                //printf("%d %d\n",valx , valy);
                setPoint(&tree[i].P[j], valx, valy);
                //printf("%d %d\n", building[i].P[j].x, building[i].P[j].y);
                // building.push_back(pTemp);
                j++;
            }
            tree[i].neff = j;
            // printf("%d\n", building[i].neff);
            i++;
        }
    };
    nTree = i;
    fclose(file);
}
void loadStreet() {
    //printf("TES");
    char c;
    int valx, valy;
    Point pTemp;
    FILE *file;
    file = fopen("jalan.txt", "r");
    int i, j;
    street = malloc(200 * sizeof(Street));
    for (i = 0; i < 200; i++) {
        street[i].P = (Point *) malloc(50 * sizeof(Point));
    }
    int itbuilding;
    int itpoint;
    i = 0;

    while (!feof(file)) {
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='#');
        do {
                c = getc(file);
                if (feof(file)) break;
        } while (c!='\n');
        j = 0;
        if (!feof(file)) {
            // printf("building %d\n", i);
            while (c = getc(file) != '#'){
                fscanf (file, "%d", &valx);
                fscanf (file, "%d", &valy);
                //printf("%d %d\n",valx , valy);
                setPoint(&street[i].P[j], valx, valy);
                //printf("%d %d\n", building[i].P[j].x, building[i].P[j].y);
                // building.push_back(pTemp);
                j++;
            }
            street[i].neff = j;
            // printf("%d\n", building[i].neff);
            i++;
        }
    };
    nStreet = i;
    fclose(file);
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

/*==============DRAW LINE UNTUK ZOOM==================*/
void drawZoomLineX(Point* p1, Point* p2, Color* c, int positif) {
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

void drawZoomLineY(Point* p1, Point* p2, Color* c, int positif) {
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


void drawZoomLine(Point* p1, Point* p2, Color* c) {
    int dx = p2 -> x - p1 -> x;
    int dy = p2 -> y - p1 -> y;
    int octant = getOctant(dx, dy);    
    // algoritma bresenham sebenernya hanya bisa menggambar di octant 0,
    // atur sedemikian rupa supaya 7 octant lainnya bisa masuk ke algoritma
    switch (octant) {
        case 0: drawZoomLineX(p1, p2, c, 1); break;
        case 1: drawZoomLineY(p1, p2, c, 1); break;
        case 2: drawZoomLineY(p1, p2, c, -1); break;
        case 3: swapPoint(p1, p2); drawZoomLineX(p1, p2, c, -1); swapPoint(p1, p2); break;
        case 4: swapPoint(p1, p2); drawZoomLineX(p1, p2, c, 1); swapPoint(p1, p2); break;
        case 5: swapPoint(p1, p2); drawZoomLineY(p1, p2, c, 1); swapPoint(p1, p2); break;
        case 6: swapPoint(p1, p2); drawZoomLineY(p1, p2, c, -1); swapPoint(p1, p2); break;
        case 7: drawZoomLineX(p1, p2, c, -1); break;
    }
}
/*------------------------------------------------------*/

/*==============DRAW LINE UNTUK BIASA===========*/
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
			location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
			changeARGB(location, c);
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
			location = (i + vinfo.xoffset) * bytePerPixel + (j + vinfo.yoffset) * finfo.line_length;
			changeARGB(location, c);
		}
        if (d > 0) {
            // positif = 1 berarti x makin lama makin bertambah
            i += positif;
            d -= dy;
        }
        d += dx;
    }
}

void drawLine(Point* p1, Point* p2, Color* c) {
    int dx = p2 -> x - p1 -> x;
    int dy = p2 -> y - p1 -> y;
    int octant = getOctant(dx, dy);    
    // algoritma bresenham sebenernya hanya bisa menggambar di octant 0,
    // atur sedemikian rupa supaya 7 octant lainnya bisa masuk ke algoritma
    //printf("%d-%d\n",p1->x,p1->y);
    //printf("%d-%d\n",p2->x,p2->y);
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
/*------------------------------------------------------*/

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

Point * initZoomWindow(Point * window_center){
	Point * zoom = (Point*) malloc(5 * sizeof(Point));
	int x = window_center->x;
	int y = window_center->y;
	setPoint(&zoom[0], x-zoom_width/2, y-zoom_height/2);
	setPoint(&zoom[1], x+zoom_width/2, y-zoom_height/2);
	setPoint(&zoom[2], x+zoom_width/2, y+zoom_height/2);
	setPoint(&zoom[3], x-zoom_width/2, y+zoom_height/2);
	setPoint(&zoom[4], x-zoom_width/2, y-zoom_height/2);
	return zoom;
}

void drawBuilding(int zoom, Point* p, int numPoints, Color c) {
	int k, i;
	for (k = 0; k<zoom; k++) {
		for (i = 0; i < numPoints-1; i++) {
			drawLine(&p[i], &p[i+1], &c);
			//p[i].x++;
			//p[i].y++;
		}
		drawLine(&p[i], &p[0], &c);
		//p[i].x++;
		//p[i].y++;
	}
}

void drawZoomBuilding(int zoom, Point* p, int numPoints, Color c) {
	int k, i;
		for (i = 0; i < numPoints-1; i++) {
			drawZoomLine(&p[i], &p[i+1], &c);
			//p[i].x++;
			//p[i].y++;
		}
		drawZoomLine(&p[i], &p[0], &c);
		//p[i].x++;
		//p[i].y++;
}

void drawMap(Building* building, Color c) {
	int i, j;
	for(i = 0; i < nBuilding; i++) {
		drawBuilding(1, building[i].P, building[i].neff, c);
	}
}


void drawZoomMap(Building* building, Color c) {
	int i, j;
	for(i = 0; i < nBuilding; i++) {
		drawZoomBuilding(1, building[i].P, building[i].neff, c);
	}
}
void zoom(float zoom, Point * zoomPoint, Building * building){
	Point * zoomtemp;
	zoomtemp = (Point*) malloc(1 * sizeof(Point));
	zoomtemp[0].x = zoomPoint->x * zoom;
	zoomtemp[0].y = zoomPoint->y * zoom;
	int deltax = zoomtemp[0].x - window_center->x;
	deltax=deltax*-1;
	int deltay = zoomtemp[0].y - window_center->y;
	deltay=deltay*-1;
	
	Building * temp;
	temp = malloc(nBuilding * sizeof(Building));
	for (int it = 0; it < nBuilding; it++) {
		temp[it].neff = building[it].neff;
        temp[it].P = (Point *) malloc(50 * sizeof(Point));
    }
    
	int a, b;
	for(a = 0; a < nBuilding; a++) {
		for (b = 0; b < building[a].neff; b++) {
			temp[a].P[b].x = (building[a].P[b].x * zoom) + deltax;
			temp[a].P[b].y = (building[a].P[b].y * zoom) + deltay;
			//printf("%d %d\n",building[a].P[b].x,building[a].P[b].y);
			//printf("%d %d\n",temp[a].P[b].x,temp[a].P[b].y);
		}
	}
	
	Color c;
	setColor(&c, 255, 0, 0);
	drawZoomMap(temp, c);
	free(temp);
}


int main() {
    setColor(&bg, 0, 0, 0);
    connectBuffer();
    clearScreen(&bg);
    Color c, cDel;
    Color cT;
    Color cS;
    setColor(&c, 255, 0, 0);
    setColor(&c2, 0, 255, 255);
    setColor(&cDel, 255, 255, 255);
    setColor(&cT, 0, 255, 0);
    setColor(&cS, 0, 0, 255);

    window_center = (Point*) malloc(1 * sizeof(Point));
	setPoint(&window_center[0], 650, 200);
	Point * zoomPoint = (Point*) malloc(1 * sizeof(Point));
	Point * zoom2 = (Point*) malloc(5 * sizeof(Point));
	setPoint(&zoomPoint[0], 300, 300);
	window = initWindow(window_center);
	zoom2 = initZoomWindow(zoomPoint);
    clearScreen(&bg);
    int i,j;
    char ch;
    loadBuildings4();
    loadStreet();
    loadTree();
    float zoom_val = 2;
    char stroke;
    int alreadyPaintBuilding = 1;
    int alreadyPaintTree = 1;
    int alreadyPaintStreet = 1;
    while(1){
		clearScreen(&bg);
		//draw map
		nBuilding = nBuilding2;
		if (alreadyPaintBuilding == 1) {
			drawMap(building, c);
			zoom(zoom_val,zoomPoint,building);
		
		}
		nBuilding = nTree;
		if (alreadyPaintTree == 1) {
			drawMap(tree, cT);
			zoom(zoom_val,zoomPoint,tree);
		
		}
		nBuilding = nStreet;
		if (alreadyPaintStreet == 1) {
			drawMap(street, cS);
		zoom(zoom_val,zoomPoint,street);
	
		}
		
		for(j = 0; j < 4; j++) {
			drawLine(&window[j], &window[j + 1], &c);
		}
		
		for(j = 0; j < 4; j++) {
			drawLine(&zoom2[j], &zoom2[j + 1], &cDel);
		}
		
		
		stroke = getch();
		switch (stroke) {
			case 'd': zoomPoint->x+=20; break;
			case 's': zoomPoint->y+=20; break;
			case 'a': zoomPoint->x-=20; break;
			case 'w': zoomPoint->y-=20; break;
			case 'z':
				zoom_val*=2;
				zoom_height=zoom_height/2;
				zoom_width=zoom_width/2;
				break;
			case 'x':
				zoom_val=zoom_val/2;
				zoom_height*=2;
				zoom_width*=2;
				break;
			case 't':
				if(alreadyPaintTree == 0) {
					alreadyPaintTree = 1;
				} else {
					alreadyPaintTree = 0;
				}
				break;
			case 'j' :
				if(alreadyPaintStreet == 0) {
					alreadyPaintStreet = 1;
				} else {
					alreadyPaintStreet = 0;
				}
				break;
			case 'b' :
				if(alreadyPaintBuilding == 0) {
					alreadyPaintBuilding = 1;
				} else {
					alreadyPaintBuilding = 0;
				}
				break;
			
					
		}
		zoom2 = initZoomWindow(zoomPoint);
	}
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

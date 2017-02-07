#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>

typedef struct Points {
    int x;
    int y;
} Point;
typedef struct Colors {
    char a;
    char r;
    char g;
    char b;
} Color;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

Color bg; // warna background
char* fbp; // memory map ke fb0
int fbfd; // file fb0
int screensize; // length nya si fbp
int kaboom = 0; // bernilai 1 jika pesawat sudah tertembak
int overPixel = 10; // intinya satu baris ada 1376 pixel = 1366 pixel + 10 pixel "ngumpet"
int bytePerPixel;
int srcXBeam, srcYBeam, destXBeam;
int headPlane, tailPlane;

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
    int location = 0;
    // iterasi pixel setiap baris dan setiap kolom
    for (j = 0; j < vinfo.yres; j++) {
        for (i = 0; i < vinfo.xres; i++) {
            changeARGB(location, c);
            location += bytePerPixel; // pixel sebelah kanannya berjarak sekian byte
        }
        location += (bytePerPixel * overPixel); // pixel pertama pada baris selanjutnya berjarak sekian byte
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

void drawLineY(Point* p1, Point* p2, Color* c, int positif) {
	// algoritma Bresenham
    int dx = (p2 -> x - p1 -> x) * positif;
    int dy = p2 -> y - p1 -> y;
    int d = dx + dx - dy;
    int i = p1 -> x;
    int j, location;
    // lebih panjang vertikal, maka iterasi berdasarkan y
    for (j = p1 -> y; j <= p2 -> y; j++) {
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



void solidFill(Point* firepoint, Color c){
    Point newfp;
    
    if(firepoint->x>1 && firepoint->x<vinfo.xres-1 && firepoint->y>1 && firepoint->y<vinfo.yres-1){
        newfp.x = firepoint->x+1;
        newfp.y = firepoint->y;
        long location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
            //ganti jadi rgb yang di mau buat gnti warna
            *(fbp + location) =c.a;
            *(fbp + location +1) = c.r;
            *(fbp + location +2) = c.g;
            *(fbp + location + 3) = c.b;
            solidFill(&newfp, c);
        }
        
        newfp.x = firepoint->x-1;
        newfp.y = firepoint->y;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
            *(fbp + location) =c.a;
            *(fbp + location +1) = c.r;
            *(fbp + location +2) = c.g;
            *(fbp + location + 3) = c.b;
            solidFill(&newfp, c);
        }
        
        newfp.x = firepoint->x;
        newfp.y = firepoint->y+1;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
            *(fbp + location) =c.a;
            *(fbp + location +1) = c.r;
            *(fbp + location +2) = c.g;
            *(fbp + location + 3) = c.b;
            solidFill(&newfp, c);
        }
        
        newfp.x = firepoint->x;
        newfp.y = firepoint->y-1;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==0 && *(fbp + location +1)==0 && *(fbp + location +2) ==0){
            *(fbp + location) =c.a;
            *(fbp + location +1) = c.r;
            *(fbp + location +2) = c.g;
            *(fbp + location + 3) = c.b;
            solidFill(&newfp, c);
        }
    }
}

void falldown4point(Point* p, Point firepoint, Color c) {
	int i, j;
	int minY = p[0].y;
	int minX = p[0].x;
	int maxX = p[0].x;

	//cari titik terbawah
	for (i=1; i<4; i++) {
		if (p[i].y>minY)
			minY = p[i].y;
	}

	//cari titik terkiri dan terkanan
	for (i=1; i<4; i++) {
		if (p[i].x > maxX)
			maxX = p[i].x;
		if (p[i].x < minX)
			minX = p[i].x;
	}

	while (minY < vinfo.yres-40) {
		//hapus
		for (i=0; i<3; i++) {
			drawLine(&p[i], &p[i + 1], &bg);
	    }
	    drawLine(&p[3], &p[0], &bg);

	    for(int x=minX; x<=maxX; x++){
			for(int y=0; y<minY; y++){
				long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) =0;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 0;
			}
		}

	    //gambar ulang
		for (i=0; i<4; i++) {
			setPoint(&p[i], p[i].x, p[i].y+1);
		}
		for (i=0; i<3; i++) {
			drawLine(&p[i], &p[i + 1], &c);
	    }
	    drawLine(&p[3], &p[0], &c);
	    //warnai ulang
	    setPoint(&firepoint, firepoint.x, firepoint.y+1);
	    solidFill(&firepoint, c);
	    usleep(500);
	}
}
void drawPlaneBreak(Point* plane) {
	Color cDestroy;
	setColor(&cDestroy, 255, 255, 255);
	int i, j;
	Point* planeBreak1;
	Point* planeBreak2;
	Point* planeBreak3;
	Point temp1, temp2, temp3;
	planeBreak1 = (Point*) malloc (4 * sizeof(Point));
	planeBreak2 = (Point*) malloc (4 * sizeof(Point));
	planeBreak3 = (Point*) malloc (4 * sizeof(Point));

	//Buat bagian plane 1
	setPoint(&planeBreak1[0], plane[0].x, plane[0].y);
	setPoint(&planeBreak1[1], plane[1].x, plane[1].y);
	setPoint(&planeBreak1[2], plane[1].x+50, plane[2].y);
	setPoint(&planeBreak1[3], plane[0].x+60, plane[0].y);
	for(i = 0; i < 3; i++) {
        drawLine(&planeBreak1[i], &planeBreak1[i + 1], &cDestroy);
    }
    drawLine(&planeBreak1[3], &planeBreak1[0], &cDestroy);
    //Warnai
    setPoint(&temp1, planeBreak1[1].x, planeBreak1[1].y+10);
    solidFill(&temp1, cDestroy);

	//Buat bagian plane 2
	setPoint(&planeBreak2[0], planeBreak1[2].x+10, planeBreak1[2].y);
	setPoint(&planeBreak2[1], plane[2].x+10, plane[2].y);
	setPoint(&planeBreak2[2], plane[5].x+10, plane[5].y);
	setPoint(&planeBreak2[3], planeBreak1[3].x+10, planeBreak1[3].y);
	for(i = 0; i < 3; i++) {
        drawLine(&planeBreak2[i], &planeBreak2[i + 1], &cDestroy);
    }
    drawLine(&planeBreak2[3], &planeBreak2[0], &cDestroy);
    //Warnai
    setPoint(&temp2, planeBreak2[0].x+10, planeBreak2[0].y+10);
    solidFill(&temp2, cDestroy);

    //Buat bagian plane 3
	setPoint(&planeBreak3[0], planeBreak2[1].x+10, planeBreak2[1].y);
	setPoint(&planeBreak3[1], plane[3].x+20, plane[3].y);
	setPoint(&planeBreak3[2], plane[4].x+20, plane[4].y);
	setPoint(&planeBreak3[3], planeBreak2[2].x+10, planeBreak2[2].y);
	for(i = 0; i < 3; i++) {
        drawLine(&planeBreak3[i], &planeBreak3[i + 1], &cDestroy);
    }
    drawLine(&planeBreak3[3], &planeBreak3[0], &cDestroy);
    //Warnai
    setPoint(&temp3, planeBreak3[0].x+10, planeBreak3[0].y);
    solidFill(&temp3, cDestroy);

    //Jatuhkan
    falldown4point(planeBreak1, temp1, cDestroy);
    falldown4point(planeBreak2, temp2, cDestroy);
    falldown4point(planeBreak3, temp3, cDestroy);
}

void* drawPlane() {
	Color cDestroy;
	setColor(&cDestroy, 255, 255, 255);
	int i;
	Point* planeBreak1;
	Point* planeBreak2;
	Point* planeBreak3;
	Point temp;
	planeBreak1 = (Point*) malloc (4 * sizeof(Point));
	planeBreak2 = (Point*) malloc (4 * sizeof(Point));
	planeBreak3 = (Point*) malloc (4 * sizeof(Point));
    Color c, cDel;
    setColor(&c, 255, 0, 0);
    Point* plane;
    plane = (Point*) malloc(6 * sizeof(Point));
    int lengthPlane = 260; // panjang pesawat dari head sampai tail
    tailPlane = vinfo.xres;
    headPlane = tailPlane - lengthPlane;
    int j;

    while (!kaboom) { // selama pesawat belum ketembak
        while (headPlane > 0) { // selama pesawat belum mentok di kiri
            if (kaboom) { // pesawat kena tembak
            	clearScreen(&bg);
            	drawBoxgun();
                //Buat poligon tertembak
                drawPlaneBreak(plane);
                sleep(2);
                break;
            } else { // masih terbang
            	// hapus
                for(j = 0; j < 5; j++) {
                    drawLine(&plane[j], &plane[j + 1], &bg);
                }
                drawLine(&plane[5], &plane[0], &bg);
                
                setPoint(&plane[0], headPlane, 90);
                setPoint(&plane[1], headPlane + 40, 50);
                setPoint(&plane[2], headPlane + 190, 50);
                setPoint(&plane[3], headPlane + 220, 20);
                setPoint(&plane[4], headPlane + 260, 20);
                setPoint(&plane[5], headPlane + 190, 90);
                // gambar
                for(j = 0; j < 5; j++) {
                    drawLine(&plane[j], &plane[j + 1], &c);
                }
                drawLine(&plane[5], &plane[0], &c);
                // Warnai
                setPoint(&temp, plane[1].x, 60);
                solidFill(&temp, c);
                setPoint(&temp, plane[3].x, 30);
                solidFill(&temp, c);
                usleep(500);

                headPlane--;
                tailPlane--;
            }
        }
        for(int x=0; x<vinfo.xres; x++){
			for(int y=0; y<vinfo.yres/3; y++){
				long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) =0;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 0;
			}
		}
        tailPlane = vinfo.xres; // restart dari ujung kanan lagi
        headPlane = tailPlane - lengthPlane;
    }
}

void drawBoxgun() {
	Point* box;
    Point boxFirePoint;
    box = (Point*) malloc(4*sizeof(Point));
    int i;
    Color cBox; setColor(&cBox, 0, 255, 0);
    //Membuat kotak tembak
    setPoint(&box[0], vinfo.xres/2 - 40, vinfo.yres -1);
    setPoint(&box[1], vinfo.xres/2 + 40, vinfo.yres -1);
    setPoint(&box[2], vinfo.xres/2 + 40, vinfo.yres -40);
    setPoint(&box[3], vinfo.xres/2 - 40, vinfo.yres -40);
    for(i = 0; i < 3; i++) {
        drawLine(&box[i], &box[i + 1], &cBox);
    }
    drawLine(&box[0], &box[i], &cBox);
    setPoint(&boxFirePoint, vinfo.xres/2, vinfo.yres - 20);
    solidFill(&boxFirePoint, cBox);
}
void* drawLasergun() {
	Point bottomGun, mouthGun;
	Color c; setColor(&c, 0, 255, 255);
    int i;
    drawBoxgun();
    setPoint(&bottomGun, vinfo.xres / 2, vinfo.yres - 40); // berada di tengah bawah
    int moveLeft = 1; // moveLeft = 1 berarti arah mulut geser ke kiri
    int lengthGun = 100; // panjang dari bawah sampe atas
    // laser ditembakkan dari titik srcBeam ke destBeam
    srcYBeam = vinfo.yres - lengthGun;
    destXBeam = vinfo.xres;
    // konstanta-konstanta untuk menghitung srcXBeam, didapat dari penurunan rumus
    int constA = vinfo.xres * lengthGun / 2;
    int constB = vinfo.xres * vinfo.yres / 2;
    while (1) { // berhenti hanya jika di interrupt master
        // penembak ganti arah bolak balik kiri kanan
        if (moveLeft) {
            destXBeam--;
            if (destXBeam <= 0) {
                moveLeft = 0;
            }
        } else {
            destXBeam++;
            if (destXBeam >= vinfo.xres) {
                moveLeft = 1;
            }
        }
        srcXBeam = (destXBeam * lengthGun + constB - constA) / vinfo.yres;
        setPoint(&mouthGun, srcXBeam, srcYBeam);
        // gambar
        drawLine(&bottomGun, &mouthGun, &c);
        usleep(2000);
        // hapus
        drawLine(&bottomGun, &mouthGun, &bg);
    }
}

void* drawBeam() {
	Point srcBeam, destBeam;
    Color c; setColor(&c, 255, 0, 255);
    char stroke;
    while(!kaboom) {
        stroke = fgetc(stdin);
        if (stroke == 10) { // ENTER
            setPoint(&srcBeam, srcXBeam, srcYBeam);
            setPoint(&destBeam, destXBeam, 0);
            // gambar
            
            drawLine(&srcBeam, &destBeam, &c);
            if (destXBeam > headPlane && destXBeam < tailPlane) { // kena
                kaboom = 1;
            } else {
            	// hapus
                usleep(20000);
                drawLine(&srcBeam, &destBeam, &bg);
            }
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

int main() {
    setColor(&bg, 0, 0, 255);
    connectBuffer();
    clearScreen(&bg);
    pthread_t thrPlane, thrLasergun, thrBeam;
    pthread_create(&thrPlane, NULL, drawPlane, "thrPlane");
    pthread_create(&thrLasergun, NULL, drawLasergun, "thrLasergun");
    pthread_create(&thrBeam, NULL, drawBeam, "thrBeam");
    pthread_join(thrBeam, NULL); // sampe berhasil nembak
    pthread_join(thrPlane, NULL);
    pthread_cancel(thrLasergun);
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

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

void* drawPlane() {
    Color c, cDestroy;
    setColor(&c, 255, 0, 0);
    setColor(&cDestroy, 255, 255, 0);
    Point* plane; // kumpulan titik yang membentuk gambar pesawat
    Point temp;
    plane = (Point*) malloc(24 * sizeof(Point));
    int lengthPlane = 260; // panjang pesawat dari head sampai tail
    tailPlane = vinfo.xres;
    headPlane = tailPlane - lengthPlane;
    int j;

    while (!kaboom) { // selama pesawat belum ketembak
        while (headPlane > 0) { // selama pesawat belum mentok di kiri
            if (kaboom) { // pesawat kena tembak
                //Hapus warna pesawat
                /*setPoint(&temp, plane[1].x, 60);
                solidFill(temp, bg);
                setPoint(&temp, plane[3].x, 30);
                solidFill(temp, bg);*/
                //Buat poligon tertembak

                setPoint(&plane[0], 0, 22);
                setPoint(&plane[1], 2, 2);
                setPoint(&plane[2], 5, 6);
                setPoint(&plane[3], 9, 8);
                setPoint(&plane[4], 12, 8);
                setPoint(&plane[5], 19, 1);
                setPoint(&plane[6], 21.5, 1);
                setPoint(&plane[7], 20, 8);
                setPoint(&plane[8], 21, 8);
                setPoint(&plane[9], 22, 7);
                setPoint(&plane[10], 24, 7);
                setPoint(&plane[11], 25, 8);
                setPoint(&plane[12], 27, 8);
                setPoint(&plane[13], 33, 10);
                setPoint(&plane[14], 27, 12);
                setPoint(&plane[15], 20, 12);
                setPoint(&plane[16], 19, 13);
                setPoint(&plane[17], 18, 13);
                setPoint(&plane[18], 10, 22);
                setPoint(&plane[19], 8, 22);
                setPoint(&plane[20], 10.5, 12);
                setPoint(&plane[21], 0, 12);
                setPoint(&plane[22], 1, 8);
                setPoint(&plane[23], 2, 8);
                

                // gambar pesawat meledak
                for(j = 0; j < 24; j++) {
                    drawLine(&plane[j], &plane[j + 1], &cDestroy);
                }
                drawLine(&plane[23], &plane[0], &cDestroy);
                //Warnai
                setPoint(&temp, plane[2].x, 50);
                solidFill(&temp, cDestroy);
                break;
            } else { // masih terbang
                //setPoint(&plane[0], headPlane, 90);
                //setPoint(&plane[1], headPlane + 40, 50);
                //setPoint(&plane[2], headPlane + 190, 50);
                //setPoint(&plane[3], headPlane + 220, 20);
                //setPoint(&plane[4], headPlane + 260, 20);
                //setPoint(&plane[5], headPlane + 190, 90);
                setPoint(&plane[0], 10, 220);
                setPoint(&plane[1], 20, 200);
                setPoint(&plane[2], 50, 60);
                setPoint(&plane[3], 90, 80);
                setPoint(&plane[4], 120, 80);
                setPoint(&plane[5], 190, 10);
                setPoint(&plane[6], 215, 10);
                setPoint(&plane[7], 200, 80);
                setPoint(&plane[8], 210, 80);
                setPoint(&plane[9], 220, 70);
                setPoint(&plane[10], 240, 70);
                setPoint(&plane[11], 250, 80);
                setPoint(&plane[12], 270, 80);
                setPoint(&plane[13], 330, 100);
                setPoint(&plane[14], 270, 120);
                setPoint(&plane[15], 200, 120);
                setPoint(&plane[16], 190, 130);
                setPoint(&plane[17], 180, 130);
                setPoint(&plane[18], 100, 220);
                setPoint(&plane[19], 80, 220);
                setPoint(&plane[20], 105, 120);
                setPoint(&plane[21], 10, 120);
                setPoint(&plane[22], 10, 80);
                setPoint(&plane[23], 20, 80);
                
                // gambar
                for(j = 0; j < 24; j++) {
                    drawLine(&plane[j], &plane[j + 1], &c);
                }
                //drawLine(&plane[23], &plane[0], &c);
                // Warnai
                //setPoint(&temp, plane[1].x, 60);
                //solidFill(&temp, c);
                //setPoint(&temp, plane[3].x, 30);
                //solidFill(&temp, c);
                usleep(500);

                // hapus
                for(j = 0; j < 24; j++) {
                    drawLine(&plane[j], &plane[j + 1], &bg);
                }
                //drawLine(&plane[23], &plane[0], &bg);
                headPlane--;
                tailPlane--;
            }
        }
        tailPlane = vinfo.xres; // restart dari ujung kanan lagi
        headPlane = tailPlane - lengthPlane;
    }
}

void* drawLasergun() {
	Point bottomGun, mouthGun;
    Point* box;
    Point boxFirePoint;
    box = (Point*) malloc(4*sizeof(Point));
    int i;
    Color c; setColor(&c, 0, 255, 255);
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
    if ((int) fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
}

int main() {
    setColor(&bg, 0, 0, 255);
    connectBuffer();
    clearScreen(&bg);
    drawPlane();
    //pthread_t thrPlane, thrLasergun, thrBeam;
    //pthread_create(&thrPlane, NULL, drawPlane, "thrPlane");
    //pthread_create(&thrLasergun, NULL, drawLasergun, "thrLasergun");
    //pthread_create(&thrBeam, NULL, drawBeam, "thrBeam");
    //pthread_join(thrBeam, NULL); // sampe berhasil nembak
    //pthread_join(thrPlane, NULL);
    //pthread_cancel(thrLasergun);
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

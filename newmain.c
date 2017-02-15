#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <math.h>

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

Color bg; // warna background
char* fbp; // memory map ke fb0
int fbfd; // file fb0
int screensize; // length nya si fbp
int kaboom = 0; // bernilai 1 jika pesawat sudah tertembak
int overPixel = 10; // intinya satu baris ada 1376 pixel = 1366 pixel + 10 pixel "ngumpet"
int bytePerPixel;
int srcXBeam, srcYBeam, destXBeam;
int headPlane, tailPlane;
Point roda1, roda2;

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

void clearFill (Point* firepoint, Color cbef) {
	Point newfp;
	//Color fillColor;
	long location;
	//location = (firepoint->x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (firepoint->y+vinfo.yoffset) * finfo.line_length;
	int a = cbef.a;
	int r = cbef.r;
	int g = cbef.g;
	int b = cbef.b;
	/*printf("a = %d, r = %d, g = %d, b = %d", a, r, g, b);
	setColor(&fillColor, r, g, b);*/
    //hapus warna
    if(firepoint->x>1 && firepoint->x<vinfo.xres-1 && firepoint->y>1 && firepoint->y<vinfo.yres-1){
        newfp.x = firepoint->x+1;
        newfp.y = firepoint->y;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==a && *(fbp + location +1)==r && *(fbp + location +2) ==g){
            //ganti jadi rgb yang di mau buat gnti warna
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = 0;
            clearFill(&newfp, cbef);
        }
        
        newfp.x = firepoint->x-1;
        newfp.y = firepoint->y;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==a && *(fbp + location +1)==r && *(fbp + location +2) ==g){
            //ganti jadi rgb yang di mau buat gnti warna
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = 0;
            clearFill(&newfp, cbef);
        }
        
        newfp.x = firepoint->x;
        newfp.y = firepoint->y+1;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==a && *(fbp + location +1)==r && *(fbp + location +2) ==g){
            //ganti jadi rgb yang di mau buat gnti warna
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = 0;
            clearFill(&newfp, cbef);
        }
        
        newfp.x = firepoint->x;
        newfp.y = firepoint->y-1;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==a && *(fbp + location +1)==r && *(fbp + location +2) ==g){
            //ganti jadi rgb yang di mau buat gnti warna
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = 0;
            clearFill(&newfp, cbef);
        }
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

void solidFillReverse(Point* firepoint, Color c){
    Point newfp;
    if(firepoint->x>1 && firepoint->x<vinfo.xres-1 && firepoint->y>1 && firepoint->y<vinfo.yres-1){
        newfp.x = firepoint->x+1;
        newfp.y = firepoint->y;
        long location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==c.a && *(fbp + location +1)==c.r && *(fbp + location +2) ==c.g){
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = c.b;
            solidFillReverse(&newfp, c);
        }
        
        newfp.x = firepoint->x-1;
        newfp.y = firepoint->y;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==c.a && *(fbp + location +1)==c.r && *(fbp + location +2) ==c.g){
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = c.b;
            solidFillReverse(&newfp, c);
        }
        
        newfp.x = firepoint->x;
        newfp.y = firepoint->y+1;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==c.a && *(fbp + location +1)==c.r && *(fbp + location +2) ==c.g){
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = c.b;
            solidFillReverse(&newfp, c);
        }
        
        newfp.x = firepoint->x;
        newfp.y = firepoint->y-1;
        location = (newfp.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (newfp.y+vinfo.yoffset) * finfo.line_length;
        if(*(fbp + location)==c.a && *(fbp + location +1)==c.r && *(fbp + location +2) ==c.g){
            *(fbp + location) =0;
            *(fbp + location +1) = 0;
            *(fbp + location +2) = 0;
            *(fbp + location + 3) = c.b;
            solidFillReverse(&newfp, c);
        }
    }
}


void* falldown4point(void* params) {
	int i, j;
	struct readFallDownParams *readparams = params;
	int minY = readparams->p[0].y;
	int minX = readparams->p[0].x;
	int maxX = readparams->p[0].x;

	//cari titik terbawah dan teratas
	for (i=1; i<4; i++) {
		if (readparams->p[i].y>minY)
			minY = readparams->p[i].y;
	}

	//cari titik terkiri dan terkanan
	for (i=1; i<4; i++) {
		if (readparams->p[i].x > maxX)
			maxX = readparams->p[i].x;
		if (readparams->p[i].x < minX)
			minX = readparams->p[i].x;
	}

	while (minY < vinfo.yres-40) {
		//hapus
		for (i=0; i<3; i++) {
			drawLine(&readparams->p[i], &readparams->p[i + 1], &bg);
	    }
	    drawLine(&readparams->p[3], &readparams->p[0], &bg);

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
			setPoint(&readparams->p[i], readparams->p[i].x, readparams->p[i].y+1);
		}
		for (i=0; i<3; i++) {
			drawLine(&readparams->p[i], &readparams->p[i + 1], &readparams->c);
	    }
	    drawLine(&readparams->p[3], &readparams->p[0], &readparams->c);
	    //warnai ulang
	    setPoint(&readparams->firepoint, readparams->firepoint.x, readparams->firepoint.y+1);
	    solidFill(&readparams->firepoint,  readparams->c);
	    minY++;
	    usleep(5000);
	}
}

Point spinDegree(Point p, int degree) {
	int i;
	Point pTemp;
	double PI = 3.14159265;
	double degInRad = degree*PI/180;
	pTemp.x = ceil(p.x*cos(degInRad)-p.y*sin(degInRad));
	pTemp.y = ceil(p.x*sin(degInRad)+p.y*cos(degInRad));
	return pTemp;
}

void spinDegreeUsingCenter(Point *p, int degree, Point center) {
	int i;
	Point pTemp;
	double PI = 3.14159265;
	double degInRad = degree*PI/180;
	pTemp.x = ceil(((*p).x-center.x)*cos(degInRad)-((*p).y-center.y)*sin(degInRad) + center.x);
	pTemp.y = ceil(((*p).x-center.x)*sin(degInRad)+((*p).y-center.y)*cos(degInRad) + center.y);
	(*p).x = pTemp.x;
	(*p).y = pTemp.y;
}

void* fallSpin(void *params) {
	//Thread procedure untuk menjatuhkan sambil memutaint i, j;

	int i;
	Point pTemp;
	Color cc;
	setColor(&cc, 255, 0, 0);
	struct readFallSpinParams *readparams = params;
	int minY = readparams->p[0].y;
	int maxY = readparams->p[0].y;
	int minX = readparams->p[0].x;
	int maxX = readparams->p[0].x;

	//cari titik terbawah dan teratas
	for (i=1; i<4; i++) {
		if (readparams->p[i].y<minY)
			minY = readparams->p[i].y;
		if (readparams->p[i].y>maxY)
			maxY = readparams->p[i].y;
	}

	//cari titik terkiri dan terkanan
	for (i=1; i<4; i++) {
		if (readparams->p[i].x > maxX)
			maxX = readparams->p[i].x;
		if (readparams->p[i].x < minX)
			minX = readparams->p[i].x;
	}

	while (maxY < vinfo.yres-40) {
		//hapus
		//solidFill(&readparams->firepoint, bg);
		clearFill(&readparams->firepoint, readparams->c);

		for (i=0; i<3; i++) {
			drawLine(&readparams->p[i], &readparams->p[i + 1], &bg);
	    }
	    drawLine(&readparams->p[3], &readparams->p[0], &bg);	    
	    //clearScreen(&bg)
	    
	    /*for(int x=minX; x<=maxX; x++){
			for(int y=minY; y<maxY; y++){
				long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				*(fbp + location) =0;
				*(fbp + location +1) = 0;
				*(fbp + location +2) = 0;
				*(fbp + location + 3) = 0;
				
			}
		}*/

	    //gambar ulang dengan memutar
	    //kembalikan ke titik 0
	    for (i=0; i<4; i++) {
			setPoint(&readparams->p[i], readparams->p[i].x-readparams->pivot.x, readparams->p[i].y-readparams->pivot.y);
		}
		setPoint(&readparams->firepoint, readparams->firepoint.x-readparams->pivot.x, readparams->firepoint.y-readparams->pivot.y);

		//putar 45 derajat dan turunkan sekian piksel
		for (i=0; i<4; i++) {
			pTemp = spinDegree(readparams->p[i], 45);
			setPoint(&readparams->p[i], pTemp.x, pTemp.y+30);
		}
		//putar firepoint dan turunkan sekian piksel
		pTemp = spinDegree(readparams->firepoint, 45);
		setPoint(&readparams->firepoint, pTemp.x, pTemp.y+30);

		//turunkan pivot
		setPoint(&readparams->pivot, readparams->pivot.x, readparams->pivot.y+30);

		//kembalikan ke titik asal
		for (i=0; i<4; i++) {
			setPoint(&readparams->p[i], readparams->p[i].x+readparams->pivot.x, readparams->p[i].y+readparams->pivot.y);
		}
		setPoint(&readparams->firepoint, readparams->firepoint.x+readparams->pivot.x, readparams->firepoint.y+readparams->pivot.y);
		//cari titik terbawah
	    int maxY = readparams->p[0].y;
		for (i=1; i<4; i++) {
			if (readparams->p[i].y>maxY)
				maxY = readparams->p[i].y;
		}

		if (maxY < vinfo.yres - 40) {
		//gambar ulang
			for (i=0; i<3; i++) {
				drawLine(&readparams->p[i], &readparams->p[i + 1], &readparams->c);
		    }
		    drawLine(&readparams->p[3], &readparams->p[0], &readparams->c);
		    //warnai ulang
		    solidFill(&readparams->firepoint,  readparams->c);
		} else {
			sleep(10);
			break;
		}
		usleep(100000);
	}
}

void* fallRightSpin(void *params) {
    //Thread procedure untuk menjatuhkan sambil memutaint i, j;

    int i;
    Point pTemp;
    Color cc;
    setColor(&cc, 255, 0, 0);
    struct readFallSpinParams *readparams = params;
    int minY = readparams->p[0].y;
    int maxY = readparams->p[0].y;
    int minX = readparams->p[0].x;
    int maxX = readparams->p[0].x;

    //cari titik terbawah dan teratas
    for (i=1; i<4; i++) {
        if (readparams->p[i].y<minY)
            minY = readparams->p[i].y;
        if (readparams->p[i].y>maxY)
            maxY = readparams->p[i].y;
    }

    //cari titik terkiri dan terkanan
    for (i=1; i<4; i++) {
        if (readparams->p[i].x > maxX)
            maxX = readparams->p[i].x;
        if (readparams->p[i].x < minX)
            minX = readparams->p[i].x;
    }

    while (maxY < vinfo.yres-40) {
        //hapus
        //solidFill(&readparams->firepoint, bg);
        clearFill(&readparams->firepoint, readparams->c);

        for (i=0; i<3; i++) {
            drawLine(&readparams->p[i], &readparams->p[i + 1], &bg);
        }
        drawLine(&readparams->p[3], &readparams->p[0], &bg);        
        //clearScreen(&bg)
        
        //gambar ulang dengan memutar
        //kembalikan ke titik 0
        for (i=0; i<4; i++) {
            setPoint(&readparams->p[i], readparams->p[i].x-readparams->pivot.x, readparams->p[i].y-readparams->pivot.y);
        }
        setPoint(&readparams->firepoint, readparams->firepoint.x-readparams->pivot.x, readparams->firepoint.y-readparams->pivot.y);

        //putar 45 derajat dan turunkan sekian piksel
        for (i=0; i<4; i++) {
            pTemp = spinDegree(readparams->p[i], 37);
            setPoint(&readparams->p[i], pTemp.x+10, pTemp.y+10);
        }
        //putar firepoint dan turunkan sekian piksel
        pTemp = spinDegree(readparams->firepoint, 37);
        setPoint(&readparams->firepoint, pTemp.x+10, pTemp.y+10);

        //kembalikan ke titik asal
        for (i=0; i<4; i++) {
            setPoint(&readparams->p[i], readparams->p[i].x+readparams->pivot.x, readparams->p[i].y+readparams->pivot.y);
        }
        setPoint(&readparams->firepoint, readparams->firepoint.x+readparams->pivot.x, readparams->firepoint.y+readparams->pivot.y);
        
        //turunkan pivot
        setPoint(&readparams->pivot, (readparams->pivot.x+10)%vinfo.yres, readparams->pivot.y+10);

        //cari titik terbawah
        int maxY = readparams->p[0].y;
        for (i=1; i<4; i++) {
            if (readparams->p[i].y>maxY)
                maxY = readparams->p[i].y;
        }

        if (maxY < vinfo.yres - 40) {
        //gambar ulang
            for (i=0; i<3; i++) {
                drawLine(&readparams->p[i], &readparams->p[i + 1], &readparams->c);
            }
            drawLine(&readparams->p[3], &readparams->p[0], &readparams->c);
            //warnai ulang
            solidFill(&readparams->firepoint,  readparams->c);
        } else {
            sleep(2);
            break;
        }
        usleep(100000);
    }
}

void* fallLeftSpin(void *params) {
    //Thread procedure untuk menjatuhkan sambil memutaint i, j;

    int i;
    Point pTemp;
    Color cc;
    setColor(&cc, 255, 0, 0);
    struct readFallSpinParams *readparams = params;
    int minY = readparams->p[0].y;
    int maxY = readparams->p[0].y;
    int minX = readparams->p[0].x;
    int maxX = readparams->p[0].x;

    //cari titik terbawah dan teratas
    for (i=1; i<4; i++) {
        if (readparams->p[i].y<minY)
            minY = readparams->p[i].y;
        if (readparams->p[i].y>maxY)
            maxY = readparams->p[i].y;
    }

    //cari titik terkiri dan terkanan
    for (i=1; i<4; i++) {
        if (readparams->p[i].x > maxX)
            maxX = readparams->p[i].x;
        if (readparams->p[i].x < minX)
            minX = readparams->p[i].x;
    }

    while (maxY < vinfo.yres-40) {
        //hapus
        //solidFill(&readparams->firepoint, bg);
        clearFill(&readparams->firepoint, readparams->c);

        for (i=0; i<3; i++) {
            drawLine(&readparams->p[i], &readparams->p[i + 1], &bg);
        }
        drawLine(&readparams->p[3], &readparams->p[0], &bg);        
        
        //gambar ulang dengan memutar
        //kembalikan ke titik 0
        for (i=0; i<4; i++) {
            setPoint(&readparams->p[i], readparams->p[i].x-readparams->pivot.x, readparams->p[i].y-readparams->pivot.y);
        }
        setPoint(&readparams->firepoint, readparams->firepoint.x-readparams->pivot.x, readparams->firepoint.y-readparams->pivot.y);

        //putar 45 derajat dan turunkan sekian piksel
        for (i=0; i<4; i++) {
            pTemp = spinDegree(readparams->p[i], 45);
            setPoint(&readparams->p[i], pTemp.x-10, pTemp.y+10);
        }
        //putar firepoint dan turunkan sekian piksel
        pTemp = spinDegree(readparams->firepoint, 45);
        setPoint(&readparams->firepoint, pTemp.x-10, pTemp.y+10);

        //kembalikan ke titik asal
        for (i=0; i<4; i++) {
            setPoint(&readparams->p[i], readparams->p[i].x+readparams->pivot.x, readparams->p[i].y+readparams->pivot.y);
        }
        setPoint(&readparams->firepoint, readparams->firepoint.x+readparams->pivot.x, readparams->firepoint.y+readparams->pivot.y);
        
        //turunkan pivot
        setPoint(&readparams->pivot, (readparams->pivot.x-10)%vinfo.xres, readparams->pivot.y+10);
        
        //cari titik terbawah
        int maxY = readparams->p[0].y;
        for (i=1; i<4; i++) {
            if (readparams->p[i].y>maxY)
                maxY = readparams->p[i].y;
        }

        if (maxY < vinfo.yres - 40) {
        //gambar ulang
            for (i=0; i<3; i++) {
                drawLine(&readparams->p[i], &readparams->p[i + 1], &readparams->c);
            }
            drawLine(&readparams->p[3], &readparams->p[0], &readparams->c);
            //warnai ulang
            solidFill(&readparams->firepoint,  readparams->c);
        } else {
            sleep(2);
            break;
        }
        usleep(100000);
    }
}

void printSquare (int edge, int loc_x, int loc_y, Color C) {
    long int location;
    int i,j;
    if (((loc_x)>=0) && ((loc_x + edge)<vinfo.xres) && ((loc_y)>=0) && ((loc_y + edge)<vinfo.yres)) {
		for (i = loc_x; i < (loc_x+edge); i++) {
			for (j = loc_y; j < (loc_y+edge); j++) {
				location = (i+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (j+vinfo.yoffset) * finfo.line_length;
				
				if (fbp + location) { //check for segmentation fault
					if (vinfo.bits_per_pixel == 32) {
						*(fbp + location) = C.a;            //Blue
						*(fbp + location + 1) = C.r;        //Green
						*(fbp + location + 2) = C.g;        //Red
						*(fbp + location + 3) = C.b;          //Transparancy
					} else  { //assume 16bpp
						int r = C.a;     //Red
						int g = C.r;     //Green
						int b = C.g;     //Blue
						
						unsigned short int t = r<<11 | g << 5 | b;
						*((unsigned short int*)(fbp + location)) = t;
					}
				} else {
					return;
				}
			}
		}
	}
}

void plot8pixel (Point P, int p, int q, int W, Color C) {
    printSquare(W, P.x+p, P.y+q, C);
    printSquare(W, P.x-p, P.y+q, C);
    printSquare(W, P.x+p, P.y-q, C);
    printSquare(W, P.x-p, P.y-q, C);

    printSquare(W, P.x+q, P.y+p, C);
    printSquare(W, P.x-q, P.y+p, C);
    printSquare(W, P.x+q, P.y-p, C);
    printSquare(W, P.x-q, P.y-p, C);
}


void plot4pixel (Point P, int p, int q, int W, Color C) {
    //printSquare(W, P.x+p, P.y+q, C);
    //printSquare(W, P.x-p, P.y+q, C);
    printSquare(W, P.x+p, P.y-q, C);
    printSquare(W, P.x-p, P.y-q, C);

    //printSquare(W, P.x+q, P.y+p, C);
    //printSquare(W, P.x-q, P.y+p, C);
    printSquare(W, P.x+q, P.y-p, C);
    printSquare(W, P.x-q, P.y-p, C);
}

void drawCircle (int radius, Point P, int W, Color C) {
    int d, p, q;

    p = 0;
    q = radius;
    d = 3 - 2*radius;

    plot8pixel(P, p, q, W, C);

    while (p < q) {
        p++;
        if (d<0) {
            d = d + 4*p + 6;
        }
        else {
            q--;
            d = d + 4*(p-q) + 10;
        }

        plot8pixel(P, p, q, W, C);
    }
}

void drawSolidCircle (int radius, Point P, int W, Color C){
	Point center; setPoint(&center,P.x+radius/2,P.y + radius/2);
	drawCircle(radius,P,W,C);
	solidFill(&center,C);
	
}

void drawHalfSolidCircle (int radius, Point P, int W, Color C){
	int d, p, q;

    p = 0;
    q = radius;
    d = 3 - 2*radius;

    plot4pixel(P, p, q, W, C);
    while (p < q) {
        p++;
        if (d<0) {
            d = d + 4*p + 6;
        }
        else {
            q--;
            d = d + 4*(p-q) + 10;
        }
		plot4pixel(P, p, q, W, C);
    }
    Point p1; setPoint(&p1,P.x+W+radius,P.y);
    Point p2; setPoint(&p2,P.x-W-radius,P.y);
    drawLine(&p1,&p2,&C);
    setPoint(&p1,P.x,P.y-radius/2);
    drawLine(&p1,&p2,&C);
    solidFill(&p1,C);
}

void drawCircleProjectory(Point start, Point finish, int deltax, int deltay, int p){
	Color c;
	setColor(&c, 255, 0, 0);
	Color cdel;
	setColor(&cdel, 0, 0, 0);
	int p1;
	if(start.y == finish.y){
		if(start.x<=finish.x){
			//printf("babx\n");
			while(start.x <= finish.x){
				drawCircle(12, start, 2, c);
				usleep(5000);
				drawCircle(12, start, 2, cdel);
				start.x+=3;
			}
		}
	}
	else if(start.x == finish.x){
		if(start.y<=finish.y){
			while(start.y <= finish.y){
				long location = (start.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (start.y+vinfo.yoffset) * finfo.line_length;
				drawCircle(12, start, 2, c);
				usleep(5000);
				drawCircle(12, start, 2, cdel);
				start.y+=3;
			}
		}
	}
	else{
		drawCircle(12, start, 2, c);
		usleep(5000);
		drawCircle(12, start, 2, cdel);
		if(p>=0){
			start.x++;
			start.y++;
			p1 = p + 2*deltay - 2*deltax;
		}
		else{
			start.x++;
			p1 = p + 2*deltay;
		}
		drawCircleProjectory(start,finish,deltax,deltay,p1);
	}
}

//gambar garis naik start.y > finish.y
void drawCircleProjectory2(Point start, Point finish, int deltax, int deltay, int p){
	Color c;
	setColor(&c, 255, 0, 0);
	Color cdel;
	setColor(&cdel, 0, 0, 0);
	int p1;
	if(start.y == finish.y){
		if(start.x<=finish.x){
			while(start.x <= finish.x){
				drawCircle(12, start, 2, c);
				usleep(5000);
				drawCircle(12, start, 2, cdel);
				start.x++;
			}
		}
	}
	else if(start.x == finish.x){
		if(start.y>=finish.y){
			while(start.y >= finish.y){
				drawCircle(12, start, 2, c);
				usleep(5000);
				drawCircle(12, start, 2, cdel);
				start.y--;
			}
		}
	}
	else{
		drawCircle(12, start, 2, c);
		usleep(5000);
		drawCircle(12, start, 2, cdel);
		if(p>=0){
			start.x++;
			start.y--;
			p1 = p + 2*deltay - 2*deltax;
		}
		else{
			start.x++;
			p1 = p + 2*deltay;
		}
		drawCircleProjectory2(start,finish,deltax,deltay,p1);
	}
}

void * drawFallingWheels(void * p22){	
	Point * p = (Point *) p22;
	Point start = *p;
	Point finish;
	finish.x = start.x;
	finish.y = vinfo.yres-1;
	
	//rumus bersenham itu
	int deltay = finish.y - start.y;
	int deltax = finish.x - start.x;
	int px = 2*deltay -deltax;
	
	//kalo start y > finish y pake drawline2
	if(deltay < 0){
		deltay = deltay * -1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
	}
	else{
		drawCircleProjectory(start,finish,deltax,deltay,px);
	}
	
	
	int distx = (vinfo.xres-1 - start.x)/2;
	int disty = (vinfo.xres-1 - start.x)/2;
	distx -= 1;
	disty -= 1;
	
	start = finish;
	finish.x = finish.x + distx/2;
	finish.y = finish.y - disty/2;
	
	deltay = finish.y - start.y;
	deltax = finish.x - start.x;
	px = 2*deltay -deltax;
	if(deltay < 0){
		deltay = deltay * -1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
		start = finish;
		finish.x = finish.x + distx/2;
		finish.y = vinfo.yres-1;
		drawCircleProjectory(start,finish,deltax,deltay,px);
	}
	else{
		drawCircleProjectory(start,finish,deltax,deltay,px);
		start = finish;
		finish.x = finish.x + distx/2;
		finish.y = vinfo.yres-1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
	}
	
	start = finish;
	finish.x = finish.x + distx/4;
	finish.y = finish.y - disty/4;
	
	deltay = finish.y - start.y;
	deltax = finish.x - start.x;
	px = 2*deltay -deltax;
	if(deltay < 0){
		deltay = deltay * -1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
		start = finish;
		finish.x = finish.x + distx/4;
		finish.y = vinfo.yres-1;
		drawCircleProjectory(start,finish,deltax,deltay,px);
	}
	else{
		drawCircleProjectory(start,finish,deltax,deltay,px);
		start = finish;
		finish.x = finish.x + distx/4;
		finish.y = vinfo.yres-1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
	}
	
	start = finish;
	finish.x = finish.x + distx/4;
	finish.y = finish.y - disty/4;
	
	deltay = finish.y - start.y;
	deltax = finish.x - start.x;
	px = 2*deltay -deltax;
	if(deltay < 0){
		deltay = deltay * -1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
		start = finish;
		finish.x = finish.x + distx/4;
		finish.y = vinfo.yres-1;
		drawCircleProjectory(start,finish,deltax,deltay,px);
	}
	else{
		drawCircleProjectory(start,finish,deltax,deltay,px);
		start = finish;
		finish.x = finish.x + distx/4;
		finish.y = vinfo.yres-1;
		drawCircleProjectory2(start,finish,deltax,deltay,px);
	}
}

void drawPlaneBreak(Point* plane) {
	Color cDestroy;
	setColor(&cDestroy, 255, 255, 0);
	int i, j, ret;
	Point* planeBreak1;
	Point* planeBreak2;
	Point* planeBreak3;
	Point firepoint1, firepoint2, firepoint3;
	Point pivot1, pivot2, pivot3;
	planeBreak1 = (Point*) malloc (4 * sizeof(Point));
	planeBreak2 = (Point*) malloc (4 * sizeof(Point));
	planeBreak3 = (Point*) malloc (4 * sizeof(Point));

	//Buat bagian plane 1
	setPoint(&planeBreak1[0], plane[0].x, plane[0].y);
	setPoint(&planeBreak1[1], plane[1].x, plane[1].y);
	setPoint(&planeBreak1[2], plane[1].x+50, plane[2].y);
	setPoint(&planeBreak1[3], plane[0].x+100, plane[0].y);
	for(i = 0; i < 3; i++) {
        drawLine(&planeBreak1[i], &planeBreak1[i + 1], &cDestroy);
    }
    drawLine(&planeBreak1[3], &planeBreak1[0], &cDestroy);
    //Warnai
    setPoint(&firepoint1, planeBreak1[1].x, planeBreak1[1].y+20);
    solidFill(&firepoint1, cDestroy);

	//Buat bagian plane 2
	setPoint(&planeBreak2[0], planeBreak1[2].x+50, planeBreak1[2].y);
	setPoint(&planeBreak2[1], plane[2].x+50, plane[2].y);
	setPoint(&planeBreak2[2], plane[5].x+50, plane[5].y);
	setPoint(&planeBreak2[3], planeBreak1[3].x+50, planeBreak1[3].y);
	for(i = 0; i < 3; i++) {
        drawLine(&planeBreak2[i], &planeBreak2[i + 1], &cDestroy);
    }
    drawLine(&planeBreak2[3], &planeBreak2[0], &cDestroy);
    //Warnai
    setPoint(&firepoint2, planeBreak2[0].x+50, planeBreak2[0].y+20);
    solidFill(&firepoint2, cDestroy);
    

    //Buat bagian plane 3
	setPoint(&planeBreak3[0], planeBreak2[1].x+50, planeBreak2[1].y);
	setPoint(&planeBreak3[1], plane[3].x+100, plane[3].y);
	setPoint(&planeBreak3[2], plane[4].x+100, plane[4].y);
	setPoint(&planeBreak3[3], planeBreak2[2].x+50, planeBreak2[2].y);
	for(i = 0; i < 3; i++) {
        drawLine(&planeBreak3[i], &planeBreak3[i + 1], &cDestroy);
    }
    drawLine(&planeBreak3[3], &planeBreak3[0], &cDestroy);
    //Warnai
    setPoint(&firepoint3, planeBreak3[0].x+10, planeBreak3[0].y);
    solidFill(&firepoint3, cDestroy);
    usleep(400000);
    clearFill(&firepoint1, cDestroy);
    clearFill(&firepoint2, cDestroy);
    clearFill(&firepoint3, cDestroy);

    //Jatuhkan
    struct readFallSpinParams readparams1;
    struct readFallSpinParams readparams2;
    struct readFallSpinParams readparams3;
    
    setPoint(&pivot1, planeBreak1[0].x+20, planeBreak1[0].y-10);
    readparams1.p = planeBreak1;
    readparams1.firepoint=firepoint1;
    readparams1.c= cDestroy;
    readparams1.pivot = pivot1;

    setPoint(&pivot2, planeBreak2[0].x+10, planeBreak2[0].y+10);
    readparams2.p = planeBreak2;
    readparams2.firepoint=firepoint2;
    readparams2.c= cDestroy;
    readparams2.pivot = pivot2;

    setPoint(&pivot3, planeBreak3[0].x+10, planeBreak3[0].y);
    readparams3.p = planeBreak3;
    readparams3.firepoint=firepoint3;
    readparams3.c= cDestroy;
    readparams3.pivot = pivot3;

    pthread_t thrfd1, thrfd2, thrfd3, thrFallWheel, thrFallWheel2;
    // ret *= pthread_create(&thrFallWheel, NULL, drawFallingWheels, &roda1);
    pthread_create(&thrFallWheel, NULL, drawFallingWheels, &roda1);
    pthread_create(&thrFallWheel2, NULL, drawFallingWheels, &roda2);
    usleep(400000);
    pthread_create(&thrfd1, NULL, fallLeftSpin, &readparams1);
	pthread_create(&thrfd2, NULL, fallSpin, &readparams2);
	pthread_create(&thrfd3, NULL, fallRightSpin, &readparams3);
    
	// if (!ret) {
		pthread_join(thrfd1, NULL);
		pthread_join(thrfd2, NULL);
		pthread_join(thrfd3, NULL);
		pthread_join(thrFallWheel, NULL);
    	pthread_join(thrFallWheel2, NULL);
	// }

}


void* drawPlane() {
	Color cDestroy;
	setColor(&cDestroy, 255, 255, 255);
	int i;
	Point temp, temp1, temp2;
	Point circle, circle1;
    Color c, cDel;
    setColor(&c, 255, 0, 0);
    Point* plane;
    plane = (Point*) malloc(6 * sizeof(Point));
    int lengthPlane = 260; // panjang pesawat dari head sampai tail
    tailPlane = vinfo.xres;
    headPlane = tailPlane - lengthPlane;
   
    int j;
    Point* sayap;

    sayap = (Point*) malloc(4* sizeof(Point));
    while (!kaboom) { // selama pesawat belum ketembak
        while (headPlane > 0) { // selama pesawat belum mentok di kiri
            if (kaboom) { // pesawat kena tembak
            	clearScreen(&bg);
            	//drawBoxgun();
                //Buat poligon tertembak
                drawPlaneBreak(plane);
                sleep(1);
                break;
            } else { // masih terbang
            	// hapus
            	circle.x = headPlane+40;
				circle.y = 90;
				circle1.x = headPlane+140;
				circle1.y = 90;
                for(j = 0; j < 5; j++) {
                    drawLine(&plane[j], &plane[j + 1], &bg);
                }
               // drawCircle(12, circle, 2, bg);
                drawLine(&plane[5], &plane[0], &bg);
                

                //bikin sayap
                setPoint(&sayap[0], headPlane+60, 70);
                setPoint(&sayap[1], headPlane+100, 10);
                setPoint(&sayap[2], headPlane+130, 10);
                setPoint(&sayap[3], headPlane+90, 70);
                for(j = 0; j < 3; j++) {
                    drawLine(&sayap[j], &sayap[j + 1], &c);
                }               
                drawLine(&sayap[3], &sayap[0], &c);
                setPoint(&temp, sayap[1].x, sayap[1].y+20);
                solidFill(&temp, c);

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
                drawCircle(12, circle, 2, c);
                drawCircle(13, circle, 2, bg);
                drawCircle(12, circle1, 2, c);
                drawCircle(13, circle1, 2, bg);
                roda1 = circle;
                roda2 = circle1;
                drawLine(&plane[5], &plane[0], &c);
                
				//circle.x = 90 + 260;
				//circle.y = 100 + 3;
				
                // Warnai
                setPoint(&temp2, plane[1].x, 60);
                solidFill(&temp2, c);
                setPoint(&temp, plane[3].x, 30);
                solidFill(&temp, c);
                //maenin usleepnya aja, 50000 lmyn ga kedip tp lama -abe1.0
                usleep(20000);
                solidFillReverse(&temp, c);
                solidFillReverse(&temp2, c);

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
        //tes
    }
}

void drawBox(Point position, int width, int height, Color c, int Degree) {
	Point* box;
    Point boxFirePoint, temp;
    box = (Point*) malloc(4*sizeof(Point));
    int i;
    
    setPoint(&box[0], position.x, position.y);
    setPoint(&box[1], position.x + width, position.y);
    setPoint(&box[2], position.x + width, position.y + height);
    setPoint(&box[3], position.x, position.y + height);
    for(i = 0; i < 3; i++) {
        spinDegreeUsingCenter(&box[i+1],Degree,box[0]);
        drawLine(&box[i], &box[i + 1], &c);
    }
    drawLine(&box[0], &box[i], &c);
    if (width != 1) {
		setPoint(&boxFirePoint, (box[0].x+box[2].x)/2, (box[0].y+box[2].y)/2);
		solidFill(&boxFirePoint, c);
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
	Point bottomGun, mouthGun, boxFirePoint;
	Color c; setColor(&c, 0, 255, 255);
	Color cBox; setColor(&cBox, 0, 255, 0);
	Point* box = (Point*) malloc(4*sizeof(Point));
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
    while (!kaboom) { // berhenti hanya jika di interrupt master
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
        
        //Gambar senjata
        setPoint(&mouthGun, srcXBeam, srcYBeam);
        setPoint(&box[0], srcXBeam - 4, srcYBeam);
		setPoint(&box[1], srcXBeam + 4, srcYBeam);
		setPoint(&box[2], vinfo.xres / 2  + 4, vinfo.yres -40);
		setPoint(&box[3], vinfo.xres/2 - 4, vinfo.yres -40);
		for(i = 0; i < 3; i++) {
			drawLine(&box[i], &box[i + 1], &cBox);
		}
		drawLine(&box[0], &box[i], &cBox);
		setPoint(&boxFirePoint, (vinfo.xres/2+ srcXBeam)/2, (srcYBeam+vinfo.yres-40)/2);
		solidFill(&boxFirePoint, cBox);
        usleep(2000);
        // hapus
        for(i = 0; i < 3; i++) {
			drawLine(&box[i], &box[i + 1], &bg);
		}
		drawLine(&box[0], &box[i], &bg);
		solidFill(&boxFirePoint, bg);
    }
}

void drawPeopleWithParachute(Point initialPosition) {
	Point temp;
	Color faceColor; setColor(&faceColor, 0, 255, 0);
	Color black; setColor(&black, 0, 0, 0);
	setPoint(&temp,initialPosition.x, initialPosition.y-50); //parachute
	drawHalfSolidCircle(120,temp,1,faceColor);
	drawSolidCircle(20,initialPosition,2,faceColor);
	setPoint(&temp,initialPosition.x, initialPosition.y+22); // body
	drawBox(temp,4,50,faceColor,0);
	setPoint(&temp,temp.x, temp.y+50); //waist
	drawBox(temp,4,20,faceColor,30);
	drawBox(temp,4,20,faceColor,-30);
	setPoint(&temp,temp.x+10,temp.y+17); // right foot
	drawBox(temp,4,20,faceColor,0);
	setPoint(&temp,temp.x-19,temp.y); // right foot
	drawBox(temp,4,20,faceColor,0);
	setPoint(&temp,initialPosition.x, initialPosition.y+35); //arm
	drawBox(temp,4,20,faceColor,100);
	drawBox(temp,4,20,faceColor,-100);
	setPoint(&temp,temp.x+20, temp.y-5); //right hand
	drawBox(temp,4,30,faceColor,-150);
	setPoint(&temp,temp.x-38, temp.y+1); //left hand
	drawBox(temp,4,30,faceColor,150);
	setPoint(&temp,initialPosition.x, initialPosition.y+35); //parachute string
	drawBox(temp,1,100,faceColor,148);
	drawBox(temp,1,140,faceColor,127);
	drawBox(temp,1,100,faceColor,-148);
	drawBox(temp,1,140,faceColor,-127);
}

void peopleFall(Point p){
	Point temp; setPoint(&temp,p.x,p.y);
	while (temp.y < (vinfo.yres - 120)) {
		drawPeopleWithParachute(temp);
		usleep(20000);
		printSquare(300,temp.x-150,temp.y-180,bg);
		setPoint(&temp,temp.x,temp.y+5);
	}
}

int getAbsisInlineEquation(Point p1, Point p2, int ordinat) {

	float temp, x;
	x =  (float) (ordinat-p1.y);
	x = (ordinat-p1.y)/ (float) (p2.y-p1.y);
	temp = ((x)*(p2.x-p1.x)+p1.x);
	return (int) temp;
}

void* drawBeam() {
	Point srcBeam, destBeam, backOfBullet, fixSrcBeam, boxFirePoint;
	int tempXBeam, tempYBeam, tempDXBeam,i;
    Color c; setColor(&c, 255, 0, 255);
    char stroke;
    Point* box;
    box = (Point*) malloc(4*sizeof(Point));
    while(!kaboom) {
        stroke = fgetc(stdin);
        if (stroke == 10) { // ENTER
			tempXBeam = srcXBeam;
			tempYBeam = srcYBeam;
            setPoint(&fixSrcBeam, srcXBeam, srcYBeam);
            setPoint(&destBeam, vinfo.xres / 2, vinfo.yres -40);
            tempDXBeam = getAbsisInlineEquation(fixSrcBeam,destBeam, 0);
            setPoint(&destBeam, tempDXBeam, 0);            
            while (tempYBeam > 100) {
				setPoint(&backOfBullet, tempXBeam, tempYBeam);
				tempYBeam -= 5;
				tempXBeam = getAbsisInlineEquation(fixSrcBeam, destBeam,tempYBeam);
				setPoint(&srcBeam, tempXBeam, tempYBeam);
				if (srcBeam.x > backOfBullet.x) {
					setPoint(&box[0], backOfBullet.x, srcBeam.y);
					setPoint(&box[1], srcBeam.x, srcBeam.y);
					setPoint(&box[2], srcBeam.x,backOfBullet.y);
					setPoint(&box[3], backOfBullet.x, backOfBullet.y);
				} else {
					setPoint(&box[0], srcBeam.x, srcBeam.y);
					setPoint(&box[1], backOfBullet.x, srcBeam.y);
					setPoint(&box[2], backOfBullet.x,backOfBullet.y);
					setPoint(&box[3], srcBeam.x, backOfBullet.y);
				}
				
				setPoint(&boxFirePoint, (srcBeam.x+backOfBullet.x)/2, (srcBeam.y+backOfBullet.y)/2);
				
				for(i = 0; i < 3; i++) {
					drawLine(&box[i], &box[i + 1], &c);
				}
				drawLine(&box[0], &box[i], &c);
				//solidFill(&boxFirePoint, c);
				usleep(20000);
				for(i = 0; i < 3; i++) {
					drawLine(&box[i], &box[i + 1], &bg);
				}
				drawLine(&box[0], &box[i], &bg);
				//solidFill(&boxFirePoint, bg);
			}
            if ((destBeam.x > headPlane+20) && (destBeam.x < tailPlane+20)) { // kena
                //drawPlane();
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
    Point P; setPoint(&P,300,300);
    peopleFall(P);
    /*
    pthread_t thrPlane, thrLasergun, thrBeam, thrFallWheel, thrFallWheel2;
    pthread_create(&thrPlane, NULL, drawPlane, "thrPlane");
    pthread_create(&thrLasergun, NULL, drawLasergun, "thrLasergun");
    pthread_create(&thrBeam, NULL, drawBeam, "thrBeam");
    pthread_join(thrBeam, NULL); // sampe berhasil nembak
    pthread_join(thrPlane, NULL);
    pthread_cancel(thrLasergun);
    clearScreen(&bg);
    pthread_create(&thrFallWheel, NULL, drawFallingWheels, &roda1);
    pthread_create(&thrFallWheel2, NULL, drawFallingWheels, &roda2);
    pthread_join(thrFallWheel, NULL);
    pthread_join(thrFallWheel2, NULL);*/
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

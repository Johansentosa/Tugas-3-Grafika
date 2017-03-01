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


void drawClip(Point pointClipLeftUp, Point pointClipRightDown, Color c) {
	Point ptemp1, ptemp2;
    setPoint(&ptemp1, pointClipRightDown.x, pointClipLeftUp.y);
    setPoint(&ptemp2, pointClipLeftUp.x, pointClipRightDown.y);
	drawLine(&pointClipLeftUp, &ptemp1, &c);
    drawLine(&ptemp1, &pointClipRightDown, &c);
    drawLine(&pointClipRightDown, &ptemp2, &c);
    drawLine(&ptemp2, &pointClipLeftUp, &c);
}


void* moveClip() {
	while (1) {
		char stroke = getch();
		if (stroke == 'a') {
			drawClip(pointClipLeftUp, pointClipRightDown, bg);
			if (pointClipLeftUp.x-moveFactor > 1) {
				setPoint(&pointClipLeftUp, pointClipLeftUp.x-moveFactor, pointClipLeftUp.y);
		    	setPoint(&pointClipRightDown, pointClipRightDown.x-moveFactor, pointClipRightDown.y);
		    	drawClip(pointClipLeftUp, pointClipRightDown, cBorder);
	    	}
		} else
		if (stroke == 'w') {
			drawClip(pointClipLeftUp, pointClipRightDown, bg);
			if (pointClipLeftUp.y-moveFactor > 1) {
				setPoint(&pointClipLeftUp, pointClipLeftUp.x, pointClipLeftUp.y-moveFactor);
		    	setPoint(&pointClipRightDown, pointClipRightDown.x, pointClipRightDown.y-moveFactor);
				drawClip(pointClipLeftUp, pointClipRightDown, cBorder);
	    	}
		} else
		if (stroke == 'd') {
			drawClip(pointClipLeftUp, pointClipRightDown, bg);
			if (pointClipRightDown.x+moveFactor < vinfo.xres-1) {
				setPoint(&pointClipLeftUp, pointClipLeftUp.x+moveFactor, pointClipLeftUp.y);
		    	setPoint(&pointClipRightDown, pointClipRightDown.x+moveFactor, pointClipRightDown.y);
				drawClip(pointClipLeftUp, pointClipRightDown, cBorder);
	    	}
		} else
		if (stroke == 'x') {
			drawClip(pointClipLeftUp, pointClipRightDown, bg);
			if (pointClipRightDown.y+moveFactor < vinfo.yres-1) {
				setPoint(&pointClipLeftUp, pointClipLeftUp.x, pointClipLeftUp.y+moveFactor);
		    	setPoint(&pointClipRightDown, pointClipRightDown.x, pointClipRightDown.y+moveFactor);
				drawClip(pointClipLeftUp, pointClipRightDown, cBorder);
	    	}
		}
	}
}


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <pthread.h>


typedef struct {
    int x; //absis
    int y; //ordinate
}Point;

typedef struct Colors {
    char a;
    char r;
    char g;
    char b;
} Color;

int life = 30;
int fbfd = 0;                       //Filebuffer Filedescriptor
struct fb_var_screeninfo vinfo;     //Struct Variable Screeninfo
struct fb_fix_screeninfo finfo;     //Struct Fixed Screeninfo
long int screensize = 0;            //Ukuran data framebuffer
char *fbp = 0;                      //Framebuffer di memori internal

int slope(Point p1, Point p2){
    int s;
    s = p2.y-p1.y / p2.x -p1.x;
    return s;
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

void plotPixelRGBA(int _x, int _y, int r, int g, int b, int a) {

        long int location = (_x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (_y+vinfo.yoffset) * finfo.line_length;
        *(fbp + location) = b;        //blue
        *(fbp + location + 1) = g;    //green
        *(fbp + location + 2) = r;    //red
        *(fbp + location + 3) = a;      //
}

int checkColorB(int _x, int _y, unsigned char nb) {
        long int location = (_x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (_y+vinfo.yoffset) * finfo.line_length;
		unsigned char b = *((unsigned char *) (fbp + location));        //blue
		if (nb == b) {
			return 1;
		}
		return 0;
}

int check4points(Point center, int x, int y, unsigned char nb) {
	int b = 0;
	b |= checkColorB(center.x + x, center.y + y, nb);
	b |= checkColorB(center.x - x, center.y + y, nb);
	b |= checkColorB(center.x + x, center.y - y, nb);
	b |= checkColorB(center.x - x, center.y - y, nb);
	return b;
}

int check8points(Point center, int x, int y, unsigned char nb) {
	return (check4points(center, x, y, nb) || check4points(center, y, x, nb));
}

int checkCircleCollision(Point center, int radius, unsigned char nb) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;
    /* plot first set of point */
    if (check8points(center, x, y, nb)) {
		return 1;
	}

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        if (check8points(center, x, y, nb)) {
			return 1;
		}
    }
    return 0;
}

void drawLineNegative(Point p1, Point p2, int clear, int dot){
    int xold = 50;
    int yold = 50;
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    int p = 2*dy - dx;
    int x = p2.x; int y = p2.y;
    int print = 1;
    
    if (abs(dx) > abs(dy)) {
        while (x != p1.x) {
            if (p < 0) {
                p = p + dy + dy;
                x--;
            }
            else {
                p = p + dy + dy - dx - dx;
                x--;
                y--;
            }
            
			if (dot!=0 && clear==1) {
				long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b1 = *((unsigned char *) (fbp + location));        //blue
				long int location2 = (x+1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b2 = *((unsigned char *) (fbp + location2));        //blue
				long int location3 = (x-1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b3 = *((unsigned char *) (fbp + location3));        //blue
				if (b1 != 0 || b2 != 0 || b3 != 0) {
					break;
				}
			}
            if (dot!=0 && ((x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 2025)) continue;
            if ((clear == 3) && (x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 3025) continue;
            if (dot!=0 && x%dot == 0) print = !print;
            if (!print) continue;

            if (clear == 1) {
              plotPixelRGBA(x,y,255,255,255,0);
            }
            else if (clear == 2) {
              plotPixelRGBA(x,y,255,0,0,0);
            }
            else if (clear == 3) {
              Point few = {xold, yold};
              Point pew = {x,y};
              drawCircle(few, 10, 0);
              drawCircle(pew, 10, 2);
			  usleep(1000);
			  if (checkCircleCollision(pew, 10, 200)) {
				  drawCircle(pew, 10, 0);
				  drawExplosion(pew, 15, 2);
				  usleep(50000);
				  drawExplosion(pew, 15, 0);
				  life--;
				  break;
			  }
              xold = x;
              yold = y;
              if (y < 15) {
                drawCircle(pew, 10, 0);
                break;
              }
            }
			else if (clear == 4) {
			  plotPixelRGBA(x,y,200,200,200,0);
			}
            else {
              plotPixelRGBA(x,y,0,0,0,0);
            }
        }
    } else {
        p = 2*dx -dy;
        while (y != p1.y) {
            if (p < 0) {
                p = p + dx + dx;
                y--;
            }
            else {
                p = p + dx + dx - dy - dy;
                y--;
                x--;
            }
            
			if (dot!=0 && clear==1) {
				long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b1 = *((unsigned char *) (fbp + location));        //blue
				long int location2 = (x+1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b2 = *((unsigned char *) (fbp + location2));        //blue
				long int location3 = (x-1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b3 = *((unsigned char *) (fbp + location3));        //blue
				if (b1 != 0 || b2 != 0 || b3 != 0) {
					break;
				}
			}
            if (dot!=0 && ((x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 2025)) continue;
            if ((clear == 3) && (x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 3025) continue;
            if (dot!=0 && y%dot == 0) print = !print;
            if (!print) continue;

            if (clear == 1) {
              plotPixelRGBA(x,y,255,255,255,0);
            }
            else if (clear == 2) {
              plotPixelRGBA(x,y,255,0,0,0);
            }
            else if (clear == 3) {
              Point few = {xold, yold};
              Point pew = {x,y};
              drawCircle(few, 10, 0);
              drawCircle(pew, 10, 2);
			  usleep(1000);
			  if (checkCircleCollision(pew, 10, 200)) {
				  drawCircle(pew, 10, 0);
				  drawExplosion(pew, 15, 2);
				  usleep(50000);
				  drawExplosion(pew, 15, 0);
				  life--;
				  break;
			  }
              xold = x;
              yold = y;
              if (y < 15) {
                drawCircle(pew, 10, 0);
                break;
              }
            }
			else if (clear == 4) {
			  plotPixelRGBA(x,y,200,200,200,0);
			}
            else {
              plotPixelRGBA(x,y,0,0,0,0);
            }
        }
    }

}

void drawLinePositive(Point p1, Point p2, int clear, int dot){
    int xold = 50;
    int yold = 50;
    int dx = p2.x - p1.x;
    int dy = p1.y - p2.y;
    int p = 2*dy - dx;
    int x = p1.x; int y = p1.y;
    int print = 1;
    
    if (abs(dx) > abs(dy)) {
        while (x != p2.x) {
            if (p < 0) {
                p = p + dy + dy;
                x++;
            }
            else {
                p = p + dy + dy - dx - dx;
                x++;
                y--;
            }
			if (dot!=0 && clear==1) {
				long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b1 = *((unsigned char *) (fbp + location));        //blue
				long int location2 = (x+1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b2 = *((unsigned char *) (fbp + location2));        //blue
				long int location3 = (x-1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b3 = *((unsigned char *) (fbp + location3));        //blue
				if (b1 != 0 || b2 != 0 || b3 != 0) {
					break;
				}
			}
            if (dot!=0 && ((x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 2025)) continue;
            if ((clear == 3) && (x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 3025) continue;
            if (dot!=0 && x%dot == 0) print = !print;
            if (!print) continue;

            if (clear == 1) {
              plotPixelRGBA(x,y,255,255,255,0);
            }
            else if (clear == 2) {
              plotPixelRGBA(x,y,255,0,0,0);
            }
            else if (clear == 3) {
              Point few = {xold, yold};
              Point pew = {x,y};
              drawCircle(few, 10, 0);
              drawCircle(pew, 10, 2);
			  usleep(1000);
			  if (checkCircleCollision(pew, 10, 200)) {
				  drawCircle(pew, 10, 0);
				  drawExplosion(pew, 15, 2);
				  usleep(50000);
				  drawExplosion(pew, 15, 0);
				  life--;
				  break;
			  }
              xold = x;
              yold = y;
              if (y < 15) {
                drawCircle(pew, 10, 0);
                break;
              }
            }
			else if (clear == 4) {
			  plotPixelRGBA(x,y,200,200,200,0);
			}
            else {
              plotPixelRGBA(x,y,0,0,0,0);
            }
        }
    } else {
        p = 2*dx - dy;
        y = p1.y;
        while (y != p2.y) {
            if (p < 0) {
                p = p + dx + dx;
                y--;
            }
            else {
                p = p + dx + dx - dy - dy;
                y--;
                x++;
            }
			if (dot!=0 && clear==1) {
				long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b1 = *((unsigned char *) (fbp + location));        //blue
				long int location2 = (x+1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b2 = *((unsigned char *) (fbp + location2));        //blue
				long int location3 = (x-1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
				unsigned char b3 = *((unsigned char *) (fbp + location3));        //blue
				if (b1 != 0 || b2 != 0 || b3 != 0) {
					break;
				}
			}
            if (dot!=0 && ((x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 2025)) continue;
            if ((clear == 3) && (x-vinfo.xres/2)*(x-vinfo.xres/2)+(vinfo.yres-y)*(vinfo.yres-y) <= 3025) continue;
            if (dot!=0 && y%dot == 0) print = !print;
            if (!print) continue;

            if (clear == 1) {
              plotPixelRGBA(x,y,255,255,255,0);
            }
            else if (clear == 2) {
              plotPixelRGBA(x,y,255,0,0,0);
            }
            else if (clear == 3) {
              Point few = {xold, yold};
              Point pew = {x,y};
              drawCircle(few, 10, 0);
              drawCircle(pew, 10, 2);
			  usleep(1000);
			  if (checkCircleCollision(pew, 10, 200)) {
				  drawCircle(pew, 10, 0);
				  drawExplosion(pew, 15, 2);
				  usleep(50000);
				  drawExplosion(pew, 15, 0);
				  life--;
				  break;
			  }
              xold = x;
              yold = y;
              if (y < 15) {
                drawCircle(pew, 10, 0);
                break;
              }
            }
			else if (clear == 4) {
			  plotPixelRGBA(x,y,200,200,200,0);
			}
            else {
              plotPixelRGBA(x,y,0,0,0,0);
            }
        }
    }

}

void drawHorizontalLine(Point p1, Point p2, int clear, int dot){
    //plotPixelRGBA(x1,y1,0,0,0,0);
    int print = 1;
    for(int i = p1.x ; i <= p2.x; i++){
        if (dot!=0 && i%dot == 0) print = !print;
        if (!print) continue;
        if (clear == 1) {
          plotPixelRGBA(i,p1.y,255,255,255,0);
        }
        else if (clear == 2) {
          plotPixelRGBA(i,p1.y,255,0,0,0);
        }
        else if (clear == 4) {
          plotPixelRGBA(i,p1.y,200,200,200,0);
        }
        else {
          plotPixelRGBA(i,p1.y,0,0,0,0);
        }
    }
}

void drawVerticalLine(Point p1, Point p2, int clear, int dot){
    int xold = 150;
    int yold = 150;
    int print = 1;
    for (int i=p2.y; i>=p1.y; i--){
		if (dot!=0 && clear==1) {
			long int location = (p1.x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (i+vinfo.yoffset) * finfo.line_length;
			unsigned char b1 = *((unsigned char *) (fbp + location));        //blue
			long int location2 = (p1.x+1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (i+vinfo.yoffset) * finfo.line_length;
			unsigned char b2 = *((unsigned char *) (fbp + location2));        //blue
			long int location3 = (p1.x-1+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (i+vinfo.yoffset) * finfo.line_length;
			unsigned char b3 = *((unsigned char *) (fbp + location3));        //blue
			if (b1 != 0 || b2 != 0 || b3 != 0) {
				break;
			}
		}
        if (dot!=0 && ((p1.x-vinfo.xres/2)*(p1.x-vinfo.xres/2)+(vinfo.yres-i)*(vinfo.yres-i) <= 2025)) continue;
        if ((clear == 3) && ((p1.x-vinfo.xres/2)*(p1.x-vinfo.xres/2)+(vinfo.yres-i)*(vinfo.yres-i) <= 3025)) continue;
        if (dot!=0 && i%dot == 0) print = !print;
        if (dot!=0 && !print) continue;
        
        if (clear ==1){
            plotPixelRGBA(p1.x,i,255,255,255,0);
        }
        else if (clear == 2) {
            plotPixelRGBA(p1.x,i,255,0,0,0);
        }
        else if (clear == 3) {
          Point few = {xold, yold};
          Point pew = {p1.x, i-10};
          drawCircle(few, 10, 0);
          drawCircle(pew, 10, 2);
          usleep(1000);
          if (checkCircleCollision(pew, 10, 200)) {
			  drawCircle(pew, 10, 0);
			  drawExplosion(pew, 15, 2);
			  usleep(50000);
			  drawExplosion(pew, 15, 0);
			  life--;
			  break;
		  }
          xold = p1.x;
          yold = i-10;
		  if (i-10 < 15) {
			drawCircle(pew, 10, 0);
			break;
		  }
        }
        else if (clear == 4) {
          plotPixelRGBA(p1.x,i,200,200,200,0);
        }
        else {
            plotPixelRGBA(p1.x,i,0,0,0,0);
        }
    }   
}

void drawLine(Point p1, Point p2, int clear){
    if (p1.x > p2.x) {
        drawLine(p2, p1, clear);
    }
    else if (p1.x == p2.x) {
        drawVerticalLine(p1, p2, clear,0);
    }
    else if (p1.y == p2.y){
        drawHorizontalLine(p1, p2, clear,0);
    }
    else if ((p2.x > p1.x) && (p2.y > p1.y)) {
        drawLineNegative(p1, p2, clear, 0);
    }
    else if ((p2.x > p1.x) && (p2.y < p1.y)) {
        drawLinePositive(p1, p2, clear,0);
    }
}

void drawDottedLine(Point p1, Point p2, int clear, int dot){
    if (p1.x > p2.x) {
        drawDottedLine(p2, p1, clear, dot);
    }
    else if (p1.x == p2.x) {
        drawVerticalLine(p1, p2, clear, dot);
    }
    else if (p1.y == p2.y){
        drawHorizontalLine(p1, p2, clear, dot);
    }
    else if ((p2.x > p1.x) && (p2.y > p1.y)) {
        drawLineNegative(p1, p2, clear, dot);
    }
    else if ((p2.x > p1.x) && (p2.y < p1.y)) {
        drawLinePositive(p1, p2, clear, dot);
    }
}

// draw circle section
void drawCircle(Point center, int radius, int clear) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;
    /* plot first set of point */
    plot8points(center, x, y, clear);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        plot8points(center, x, y, clear);
    }
}

void plot4points(Point center, int x, int y, int clear) {
    if (clear == 1) {
        plotPixelRGBA(center.x + x, center.y + y, 255, 255, 255, 0);
        plotPixelRGBA(center.x - x, center.y + y, 255, 255, 255, 0);
        plotPixelRGBA(center.x + x, center.y - y, 255, 255, 255, 0);
        plotPixelRGBA(center.x - x, center.y - y, 255, 255, 255, 0);
    } else 
    if (clear == 2) {
        plotPixelRGBA(center.x + x, center.y + y, 0, 255, 0, 0);
        plotPixelRGBA(center.x - x, center.y + y, 0, 255, 0, 0);
        plotPixelRGBA(center.x + x, center.y - y, 0, 255, 0, 0);
        plotPixelRGBA(center.x - x, center.y - y, 0, 255, 0, 0);
    } else 
    if (clear == 4) {
        plotPixelRGBA(center.x + x, center.y + y, 200, 200, 200, 0);
        plotPixelRGBA(center.x - x, center.y + y, 200, 200, 200, 0);
        plotPixelRGBA(center.x + x, center.y - y, 200, 200, 200, 0);
        plotPixelRGBA(center.x - x, center.y - y, 200, 200, 200, 0);
	} else {
        plotPixelRGBA(center.x + x, center.y + y, 0, 0, 0, 0);
        plotPixelRGBA(center.x - x, center.y + y, 0, 0, 0, 0);
        plotPixelRGBA(center.x + x, center.y - y, 0, 0, 0, 0);
        plotPixelRGBA(center.x - x, center.y - y, 0, 0, 0, 0);
    }
        
}

void plot8points(Point center, int x, int y, int clear) {
        plot4points(center, x, y, clear);
        plot4points(center, y, x, clear);
}

// draw circle section
void drawhCircle(Point center, int radius, int clear) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;
    /* plot first set of point */
    plot8pointsh(center, x, y, clear);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        plot8pointsh(center, x, y, clear);
    }
}

void plot4pointsh(Point center, int x, int y, int clear) {
	int _x = center.x-x;
	if (center.y-y >= 480) {
		printf("%d\n",center.y-y);
	}
	while (_x < (center.x+x)) {
		plotPixelRGBA(_x, center.y - y, 0, 255, 0, 1);
		_x+=1;
	}
}

void plot8pointsh(Point center, int x, int y, int clear) {
        plot4pointsh(center, x, y, clear);
        plot4pointsh(center, y, x, clear);
}

void drawPlane (Point start, int clear){
    int posx = start.x;
    int posy = start.y;
    
    int nPoint = 58;
    Point arrPoint[58] = {
		
        {0,2}, {2,2},
        {2,2}, {5,6},
        {5,6}, {9,8},
        {9,8}, {12,8},
        {12,8}, {19,1},
        {19,1}, {21.5,1},
        {21.5,1}, {20,8},
        {20,8}, {21,8},
        {21,8}, {22,7},
        {22,7}, {24,7},
        {24,7}, {25,8},
        {25,8}, {27,8},
        {27,8}, {33,10},
        {33,10}, {27, 12},
        {27, 12}, {20, 12},
        {20, 12}, {19,13},
        {19,13}, {18,13},
        {18,13}, {10,22},
        {10,22}, {8,22},
        {8,22}, {10.5, 12},
        {10.5, 12}, {0,12},
        {0,12}, {1,8},
        {1,8}, {2,8},
        {2,8}, {0,2}
        
        /*/tail 10
        {0,2}, {2,2},
        {2,2}, {5,6},
        {5,6}, {9,8},
        {9,8}, {1,8},
        {1,8}, {0,2},
        
        //body 28
        {0,7},{1,7},
        {7,7},{19,7},
        {23,7},{25,7},
        {25,7},{30,9},
        {30,9},{25,11},
        {25,11},{18,11},
        {18,11},{17,12},
        {17,12},{15,12},
        {15,12},{9,21},
        {9,21},{5,21},
        {5,21},{8,12},
        {8,12},{9,11},
        {9,11},{0,11},
        {0,7},{0,11},
        
        //kaca 12
        {19,7},{20,6},
        {19,7},{20,8},
        {22,6},{23,7},
        {22,8},{23,7},
        {20,6},{22,6},
        {20,8},{22,8},
        
        //sayap 6
        {10,7},{17,0},
        {17,0},{19.5,0},
        {19.5,0},{18,7},
        
        //roda 2
        {19,11},{19,12}*/
        
    };
    
    
    Point pCircle = {19,13};
    
    //scale
    int scaleFactor = 10;
    for(int i = 0; i<nPoint; i++){
        arrPoint[i].x *= scaleFactor;
        arrPoint[i].y *= scaleFactor;
    }
    pCircle.x *= scaleFactor;
    pCircle.y *= scaleFactor;
    
    //position
    for (int i = 0; i<nPoint; i++){
        arrPoint[i].x += posx;
        arrPoint[i].y += posy;
    }
    pCircle.x += posx;
    pCircle.y += posy;
    
    //draw
    for (int i = 0; i<nPoint; i = i+2){
        drawLine(arrPoint[i],arrPoint[i+1],clear);
    }   
    
    drawCircle(pCircle,8,clear);
    drawCircle(pCircle,4,clear);
    
}


void drawExplosion(Point center, int scale, int clear) {
    Point p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16;
    p1.x = center.x - (1*scale);        p1.y = center.y + (2*scale);
    p2.x = center.x;                    p2.y = center.y + (3*scale);
    p3.x = center.x + (1*scale);        p3.y = center.y + (2*scale);
    p4.x = center.x + (3*scale);        p4.y = center.y + (3*scale);
    p5.x = center.x + (2*scale);        p5.y = center.y + (1*scale);
    p6.x = center.x + (3*scale);        p6.y = center.y;
    p7.x = center.x + (2*scale);        p7.y = center.y - (1*scale);
    p8.x = center.x + (3*scale);        p8.y = center.y - (3*scale);
    p9.x = center.x + (1*scale);        p9.y = center.y - (2*scale);
    p10.x = center.x;                   p10.y = center.y - (3*scale);
    p11.x = center.x - (1*scale);       p11.y = center.y - (2*scale);
    p12.x = center.x - (3*scale);       p12.y = center.y - (3*scale);
    p13.x = center.x - (2*scale);       p13.y = center.y - (1*scale);
    p14.x = center.x - (3*scale);       p14.y = center.y;
    p15.x = center.x - (2*scale);       p15.y = center.y + (1*scale);
    p16.x = center.x - (3*scale);       p16.y = center.y + (3*scale);
    
    drawLine(p1,p2,clear);
    drawLine(p2,p3,clear);
    drawLine(p3,p4,clear);
    drawLine(p4,p5,clear);
    drawLine(p5,p6,clear);
    drawLine(p6,p7,clear);
    drawLine(p7,p8,clear);
    drawLine(p8,p9,clear);
    drawLine(p9,p10,clear);
    drawLine(p10,p11,clear);
    drawLine(p11,p12,clear);
    drawLine(p12,p13,clear);
    drawLine(p13,p14,clear);
    drawLine(p14,p15,clear);
    drawLine(p15,p16,clear);
    drawLine(p16,p1,clear);
}

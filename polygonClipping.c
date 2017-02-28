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

typedef enum {Left,Right, Bottom, Top} Edge;
#define N_EDGE 4
#define FALSE 0
#define TRUE 1

int inside (Point p, Edge b, Point wMin, Point wMax) {
	switch (b) {
		case Left 	: if (p.x < wMin.x) return FALSE; break;
		case Right 	: if (p.x > wMax.x) return FALSE; break;
		case Bottom	: if (p.y > wMax.y) return FALSE; break;
		case Top 	: if (p.y < wMin.y) return FALSE; break;
	}
	return TRUE;
}

int cross (Point p1, Point p2, Edge b, Point wMin, Point wMax) {
	if (inside(p1, b, wMin, wMax) == inside(p2,b,wMin,wMax)) {
		return FALSE;
	} else 
		return TRUE;
}

Point intersect (Point p1, Point p2, Edge b, Point wMin, Point wMax) {
	Point iPt;
	float m;
	
	if (p1.x != p2.x)
		m = (p1.y-p2.y)/(p1.x-p2.x);
	
	switch (b) {
		case Left :
			iPt.x = wMin.x;
			iPt.y = p2.y + (wMin.x - p2.x)*m;
			break;
		case Right :
			iPt.x = wMin.x;
			iPt.y = p2.y + (wMax.x - p2.x)*m;
			break;
		case Bottom :
			iPt.y = wMin.y;
			if (p1.x != p2.x)
				iPt.x = p2.y - (wMin.x - p2.x)*m;
			else
				iPt.x = p2.x;
			break;
		case Top :
			iPt.y = wMax.y;
			if (p1.x != p2.x)
				iPt.x = p2.y - (wMin.x - p2.x)*m;
			else
				iPt.x = p2.x;
			break;
	}
	return iPt;
}

void clipPoint (Point p, Edge b, Point wMin, Point wMax, Point* pOut, int* cnt, Point* first[], Point* s) {
	Point iPt;
	
	// if no previous point exist for this edge, save this point
	if (!first[b])
		first[b]=&p;
	else {
		/*Previous point exist. If p and previous point cross edge, fin intersection. 
		Clip against next boundary, if any. if no more edges, add intersection to output list.*/
		if (cross (p,s[b],b,wMin,wMax)) {
			iPt = intersect(p,s[b],b,wMin,wMax);
			if (b < Top) {
				clipPoint (iPt, b+1, wMin, wMax, pOut, cnt, first, s);
			} else {
				pOut[*cnt] = iPt;
				(*cnt)++;
			}
		}
	}
	
	s[b] = p;
	//For all, if point is inside proceed to next clip edge, if any
	if (inside(p,b,wMin,wMax)) {
		if (b < Top)
			clipPoint (p, b+1, wMin,wMax, pOut, cnt, first, s);
		else {
			pOut[*cnt] = p;
			(*cnt)++;
		}
	} 
}

void closeClip (Point wMin, Point wMax, Point* pOut, int* cnt, Point* first[], Point* s) {
	Point i;
	Edge b;
	
	for (b = Left; b <= Top; b++) {
		if (cross (s[b] , *first[b],b,wMin,wMax)) {
			i = intersect (s[b], *first[b],b, wMin, wMax);
			if (b < Top)
				clipPoint (i,b+1, wMin,wMax,pOut, cnt, first,s);
			else {
				pOut[*cnt] = i;
				(*cnt)++;
			}
		}
	}
}

int clipPolygon (Point wMin, Point wMax, int n, Point* pIn, Point* pOut) {
	/* first hold pointer to first point processed against a clip
	edge, s holds most recent point processed against an edge */
	Point* first[N_EDGE] = {0,0,0,0}, s[N_EDGE];
	int i, cnt = 0;
	
	for (i = 0; i<n; i++){
		clipPoint(pIn[i], Left, wMin,wMax,pOut,&cnt,first,s);
	}
	closeClip(wMin, wMax, pOut, &cnt, first, s);
	return cnt;
}

int main() {
	return 0;
}


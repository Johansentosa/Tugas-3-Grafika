#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>

#define numBuildings4 4
#define numBuildings8 4

using namespace std;

	
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

/*typedef struct Polygons {
	Point* p;
	p = malloc(8*sizeof(Point));
} Building8;*/
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

vector<vector<Point> > Buildings;

void printchar() {
	FILE* file = fopen ("buildings.txt", "r");
	int i = 0;
	char c;
	do {
		c = getc(file);
	} while (c != '#');
	do {
			c = getc(file);
		} while (c!='\n');
		
	fscanf (file, "%d", &i);    
	while (!feof (file))
	{  
	  printf ("%d ", i);
	  fscanf (file, "%d", &i);      
	}
	fclose (file); 
}
void loadBuildings4() {
	cout<< "TES"<<endl;
	cout<< "while";
	char c;
	int valx, valy;
	Point pTemp;
	FILE *file;
	file = fopen("buildings.txt", "r");
	vector<Point> building;
	int itbuilding;
	int itpoint;
	while (!feof(file)) {
		do {
				c = getc(file);
				if (feof(file)) break;
		} while (c!='#');
		
		if (!feof(file)) {
			while (c = getc(file) != '#'){
				fscanf (file, "%d", &valx);
				//cout<<valx<<" ";
				fscanf (file, "%d", &valy);
				//cout<<valy<<endl;
				setPoint(&pTemp, valx, valy);
				building.push_back(pTemp);
			}
			Buildings.push_back(building);
			//cout<<endl;
			building.clear();
		}
	};
	fclose(file);
	//print
	for (int itbuilding = 0; itbuilding < Buildings.size(); itbuilding++) {
		cout<<"Building number "<<itbuilding+1<<endl;
		for (itpoint = 0; itpoint < Buildings[itbuilding].size(); itpoint++) {
			cout << Buildings[itbuilding][itpoint].x <<" "<<Buildings[itbuilding][itpoint].y<< endl;
		}
	}
}

int main() {
	loadBuildings4();
	return 0;
}



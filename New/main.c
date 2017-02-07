#include "shooter.c"

int main(){
	fbfd = open("/dev/fb0", O_RDWR);
     if (fbfd == -1) {
         perror("Error: cannot open framebuffer device");
         exit(1);
     }
     printf("The framebuffer device was opened successfully.\n");

     // Ambil data informasi screen
     if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         perror("Error reading fixed information");
         exit(2);
     }
     if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         perror("Error reading variable information");
         exit(3);
     }

     // Informasi resolusi, dan bit per pixel
     printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

     // Hitung ukuran memori untuk menyimpan framebuffer
     screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

     // Masukkan framebuffer ke memory, untuk kita ubah-ubah
     fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
     if ((int)fbp == -1) {
         perror("Error: failed to map framebuffer device to memory");
         exit(4);
     }
     printf("The framebuffer device was mapped to memory successfully.\n");
     
     int x;
    int y;
    for (y = 0; y < vinfo.yres - 15 ;y++){
         for (x = 0; x < vinfo.xres  ; x++) {
             if (vinfo.bits_per_pixel == 32) {
                 plotPixelRGBA(x,y,0,0,0,0);
             } else  { 
                 plotPixelRGBA(x,y,0,0,0,0);
             }

         }
	 }
    

    int posx = 10;
    int posy = 10;
    Point arrPoint[38] = {
        //tail 10
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
        
        {15,12},{9,23},
        {9,23},{5,23},
        {5,23},{8,12},
        {8,12},{9,11},
        {9,11},{0,11},
        {0,7},{0,11}
    };
    
    //scale
    int scaleFactor = 10;
    for(int i = 0; i<38; i++){
        arrPoint[i].x *= scaleFactor;
        arrPoint[i].y *= scaleFactor;
    }
    
    //position
    for (int i = 0; i<38; i++){
        arrPoint[i].x += posx;
        arrPoint[i].y += posy;
    }
    
    Point p1;
    p1.x = 10; p1.y = 10;
    
    point.x = vinfo.xres/2;
    
    Point boom = {vinfo.xres/2, vinfo.yres-50};
    drawhCircle(boom, 45, 1);
    pthread_t p;
    pthread_create(&p, NULL, &commandcall, NULL);
    pthread_create(&p, NULL, &draw, NULL);
    while(1) {
        drawPlane(p1,0);
        Point p2 = {p1.x+50, p1.y+100};
		Color warna ={0,0,0,0};
		//solidFill(&p2, warna);


        p1.x += 1;
        drawPlane(p1,4);
        p2.x = p1.x+50; p2.y=p1.y+100;
		
		Color warna2 ={0,255,0,0};
		solidFill(&p2, warna2);
        usleep(5000);
        if (life <= 0) {
			int l;
			for (l = 0; l < 39; l++) {
				int randx = rand() % 250;
				int randy = rand() % 200+50;
				int rands = rand() % 30;
				Point np = {p1.x+randx, p1.y+randy};
				drawExplosion(np, rands, 2);
				usleep(50000);
				drawExplosion(np, rands, 0);
			}
			drawPlane(p1,0);
			break;
		}
    }
    pthread_join(p, NULL);
    
    Point center = {400, 400};
    
}

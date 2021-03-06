//---------------------------------------- Include -----------------------------------------//
#include <openWS.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>
#include <time.h>
#include <math.h>
#include <list>

//------------------------------------ Global variables ------------------------------------//
int rootWindow;

//--------------------------------------- Animation ----------------------------------------//
#define ANIME_ORIGIN(i,j)	(i<<2|j)
#define ANIME_EAT(i,j)		(i<<6|j<<4)
#define ANIME_UPGRADE		(0x100)
#define ANIME_SPAWN			(0x200)
#define ANIME_EAT_X(a)		(((a)&0xC0)>>6)
#define ANIME_EAT_Y(a)		(((a)&0x30)>>4)
#define ANIME_ORIGIN_X(a)	(((a)&0xC)>>2)
#define ANIME_ORIGIN_Y(a)	(((a)&0x3))
#define ANIME_TYPE(a)		((a)&0x300)
struct Animation {
	#define TOTAL_TICK	(9)
	int ID, dep, tick, type;
	int fx, fy, tx, ty;	//from and target
	void flush() {
		void drawElement(double x, double y, double scaleRatio, int ID);
		double scale, x, y;
		x = (fx * tick + tx * (TOTAL_TICK - tick)) / (double)TOTAL_TICK,
		y = (fy * tick + ty * (TOTAL_TICK - tick)) / (double)TOTAL_TICK;
		if(type & ANIME_SPAWN)
			scale = 1.0 - tick * 1.0 / TOTAL_TICK;
		else if(type & ANIME_UPGRADE) {
			scale = tick * 0.4 / TOTAL_TICK;
			if(tick >= (TOTAL_TICK >> 1)) scale = 1.4 - scale;
			else scale = 1.0 + scale;
			if(tick == (TOTAL_TICK >> 1)) ++ID;
		} else scale = 1;
		drawElement(x, y, scale, ID);
		if(tick) tick--;
	}
    Animation(int id, int _dep, int _fx, int _fy, int _tx, int _ty, int _type) :
	ID(id), dep(_dep), tick(TOTAL_TICK), type(_type), fx(_fx), fy(_fy), tx(_tx), ty(_ty) {}
	#undef TOTAL_TICK
};
std::list<Animation> animationList;
void initAnimeData() {
	animationList.clear(); 
	extern int Map[4][4], Anime[4][4];
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			if(Map[i][j]) Anime[i][j] = ANIME_ORIGIN(i, j);
			else Anime[i][j] = 0;
}
void generateAnime() {
	extern int Map[4][4], Anime[4][4];
	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			if(Map[i][j]) {
				if(Anime[i][j] & ANIME_UPGRADE) {
					animationList.push_back(Animation(
						Map[i][j] - 1, 0,
						ANIME_EAT_X(Anime[i][j]), ANIME_EAT_Y(Anime[i][j]),
						j, i, 0));
				}
				animationList.push_back(Animation(
					Map[i][j] - ((Anime[i][j] & ANIME_UPGRADE) ? 1 : 0),
					1,
					ANIME_ORIGIN_Y(Anime[i][j]), ANIME_ORIGIN_X(Anime[i][j]),
					j, i, ANIME_TYPE(Anime[i][j])));
			}
		}
	}
}

//---------------------------------------- Display -----------------------------------------//

void drawRoundedRectangle(int width, int height, int ratio) {
	static const float PI = acos(-1);
	width = (width >> 1) - ratio;
	height = (height >> 1) - ratio;
	for(int i = 0; i < 90; i += 10)
		glVertex2f(ratio * cos(i * PI / 180.0) + width, ratio * sin(i * PI / 180.0) + height);
	for(int i = 90; i < 180; i += 10)
		glVertex2f(ratio * cos(i * PI / 180.0) - width, ratio * sin(i * PI / 180.0) + height);
	for(int i = 180; i < 270; i += 10)
		glVertex2f(ratio * cos(i * PI / 180.0) - width, ratio * sin(i * PI / 180.0) - height);
	for(int i = 270; i < 360; i += 10)
		glVertex2f(ratio * cos(i * PI / 180.0) + width, ratio * sin(i * PI / 180.0) - height);
}
void flushUnderColor(int, int) {
	glClearColor(250.0f / 256, 248.0f / 256, 239.0f / 256, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
void halfopacityCovery(int, int) {
	glClearColor(250.0f / 256, 248.0f / 256, 239.0f / 256, 0.5f);
	glClear(GL_COLOR_BUFFER_BIT);
}
void flushBackground() {
	static unsigned list = 100000000;
	if(list == 100000000) {
		list = glGenLists(1);
		glNewList(list, GL_COMPILE);
		glColor3ub(187, 173, 160);
		glPushMatrix();
		glTranslatef(220, 220, 0);
		glBegin(GL_POLYGON);
		drawRoundedRectangle(440, 440, 15);
		glEnd();
		glPopMatrix();
		glColor3ub(205, 193, 180);
		for(int i = 0; i < 4; ++i)
			for(int j = 0; j < 4; ++j) {
				glPushMatrix();
				glTranslatef(i * 110 + 55, j * 110 + 55, 0);
				glBegin(GL_POLYGON);
				drawRoundedRectangle(95, 95, 10);
				glEnd();
				glPopMatrix();
			}
		glEndList();
	}
	glLoadIdentity();
	glOrtho(0, 440, 0, 440, -1.0, 1.0);
	glCallList(list);
}
void displayMap(int, int) {
	flushBackground();
	for(auto obj = animationList.begin(), _end = animationList.end(); obj !=_end;)
		if(!obj->dep) {
			if(obj->tick) obj->flush(), ++obj;
			else obj = animationList.erase(obj);
		} else ++obj;
	for(auto obj = animationList.begin(), _end = animationList.end(); obj !=_end; ++obj)
		if(obj->dep) obj->flush();
}
void drawString(double x, double y, const char *format, ...) {
	static GLuint rasters[][16] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x78, 0x78, 0x78, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x30, 0x30, 0xf8, 0x0c, 0x0c, 0x78, 0xc0, 0xc0, 0x7c, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x8c, 0xcc, 0x60, 0x30, 0x18, 0xcc, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x76, 0xdc, 0xcc, 0xde, 0xfa, 0x70, 0xd8, 0xd8, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x60, 0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x66, 0x3c, 0xdf, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x80, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x7c, 0xc6, 0xe6, 0xf6, 0xd6, 0xde, 0xce, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfc, 0x30, 0x30, 0x30, 0x30, 0x30, 0xf0, 0x30, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfc, 0xcc, 0x60, 0x30, 0x18, 0x0c, 0xcc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0x0c, 0x0c, 0x38, 0x0c, 0x0c, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x1e, 0x0c, 0x0c, 0xfe, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0x0c, 0x0c, 0xf8, 0xc0, 0xc0, 0xc0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0xf8, 0xc0, 0xc0, 0x60, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0xc6, 0xc6, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xdc, 0x78, 0xec, 0xcc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x70, 0x30, 0x18, 0x18, 0x7c, 0xcc, 0xcc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x30, 0x18, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x18, 0x0c, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x7c, 0xc0, 0xc0, 0xde, 0xde, 0xde, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xfc, 0xcc, 0xcc, 0xcc, 0x78, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x66, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x3c, 0x66, 0xc6, 0xc0, 0xc0, 0xc0, 0xc6, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xf8, 0x6c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6c, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfe, 0x62, 0x60, 0x64, 0x7c, 0x64, 0x60, 0x62, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xf0, 0x60, 0x60, 0x64, 0x7c, 0x64, 0x62, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x3e, 0x66, 0xc6, 0xce, 0xc0, 0xc0, 0xc6, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0xfc, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xe6, 0x66, 0x6c, 0x6c, 0x78, 0x6c, 0x6c, 0x66, 0xe6, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfe, 0x66, 0x66, 0x62, 0x60, 0x60, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xd6, 0xfe, 0xfe, 0xee, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xc6, 0xc6, 0xce, 0xde, 0xfe, 0xf6, 0xe6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x38, 0x6c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xf0, 0x60, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x1e, 0x0c, 0x7c, 0xde, 0xce, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xe6, 0x66, 0x66, 0x6c, 0x7c, 0x66, 0x66, 0x66, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0x18, 0x70, 0xc0, 0xcc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0xb4, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x30, 0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x6c, 0x6c, 0x6c, 0xd6, 0xd6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0x78, 0x30, 0x78, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0x30, 0x30, 0x30, 0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfe, 0xc6, 0x62, 0x60, 0x30, 0x18, 0x98, 0xce, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x01, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0xdf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x76, 0xcc, 0xcc, 0x7c, 0x0c, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xdc, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xc0, 0xc0, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x76, 0xcc, 0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0x0c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xc0, 0xfc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xf0, 0x60, 0x60, 0x60, 0xf8, 0x60, 0x60, 0x6c, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x78, 0xcc, 0x0c, 0x7c, 0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xe6, 0x66, 0x66, 0x66, 0x76, 0x6c, 0x60, 0x60, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x78, 0xcc, 0xcc, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xe6, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x60, 0x60, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xc6, 0xd6, 0xd6, 0xd6, 0xd6, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xf0, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x1e, 0x0c, 0x7c, 0xcc, 0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xf0, 0x60, 0x60, 0x76, 0x6e, 0xec, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x78, 0xcc, 0x18, 0x60, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x38, 0x6c, 0x60, 0x60, 0x60, 0xfc, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x76, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x30, 0x78, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x6c, 0x6c, 0xd6, 0xd6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xf0, 0x18, 0x0c, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xfc, 0xc4, 0x60, 0x18, 0x8c, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x1c, 0x30, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x30, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xe0, 0x30, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x30, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce, 0xda, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0xfe, 0xc6, 0xc6, 0x6c, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
	};
	static char str[1024];
	char *pt = str;
	va_list args;
	va_start(args, format);
	vsprintf(str, format, args);
	va_end(args);
	glRasterPos2f(x, y);
	while(*pt & 0xe0) glBitmap(8, 16, 0.0, 0.0, 8.0, 0.0, (GLubyte *)rasters[*pt - 32]), ++pt;
}
void drawElement(double x, double y, double scaleRatio, int ID) {
	static const unsigned char color[][3] = {
		{255, 255, 255},/*     0*/	{238, 228, 218},/*     2*/
		{237, 224, 200},/*     4*/	{242, 177, 121},/*     8*/
		{245, 149,  99},/*    16*/	{246, 124,  95},/*    32*/
		{249,  94,  59},/*    64*/	{237, 207, 114},/*   128*/
		{237, 204,  97},/*   256*/	{237, 200,  80},/*   512*/
		{237, 197,  63},/*  1024*/	{237, 194,  46},/*  2048*/
		{237, 182,  30},/*  4096*/	{200, 191,  50},/*  8192*/
		{180, 210,  50},/* 16384*/	{168, 210,  70},/* 32768*/
		{150, 210,  90},/* 65536*/	{120, 210, 120} /*131072*/
	};
	static const int len[] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6};
	static unsigned circle = 0xFFFFFFFF;
	if(circle == 0xFFFFFFFF) {
		circle = glGenLists(1);
		glNewList(circle, GL_COMPILE);
		glBegin(GL_POLYGON);
		drawRoundedRectangle(95, 95, 10);
		glEnd();
		glEndList();
	}
	glPushMatrix();
	glTranslatef(x * 110 + 55, y * 110 + 55, 0);
	glScaled(scaleRatio, scaleRatio, 1);
	glColor3ubv(color[ID]);
	glCallList(circle);
	glPopMatrix();
	glPushMatrix();
	glColor3b(255, 255, 255);
	drawString(x * 110 + 55 - 4 * len[ID], y * 110 + 55, "%d", 1 << ID);
	glPopMatrix();
}
void displayInfo(int, int) {
	extern int totalScore;
	glClearColor(250.0f / 256, 248.0f / 256, 239.0f / 256, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	glOrtho(0, 440, 0, 20, -1, 1);
	glColor3ub(0, 0, 0);
	drawString(0, 10, "Score: %08d", totalScore);
}

//---------------------------------------- Control -----------------------------------------//

int failKey(int windowID, int key, int, int, int) {
	void mapReset();
	if(key == WS_KEY_R) {
		mapReset();
		wsCloseWindow(windowID);
	} else if(key == WS_KEY_ESCAPE) return 1;
	return 0;
}
int keyCallback(int, int key, int, int action, int) {
	void mapUp(); void mapDown(); void mapLeft(); void mapRight(); void mapReset();
	if(action != WS_PRESS) return 0;
	switch(key) {
		case WS_KEY_W: case WS_KEY_UP:	  mapUp();	  break;
		case WS_KEY_S: case WS_KEY_DOWN:  mapDown();  break;
		case WS_KEY_A: case WS_KEY_LEFT:  mapLeft();  break;
		case WS_KEY_D: case WS_KEY_RIGHT: mapRight(); break;
		case WS_KEY_R: mapReset(); break;
		case WS_KEY_ESCAPE: wsTerminate(); break;
	}
	return 0;
}

//------------------------------------- Map Operations -------------------------------------//

int Map[4][4], Anime[4][4], numOfBlocks, totalScore;
void newBlock() {
	bool neecCheck();
	int cnt = rand() % (16 - numOfBlocks);
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			if(!Map[i][j] && !cnt--) {
				Map[i][j] = 1 + !(rand() & 0x3);
				Anime[i][j] = ANIME_SPAWN | ANIME_ORIGIN(i, j);
			}
	++numOfBlocks;
}
void check() {
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 3; ++j)
			if(Map[i][j] == Map[i][j + 1] || Map[j][i] == Map[j + 1][i]) return;
	int id = wsCreateWindow("fail", 30, 80, 440, 440, nullptr, WS_STYLE_DEFAULT, rootWindow);
	wsSetDisplayCallback(id, halfopacityCovery);
	wsSetKeyCallback(id, failKey);
}
void mapUp() {
	bool updated = false;
	initAnimeData();
	for(int i = 0; i < 4; ++i) {
		for(int j = 2, last = 3; ~j && last >= j; --j) {
			if(Map[j][i]) {
				while(last > j && Map[last][i] && Map[last][i] != Map[j][i]) --last;
				if(last == j) continue;
				if(Map[last][i]) {
					totalScore += 1 << (++Map[last][i]); Anime[last][i] |= ANIME_UPGRADE | ANIME_EAT(i, j);
					--last, --numOfBlocks;
				} else {
					Map[last][i] = Map[j][i];
					Anime[last][i] = Anime[j][i];
				}
				updated = true;
				Map[j][i] = 0;
			}
		}
	}
	if(updated) newBlock();
	else if(numOfBlocks == 16) check();
	generateAnime();
}
void mapDown() {
	bool updated = false;
	initAnimeData();
	for(int i = 0; i < 4; ++i) {
		for(int j = 1, last = 0; j < 4 && last <= j; ++j) {
			if(Map[j][i]) {
				while(last < j && Map[last][i] && Map[last][i] != Map[j][i]) ++last;
				if(last == j) continue;
				if(Map[last][i]) {
					totalScore += (1 << (++Map[last][i])); Anime[last][i] |= ANIME_UPGRADE | ANIME_EAT(i, j);
					++last, --numOfBlocks;
				} else {
					Map[last][i] = Map[j][i];
					Anime[last][i] = Anime[j][i];
				}
				updated = true;
				Map[j][i] = 0;
			}
		}
	}
	if(updated) newBlock();
	else if(numOfBlocks == 16) check();
	generateAnime();
}
void mapLeft() {
	bool updated = false;
	initAnimeData();
	for(int i = 0; i < 4; ++i) {
		for(int j = 1, last = 0; j < 4 && last <= j; ++j) {
			if(Map[i][j]) {
				while(last < j && Map[i][last] && Map[i][last] != Map[i][j]) ++last;
				if(last == j) continue;
				if(Map[i][last]) {
					totalScore += 1 << (++Map[i][last]); Anime[i][last] |= ANIME_UPGRADE | ANIME_EAT(j, i);
					++last, --numOfBlocks;
				} else {
					Map[i][last] = Map[i][j];
					Anime[i][last] = Anime[i][j];
				}
				updated = true;
				Map[i][j] = 0;
			}
		}
	}
	if(updated) newBlock();
	else if(numOfBlocks == 16) check();
	generateAnime();
}
void mapRight() {
	bool updated = false;
	initAnimeData();
	for(int i = 0; i < 4; ++i) {
		for(int j = 2, last = 3; ~j && last >= j; --j) {
			if(Map[i][j]) {
				while(last > j && Map[i][last] && Map[i][last] != Map[i][j]) --last;
				if(last == j) continue;
				if(Map[i][last]) {
					totalScore += (1 << (++Map[i][last])); Anime[i][last] |= ANIME_UPGRADE | ANIME_EAT(j, i);
					--last, --numOfBlocks;
				} else {
					Map[i][last] = Map[i][j];
					Anime[i][last] = Anime[i][j];
				}
				updated = true;
				Map[i][j] = 0;
			}
		}
	}
	if(updated) newBlock();
	else if(numOfBlocks == 16) check();
	generateAnime();
}
void mapReset() {
	initAnimeData();
	memset(Map, 0, sizeof Map);
	totalScore = 0;
	numOfBlocks = 0;
	newBlock();
	newBlock();
	generateAnime();
}

int main() {
    wsInit();
	srand(time(0));
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	mapReset();
	rootWindow = wsCreateWindow("2048", 128, 128, 500, 550, nullptr);
	int id = wsCreateWindow("Map", 30, 80, 440, 440, nullptr, WS_STYLE_DEFAULT, rootWindow);
	wsSetDisplayCallback(id, displayMap);
	id = wsCreateWindow("Info", 30, 30, 440, 30, nullptr, WS_STYLE_DEFAULT, rootWindow);
	wsSetDisplayCallback(id, displayInfo);
	wsSetDisplayCallback(rootWindow, flushUnderColor);
	wsSetKeyCallback(rootWindow, keyCallback);
	wsMainLoop();
}

// Common headers:
#include <iostream>
#include <stdlib.h>
#include <stdarg.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <map>

// Windows headers:
#include <GL/glut.h>
#include <GL/gl.h>
#include <windows.h> // for Sleep under win

// OS X headers:
//#include <GLUT/glut.h>
//#include "/System/Library/Frameworks/OpenGL.framework/Headers/gl.h"
//#include <unistd.h> // for sleep() under nix

#define DEFAULT_NUMBLOCKS 30
enum {
	UP = 1,
	DOWN = 2,
} keypress;

enum {
	PRMODE_CALC = 0,
	PRMODE_REC = 1,
	PRMODE_VISUAL = 2
} genmodes;

enum {
	DSTMODE_UNI = 0,
	DSTMODE_EXP = 1,
	DSTMODE_POI = 2,
	DSTMODE_GEO = 3,
} ;

using namespace std;

int compars = 0;
double lasttime = 0;

int dstMode = DSTMODE_UNI;
bool flipper = true;

// прототипы
void record_shake(void);
void fast_shake(void);
void writeString(char* string, int x, int y, void* font);
float Uniform(float mean);
float Exponential(float mean);
int   Poisson(float mean);

// Количество элементов массива
int gNumblocks = DEFAULT_NUMBLOCKS;

int progMode = PRMODE_CALC;

// Курсоры just for fun
int cursors[9] = {GLUT_CURSOR_INHERIT, GLUT_CURSOR_HELP, GLUT_CURSOR_INFO, GLUT_CURSOR_WAIT, GLUT_CURSOR_TEXT, GLUT_CURSOR_SPRAY, GLUT_CURSOR_LEFT_ARROW, 
	GLUT_CURSOR_LEFT_SIDE, GLUT_CURSOR_CROSSHAIR};

//char* modes[5] = {"1280x1024:24@60", "800x600:24@60", "640x480:16@60",  "1600x1200:24@60", "1920x1080:32@60"};

// Положения курсора
int oldX=0, gW=640, gH=480, oldY=0, xPan=0, yPan=0;

// Режим анимации
bool playback = false;
int playbackCurStepN = 0;

// Заполнен ли массив
bool arrayFilled = false;

int mState = UP; // Кнопка мыши нажата/отжата

struct ColorBar
{
	int x, y, w, h;
	float hue;
	bool hl;
};

int recordSteps = 0;
struct Record
{
	char action;
	int l_id;
	int r_id;
};

ColorBar* allBars = NULL;
ColorBar* allBarsBackup = NULL;
Record* recording = NULL;

float frand()
{
	return (float)rand()/(RAND_MAX);
}

float distrUniform(float mean)
{
	float R;
	R = (float)rand()/(float)(RAND_MAX);
	return  2*mean*R;

}


float distrExponential(float mean)
{
	float R;
	R = (float)rand()/(float)(RAND_MAX);
	return  -mean*log(R);
}

int distrPoisson(float mean) //Special technique required: Box-Muller method...
{
	float L = exp(-mean);
  float p = 1.0;
  int k = 0;

  do {
    k++;
    p *= frand();
  } while (p > L);

  return k - 1;
}

int distrGeometric(float p)
{
            float R;
            R= (float)rand()/(float)(RAND_MAX);
            return (int)(log(R)/log(1-p) - 1);
}

void genArray()
{
	if(recording!= NULL)
	{
		free(recording);
		recording=NULL;
		playbackCurStepN = 0;
		recordSteps = 0;
	}

	float segW = gW / gNumblocks;

	if(allBars == NULL)
	{
		allBars = (ColorBar*)calloc(gNumblocks, sizeof(ColorBar));
	}
	else
	{
		allBars = (ColorBar*) realloc(allBars, sizeof(ColorBar) * gNumblocks);
	}
	for(int i = 0; i < gNumblocks; i++)
	{
		float rnd;
		switch(dstMode)
		{
		case DSTMODE_EXP:
			rnd = distrExponential(frand());
			rnd *= 360;
			break;
		case DSTMODE_POI:
			rnd = distrPoisson(90);
			break;
		case DSTMODE_GEO:
			rnd = distrGeometric(frand());
			break;
		case DSTMODE_UNI:
		default:
			//rnd = Uniform((float)0.5);
			rnd = rand() % 360;
			break;
		}

		ColorBar nbar;
		nbar.x = i * segW;
		nbar.y = 50;
		nbar.w = segW;
		nbar.h = (gH / 360) * rnd * 0.7;
		nbar.hue = rnd;
		nbar.hl = false;
		allBars[i] = nbar;
	}
	arrayFilled = true;
}


/* I don't remember where I copied this function from, but it seems
 that this part of code belongs to Chris Hulbert */
void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;

	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}

	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
	case 0:
		*r = v;
		*g = t;
		*b = p;
		break;
	case 1:
		*r = q;
		*g = v;
		*b = p;
		break;
	case 2:
		*r = p;
		*g = v;
		*b = t;
		break;
	case 3:
		*r = p;
		*g = q;
		*b = v;
		break;
	case 4:
		*r = t;
		*g = p;
		*b = v;
		break;
	default:		// case 5:
		*r = v;
		*g = p;
		*b = q;
		break;
	}

}

// Обработчик нажатий клавиш
void glutKeyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'q':
		exit(EXIT_SUCCESS);
		break;
	case 'c':
		glutSetCursor(cursors[rand()%8]);
		break;
	case 's':
		switch(progMode)
		{
			case PRMODE_CALC:
				time_t begin, end;
				begin = clock();
				fast_shake();
				end = clock();
				lasttime = (end - begin)/(double)CLOCKS_PER_SEC;
				cout << "time elapsed: " << end << "  -- " << begin << " - - " << (end - begin)/(double)CLOCKS_PER_SEC << " seconds" << endl;
				break;
			case PRMODE_REC:
				record_shake();
				cout << "recording..." << endl;
				break;
			case PRMODE_VISUAL:
				if(playbackCurStepN >= recordSteps || (recordSteps == 0))
				{
					playbackCurStepN = 0;
					playback = false;
				}
				else
				{
					playback = !playback;
				}
				break;
		}
		glutPostRedisplay();
		break;
	case 'd':
		if(progMode == PRMODE_CALC)
		{
			if(dstMode < 4)
			{
				dstMode++;
			}
			else
			{
				dstMode = DSTMODE_EXP;
			}
			arrayFilled = false;
			genArray();
		}
		glutPostRedisplay();
		break;
	case 'v':
		playback = false;
		playbackCurStepN = 0;
		//recordSteps = 0;
		if(progMode < 2)
		{
			lasttime = 0;
			progMode++;
		}
		else
		{
			progMode = PRMODE_CALC;
		}
		glutPostRedisplay();
		break;
	case 'g':
		if(progMode == PRMODE_CALC)
		{
			genArray();
			glutPostRedisplay();
		}
		break;
	default:
		cout << "key pressed: " << key << endl;
		break;
	}
}

// Обработчик нажатий специальных клавиш
void glutKeyboardSpec(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_UP:
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		if(progMode == PRMODE_CALC && gNumblocks < gW)
		{
			gNumblocks++;
			arrayFilled = false;
			genArray();
		}
		glutPostRedisplay();
		break;
	case GLUT_KEY_LEFT:
		if(progMode == PRMODE_CALC && gNumblocks > 2)
		{
			gNumblocks--;
			arrayFilled = false;
			genArray();
		}
		glutPostRedisplay();
		break;
	case GLUT_KEY_PAGE_UP:
		if(progMode == PRMODE_CALC)
		{
			gNumblocks = gW;
			arrayFilled = false;
			genArray();
			glutPostRedisplay();
		}
		break;
	case GLUT_KEY_PAGE_DOWN:
		if(progMode == PRMODE_CALC)
		{
			gNumblocks = DEFAULT_NUMBLOCKS;
			arrayFilled = false;
			genArray();
			glutPostRedisplay();
		}
		break;
	case GLUT_KEY_HOME:
		xPan = yPan = 0;
		glutPostRedisplay();
		break;
	default:
		cout << "key pressed: " << key << endl;
		break;
	}
}

// Обработчик изменения размеров окна
void glutReshape(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, 0, 100.0);
	gW = w;
	gH = h;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void paintBar(ColorBar bar, bool hilighted = false)
{
	float red, green, blue;
	int x, y, w , h;

	x = bar.x;
	y = bar.y;
	w = bar.w;
	h = bar.h;

	HSVtoRGB(&red, &green, &blue, bar.hue, 1.0, 1.0);
	if(bar.hl)
	{
		glColor4f(red, green, blue, 0.5);
	}
	else
	{
		glColor3f(red, green, blue);
	}

	glBegin(GL_QUADS);
	glVertex2f(x, y);
	glVertex2f(x + w, y);

	HSVtoRGB(&red, &green, &blue, bar.hue, 1.0, 0.2);
	if(bar.hl)
	{
		glColor4f(red, green, blue, 0.5);
	}
	else
	{
		glColor3f(red, green, blue);
	}

	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();

	glColor3f(0.9, 0.9, 0.9);
	
	if(bar.hl)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);
		glEnd();
	}
	char numstr[21];
	sprintf(numstr, "%3.0f", bar.hue);

	glColor3f(1, 1, 1);
	writeString(numstr, x, y + h + 14, GLUT_BITMAP_8_BY_13);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void paintAllBars(void)
{
	if(!arrayFilled)
	{
		return;
	}
	for(int i = 0; i < gNumblocks; i++)
	{
		paintBar(allBars[i]);
	}

}

// Функция для просчёта анимации в playback режиме
void playbackCalcStep(void)
{
	if(playbackCurStepN > recordSteps)
	{
		return;
	}

	int l_id, r_id;
	Record curStep = recording[playbackCurStepN];
	l_id = curStep.l_id;
	r_id = curStep.r_id;
	ColorBar t;

	if(curStep.action == 'a' && playbackCurStepN + 1 < recordSteps && recording[playbackCurStepN + 1].action == 's' && flipper)
	{
		// arrange
		allBars[curStep.l_id].hl = true;
		allBars[curStep.r_id].hl = true;
		allBars[curStep.l_id].y += 10;
		allBars[curStep.r_id].y += 10;
	}
	else if(curStep.action == 's')
	{
		// swap
		if(flipper)
		{
			t.hue = allBars[l_id].hue;
			t.h = allBars[l_id].h;
			allBars[l_id].hue = allBars[r_id].hue;
			allBars[l_id].h = allBars[r_id].h;
			allBars[r_id].hue = t.hue;
			allBars[r_id].h = t.h;
		}
		else
		{	
			allBars[curStep.l_id].hl = false;
			allBars[curStep.r_id].hl = false;
			allBars[curStep.l_id].y -= 10;
			allBars[curStep.r_id].y -= 10;
		}
	}  
	if(flipper)
	{
		playbackCurStepN++;
	}
	flipper = !flipper;
}

// Callback функция отрисовки содержимого
void glutDisplay(void)
{
	//	glClearColor(0.6, 0.6, 0.67, 0);
	glClearColor(0.025, 0.025, 0.05, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	// По пикселям
	glTranslatef (0.375, 0.375, 0);

	// Фиксированное положение камеры - в distance единицах от центра мира
	glTranslatef (0.375 + yPan, xPan + 0.375, 0);

	paintAllBars();

	// Обратно в правый верхний угол
	glTranslatef (-yPan, -xPan, 0);

	char* dstmds;
	switch(dstMode)
	{
	case DSTMODE_EXP:
		dstmds = "EXP";
		break;
	case DSTMODE_POI:
		dstmds = "POI";
		break;
	case DSTMODE_GEO:
		dstmds = "GEO";
		break;
	case DSTMODE_UNI:
	default:
		dstmds = "UNI";
		break;
	}
	char* prmds;
	switch(progMode)
	{
	case PRMODE_REC:
		prmds = "REC";
		break;
	case PRMODE_VISUAL:
		prmds = "PLY";
		break;
	case PRMODE_CALC:
	default:
		prmds = "CAL";
		break;
	}

	char numstr[512]; // enough to hold all numbers up to 64-bits

	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(gW, 0);
	glVertex2f(gW, 50);
	glVertex2f(0, 50);
	glEnd();

	glColor3f(1, 1, 1);
	sprintf(numstr, "%s | Elements: %5d | Compars: %10d | Distrib: %s | Time: %.4fs",
		prmds,
		gNumblocks,
		compars,
		dstmds,
		lasttime
		);

	writeString(numstr, 3, 18, GLUT_BITMAP_9_BY_15);

	sprintf(numstr, "Time: %.4fs",
		lasttime
		);
	writeString(numstr, 3, 36, GLUT_BITMAP_9_BY_15);

	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	glVertex2f(0, gH - 40);
	glVertex2f(gW, gH - 40);
	glVertex2f(gW, gH);
	glVertex2f(0, gH);
	glEnd();

	glColor3f(1, 1, 1);
	sprintf(numstr, "[Q]uit | [D]istr | [S]ort | [G]enerate | Elements: [L/R/PGUP/PGDN] | [V]isual | reset: HOME");
	writeString(numstr, 3, gH - 18, GLUT_BITMAP_9_BY_15);
	// Меняем буфферы (антимерцание, double buffering)
	glutSwapBuffers();
}

//////////////////////////

// Самый быстрый шейкер, без анимации и видимых изменений (кроме конечного результата), бенчмарк
void fast_shake()
{
	compars = 0;
	register int a;
	int exchange;
	ColorBar t;

	do {
		exchange = 0;
		for(a=gNumblocks-1; a > 0; --a) {
			if(allBars[a-1].hue > allBars[a].hue) {
				//				t = allBars[a-1];
				t.hue = allBars[a-1].hue;
				t.h = allBars[a-1].h;
				//				allBars[a-1] = allBars[a];
				allBars[a-1].hue = allBars[a].hue;
				allBars[a-1].h = allBars[a].h;
				//				allBars[a] = t;
				allBars[a].hue = t.hue;
				allBars[a].h = t.h;
				exchange = 1;
			}
			compars++;
		}

		for(a=1; a < gNumblocks; ++a) {
			if(allBars[a-1].hue > allBars[a].hue) {
				//				t = allBars[a-1];
				t.hue = allBars[a-1].hue;
				t.h = allBars[a-1].h;
				//				allBars[a-1] = allBars[a];
				allBars[a-1].hue = allBars[a].hue;
				allBars[a-1].h = allBars[a].h;
				//				allBars[a] = t;
				allBars[a].hue = t.hue;
				allBars[a].h = t.h;
				exchange = 1;
			}
			compars++;
		}
	} while(exchange); // sort until no exchanges take place
	glutPostRedisplay();
}

// То же самое, но в режиме записи действий
void record_shake()
{
	// Сбрасываем массив, чтобы был свежий
	// arrayFilled = false;
	// genArray();
	glutSetCursor(cursors[3]);
	glutPostRedisplay();

	allBarsBackup = (ColorBar*)calloc(gNumblocks + 1, sizeof(ColorBar));
	memcpy(allBarsBackup, allBars, gNumblocks * sizeof(ColorBar));

	compars = 0;
	recordSteps = 0;
	register int a;
	int exchange;
	ColorBar t;

	lasttime = 0;
	if(recording!= NULL)
	{
		free(recording);
		recording=NULL;
		playbackCurStepN = 0;
		recordSteps = 0;
	}

	if(recording == NULL)
	{
		recording = (Record*) calloc(1, sizeof(Record));
	}
	Record curStep;

	do {
		recording = (Record*) realloc(recording, sizeof(Record) * (recordSteps + 1) + sizeof(Record) * gNumblocks * 4 + 8);

		exchange = 0;

		for(a=gNumblocks-1; a > 0; --a) {
			curStep.action = 'a';
			curStep.l_id = a-1;
			curStep.r_id = a;
			recording[recordSteps++] = curStep;

			if(allBarsBackup[a-1].hue > allBarsBackup[a].hue) {

				curStep.action = 's';
				curStep.l_id = a-1;
				curStep.r_id = a;
				recording[recordSteps++] = curStep;

//				t.hl = allBarsBackup[a-1].hl = allBarsBackup[a].hl = true;
				t.hue = allBarsBackup[a-1].hue;
				t.h = allBarsBackup[a-1].h;
				allBarsBackup[a-1].hue = allBarsBackup[a].hue;
				allBarsBackup[a-1].h = allBarsBackup[a].h;
				allBarsBackup[a].hue = t.hue;
				allBarsBackup[a].h = t.h;
				exchange = 1;
			}
			compars++;
		}

		for(a=1; a < gNumblocks; ++a) {
			curStep.action = 'a';
			curStep.l_id = a-1;
			curStep.r_id = a;
			recording[recordSteps++] = curStep;


			if(allBarsBackup[a-1].hue > allBarsBackup[a].hue) {
				curStep.action = 's';
				curStep.l_id = a-1;
				curStep.r_id = a;
				recording[recordSteps++] = curStep;


				//				t = allBars[a-1];
				t.hue = allBarsBackup[a-1].hue;
				t.h = allBarsBackup[a-1].h;
				//				allBars[a-1] = allBars[a];
				allBarsBackup[a-1].hue = allBarsBackup[a].hue;
				allBarsBackup[a-1].h = allBarsBackup[a].h;
				//				allBars[a] = t;
				allBarsBackup[a].hue = t.hue;
				allBarsBackup[a].h = t.h;
				exchange = 1;
			}
			compars++;
		}
	} while(exchange); /* sort until no exchanges take place */
	
	free(allBarsBackup); 
	allBarsBackup = NULL;

	glutSetCursor(cursors[0]);
	glutPostRedisplay();

}


//////////////////////////

void writeString(char* string, int x, int y, void* font)
{
	glRasterPos2i(x, y);
	int len = strlen(string);
	for (int i=0; i < len; i++)
	{
		glutBitmapCharacter(font, string[i]);
	}
}

// Обработчик нажатия мыши
void glutMouse(int button, int state, int x, int y) 
{
	// Запоминаем состояние кнопки
	if(state == GLUT_DOWN) 
	{
		switch(button) 
		{
		case GLUT_LEFT_BUTTON:
			mState = DOWN;
			oldX = x;
			oldY = y;
			break;
		case GLUT_RIGHT_BUTTON:
			break;
		case GLUT_MIDDLE_BUTTON:
			break;
		}
	}
	else if (state == GLUT_UP) 
	{
		mState = UP;
	}
}

// Обработчик движения мыши
void glutMotion(int x, int y) 
{
	// Если кнопка нажата, вращаем сцену
	if (mState == DOWN) 
	{
		xPan -= (oldY - y);
		yPan -= (oldX - x);
		glutPostRedisplay();
	} 
	oldX = x; 
	oldY = y;
}

void glutIdle(void)
{
	// Если анимация включена, обновляем кадр
//	if (playback)
//	{
//		glutPostRedisplay();
//	}
}

void glutTimer(int value)
{
	if(progMode == PRMODE_VISUAL && playback)
	{
		playbackCalcStep();
		glutPostRedisplay();
	}
	glutTimerFunc(40, glutTimer, 0);
}

void glutVisible(int vis)
{
	if(vis == GLUT_VISIBLE)
	{
		glutIdleFunc(glutIdle);
	}
	else
	{
		glutIdleFunc(NULL);
	}
}

int main(int argc, char *argv[])
{
	srand ( time(NULL) );

	// Инициализация GLUT
	glutInit(&argc, argv);
	// Врубаем двойную буфферизацию (антимерцание), режим rgba и буффер глубины
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);

	// Инициализация полноэкранного "игрового" режима - упрощаем жизнь
	//glutGameModeString( modes[2] );
	glutEnterGameMode();

	// Callback функции: клава, мышь, перерисовка, холостые
	glutDisplayFunc(glutDisplay);
	glutKeyboardFunc(glutKeyboard);
	glutSpecialFunc(glutKeyboardSpec);
	glutReshapeFunc(glutReshape);
	glutMotionFunc (glutMotion);
	glutMouseFunc (glutMouse);
	glutVisibilityFunc(glutVisible);
	glutTimerFunc(40, glutTimer, 0);

	// Настройка параметров отрисовки и освещения
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gW = glutGet(GLUT_SCREEN_WIDTH);
	gH = glutGet(GLUT_SCREEN_HEIGHT);
	genArray();

	// Запускаем основной цикл GLUT
	glutMainLoop();

	// Успешно сваливаем (afaik, не должно быть доступно, GLUT старьё и не предусматривает выхода из main loop)
	return EXIT_SUCCESS;
}


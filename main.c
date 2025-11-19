#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>


#define FPS 300.0f
#define g 9.81
#define PI 3.141
#define MULTIPLICATOR 1 // scales the window ( only affects the visualization not the values of the objects )
#define WIDTH 130 // keep them dividable by 2
#define HEIGHT 80 // keep them dividable by 2
#define PIXELS_PER_METER 10.
#define GLOBAL_COLLISIONDAMPING .9 // both velocities get multiplied by this value at collision
#define EXTERN_MAX_FORCE 5000.
#define BALL_COUNT 20

// ansi codes
#define ESCAPE_CODE "\x1B["
#define CLEAR_CODE ESCAPE_CODE"2J"
#define SETPOSITION_CODE ESCAPE_CODE"%d;%dH"
#define HIDECURSOR_CODE ESCAPE_CODE"?25l"
#define SHOWCURSOR_CODE ESCAPE_CODE"?25h"


// disp mat
bool disp[HEIGHT * MULTIPLICATOR][WIDTH * MULTIPLICATOR] = {0};

void disp_clear()
{
	memset(disp, 0, sizeof(disp));
}
void disp_draw(WINDOW* win){
	char line[WIDTH * MULTIPLICATOR + 1];
	for ( int y = 0; y < HEIGHT * MULTIPLICATOR; y += 2)
	{
		
		for ( int x = 0; x < WIDTH * MULTIPLICATOR; ++x)
		{
			char symbols[] = {' ','"', '_', 'O'};
			int index = disp[y+1][x] << 1 | disp[y][x];
			line[x] = symbols[index];
		}
		line[WIDTH * MULTIPLICATOR] = 0;
		mvwprintw(win, y/2, 0, "%s", line);
	}
}

// helper structs
typedef struct
{
	float x, y;
}v2;

void v2_addTo(v2* first, v2 second)
{
	first->x += second.x;
	first->y += second.y;
}
v2 v2_add(v2 first, v2 second)
{
	return (v2){first.x + second.x, first.y + second.y};
}
void v2_subFrom(v2* first, v2 second)
{
	first->x += second.x;
	first->y += second.y;
}
v2 v2_div(v2 vec, float val)
{
	return (v2){vec.x / val, vec.y / val};	
}
v2 v2_mult(v2 vec, float val)
{
	return (v2){vec.x * val, vec.y * val};
}
v2 v2_sub(v2 first, v2 second)
{
	return (v2){first.x - second.x, first.y - second.y};
}
float v2_len(v2 vec){
	return sqrtf(vec.x * vec.x + vec.y * vec.y);
}
v2 v2_v0(v2 vec){
	float vecLength = v2_len(vec);
	return v2_div(vec, vecLength);
}

// Ball
typedef struct
{
	v2 pos, vel;
	float rad;
	float m; // mass

}ball_t;

// Ball prototypes
void ball_update(ball_t* ball);
void ball_draw(ball_t* ball);
float ball_getGAcc();
v2 ball_getNextPos(ball_t* ball);

float ball_getGAcc()
{
	return g/FPS;
}
v2 ball_getNextVel(ball_t* ball)
{
	v2 vel = ball->vel;
	vel.y += ball_getGAcc();
	return vel;
}

v2 ball_getNextPos(ball_t* ball)
{
	v2 pos = ball->pos;
	v2 vel = ball_getNextVel(ball);
	
	v2_addTo(&pos, v2_div(vel, FPS / PIXELS_PER_METER));
  	return pos;
}
void ball_update(ball_t* ball)
{
	v2* pos = &ball->pos;
	v2* vel = &ball->vel;
	*pos = ball_getNextPos(ball);
	*vel = ball_getNextVel(ball);
	//*vel = v2_mult(*vel, GLOBAL_DAMPING_COEFFICIENT);

	// check borders
	if (pos->x >= WIDTH - ball->rad || pos->x <= ball->rad)
	{
		pos->x = (pos->x >= WIDTH - ball->rad) ? WIDTH - ball->rad : ball->rad;
		vel->x *= -1 * GLOBAL_COLLISIONDAMPING;
	}
	if (pos->y >= HEIGHT - ball->rad || pos->y <= 1 + ball->rad)
	{
		pos->y = (pos->y >= HEIGHT - ball->rad) ? HEIGHT - ball->rad : 1 + ball->rad;
		vel->y *= -1 * GLOBAL_COLLISIONDAMPING;
	}

	ball_draw(ball);
}

void ball_draw(ball_t* ball)
{
	v2 pos = v2_mult(ball->pos, MULTIPLICATOR);
	float rad = ball->rad * MULTIPLICATOR;

	int startX = (int)pos.x - (int)rad - 2;
	int startY = (int)pos.y - (int)rad - 2;

	for ( int y = startY; y < startY + 2* (int)rad + 4; ++y)
	{
		for ( int x = startX; x < startX + 2* (int)rad + 4; ++x)
		{
			float diffX = pos.x - (float)x;
			float diffY = pos.y - (float)y;
			float dist = sqrtf(diffX * diffX + diffY * diffY);

			if (dist < rad)
				disp[y][x] = 1;

		}	
	}
}

float ball_getEnergy(ball_t* ball)
{	
	float kinEnergy = 0.5f * ball->m * pow(v2_len(ball->vel), 2);
	float potEnergy = ball->m * g * (HEIGHT - ball->pos.y) / PIXELS_PER_METER;

	return kinEnergy + potEnergy;
}

void ball_handleMouseMovement(ball_t* ball, v2 mousePos)
{
	// assume ball handle is not NULL

	v2 vBetweenOldAndNewPos = v2_sub(mousePos, ball->pos);

	v2 a = vBetweenOldAndNewPos;
	float maxA = EXTERN_MAX_FORCE / ball->m;
	if (v2_len(a) > maxA)
	{
		a = v2_mult(v2_v0(a), maxA);
	}

	v2_addTo(&ball->vel, v2_div(a, FPS));

}

v2 rotateCounterClockwise(v2 inp, float angle){
        return (v2){
                (float)(cos(angle) * inp.x  -  sin(angle) * inp.y),
                (float)(sin(angle) * inp.x  +  cos(angle) * inp.y)
        };
}
// Ball collision //////////
bool collBetweenBalls(ball_t* ball1, ball_t* ball2){
    // detect collision: ( calculating ahead )
    v2 distVec = v2_sub(ball_getNextPos(ball1), ball_getNextPos(ball2));
	v2 distVecCurr = v2_sub(ball1->pos, ball2->pos);
    float dist = v2_len(distVec);
	float distCurr = v2_len(distVecCurr);
	
    if(dist < ball1->rad + ball2->rad){
        // move the balls exactly ball1.radius + ball2.radius away from each other
		
        float difference = ball1->rad + ball2->rad - distCurr;
		float differenceCurr = ball1->rad + ball2->rad - distCurr;
        v2 movingVec = v2_mult(v2_v0(distVec), differenceCurr/2);
		if ( distCurr < 0)
    		v2_addTo(&ball2->pos, movingVec);
        	v2_subFrom(&ball1->pos, movingVec);

        // collision:    (only cares about the velocity)

        // calculate influence of v1
        distVec = v2_sub(ball_getNextPos(ball2), ball_getNextPos(ball1));
        float angleDiffVec = atan2(distVec.y, distVec.x);
        float angleVel1 = atan2(ball1->vel.y, ball1->vel.x);
        float angleDiff = angleDiffVec - angleVel1;
        //rotate v1 clockwise by angleDiff
        v2 rotv1 = rotateCounterClockwise(ball1->vel, angleDiff);
        v2 infVec1 = v2_mult(rotv1, cos(angleDiff));
        v2 untouchedVec1 = v2_mult(rotv1, sin(angleDiff)); // this is the velocity not affected by the collision
        untouchedVec1 = (v2){untouchedVec1.y, -untouchedVec1.x}; // rotate again by 90 degrees (Gegenkathete)
        float vel1 = v2_len(infVec1);
        // calculate influence of v2
        distVec = v2_sub(ball_getNextPos(ball1), ball_getNextPos(ball2));
        angleDiff = atan2(distVec.y, distVec.x) - atan2(ball2->vel.y, ball2->vel.x);
        //rotate v1 clockwise by angleDiff
        v2 rotv2 = rotateCounterClockwise(ball2->vel, angleDiff);
        v2 infVec2 = v2_mult(rotv2, cos(angleDiff));
        v2 untouchedVec2 = v2_mult(rotv2, sin(angleDiff)); // this is the velocity not affected by the collision
        untouchedVec2 = (v2){untouchedVec2.y, -untouchedVec2.x}; // rotate again by 90 degrees (Gegenkathete)
        float vel2 = v2_len(infVec2);

        float angleV1 = atan2(ball1->vel.y, ball1->vel.x);
        float angleDistVec = atan2(distVec.y, distVec.x);
        float angleV2 = atan2(ball2->vel.y, ball2->vel.x);
        if( fabsf(angleV1 - angleDistVec) > PI/2.f
        && fabsf(angleV2 - angleDistVec) < PI/2.f){
            vel2 = -vel2;
        }

        float resInfVel1Length = (ball1->m * vel1 + ball2->m * (2*vel2-vel1))/(ball1->m+ball2->m);
        float resInfVel2Length = (ball2->m * vel2 + ball1->m * (2*vel1-vel2))/(ball1->m+ball2->m);

        v2 resInfVel1 = v2_mult(v2_div(infVec1, vel1),resInfVel1Length * GLOBAL_COLLISIONDAMPING);
        v2 resInfVel2 = v2_mult(v2_div(infVec2, vel2), resInfVel2Length * GLOBAL_COLLISIONDAMPING);

        ball1->vel = v2_add(resInfVel1, untouchedVec1);
        ball2->vel = v2_add(resInfVel2, untouchedVec2);

		return true;
    }
	return false;
}


float exponential_moving_average(float newVal, float avg)
{
    const float alpha = 1e-3; // Faktor wieviel Einfluss der neue Wert hat

    avg = alpha * newVal + (1 - alpha) * avg;
    
    return avg;
}



int main(){
	
	// ncurses ////////////////////////////////////////
	//mouseinterval(1000);
	initscr();
	cbreak();
	noecho();
	wtimeout(stdscr, 0);
	mouseinterval(0);
  	// Enables keypad mode. This makes (at least for me) mouse events getting
  	// reported as KEY_MOUSE, instead as of random letters.
  	keypad(stdscr, TRUE);
  	// Don't mask any mouse events
  	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  	printf("\033[?1003h\n"); // Makes the terminal report mouse movement events
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK); // farbe hauptfenster. background black
	init_pair(2, COLOR_BLUE, COLOR_WHITE); // color for gui window. background blue
	// ncurses gui window: ////////
	int guiHeight = 15;
	int guiWidth = 50;
	int guiX = WIDTH*MULTIPLICATOR+1;
	int guiY = 0;
	WINDOW* gui = newwin(guiHeight, guiWidth, guiY, guiX);
	wbkgd(gui, COLOR_PAIR(2));
	box(gui, 0 ,0);
	
	//ncurses main window: //////
	int mainHeight = (HEIGHT/2)*MULTIPLICATOR+1;
	int mainWidth = WIDTH*MULTIPLICATOR+2;
	int mainX = 0;
	int mainY = 0;
	WINDOW* mainWin = newwin(mainHeight, mainWidth, mainY, mainX);
	wbkgd(mainWin, COLOR_PAIR(1));
	box(mainWin, 0, 0);

	// frame duration measuring ////////////////////////
	struct timespec start, end;
	float cpu_time_used;
	float cpu_time_avg = 0.0019; // a value that is near the expected value to shorten the settle time

	// energy calc /////////////
	float initEnergySys = 0;
	float currentEnergySys = 0;


	// init balls ///////////////////////////////////
	ball_t ballArr[BALL_COUNT];
	for (int i = 0; i < BALL_COUNT; ++i)
	{
		float x, y, rad, m;
		bool touchesAnotherBall = true;
		while(touchesAnotherBall)
		{
			touchesAnotherBall = false;
			rad = (float)(rand()%5) + 2;
			x = rad + (float)(rand()%(WIDTH - 2*(int)rad));
			y = rad + (float)(rand()%(HEIGHT - 2*(int)rad));
			m = 4.f/3.f * PI * pow(rad, 3.);
			for (int j = i-1; j >= 0; --j) // iterate over all previous balls and check if the distance is less than the sum of the radi
			{
				v2 connectionVec = v2_sub((v2){x, y}, ballArr[j].pos);
				float dist = v2_len(connectionVec);
				touchesAnotherBall = touchesAnotherBall || dist < rad + ballArr[j].rad;
			}
		}

		ballArr[i] = (ball_t){
			.pos = (v2){x, y},
			.vel = (v2){0, 0},
			.rad = rad,
			.m = m,
		};

		initEnergySys += ball_getEnergy(&ballArr[i]);
	}
	
	printf(HIDECURSOR_CODE CLEAR_CODE);


	// ball grapping mechanism ///////////
	ball_t *heldBall = NULL;
	v2 mousePos;


	// ball spawning ////////////
	float currentRad = 1;
	float currentM = 1;
	float deltaForCurrent = 0.5;

	bool running = true;
	while(running)
	{
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

		// handle mouse input
		int c = wgetch(stdscr);
    	char buffer[512];
    	size_t max_size = sizeof(buffer);
		switch(c)
		{
			case KEY_MOUSE:
			{
				MEVENT event;
				if (getmouse(&event) == OK) {
					event.y *= 2;
					mousePos = v2_div((v2){event.x, event.y}, MULTIPLICATOR);
        			// mouse fine ye
					switch(event.bstate)
					{
						case BUTTON1_PRESSED:
						{
							if (!heldBall)
							{
								// try to grap a new ball
						
								for (int i = 0; i < BALL_COUNT; ++i)
								{
									
									v2 distVec = v2_sub(mousePos, ballArr[i].pos);
									float dist = v2_len(distVec);
									if ( dist < ballArr[i].rad )
									{
										// a ball collided with the mouse cursor
										heldBall = &ballArr[i];
									}
								}
							}
						}break;
						case BUTTON1_RELEASED:
						{
							if (heldBall)
							{
								heldBall = NULL;
							}
						}break;
					}
					/*if (heldBall)
					{
						mousePos = (v2){(float)event.x, (float)event.y};
					}*/
				}
			}break;
			case KEY_EXIT:
				running = false;
			break;
			case KEY_UP:
				currentRad += deltaForCurrent;
			break;
			case KEY_DOWN:
				currentRad -= deltaForCurrent;
			break;
			case KEY_RIGHT:
				currentM += deltaForCurrent;
			break;
			case KEY_LEFT:
				currentM -= deltaForCurrent;
			break;
		}
      		
		
		if (heldBall)
			ball_handleMouseMovement(heldBall, mousePos);

		
		
		disp_clear();
		for (int i = 0; i < BALL_COUNT; ++i)
		{
			ball_update(&ballArr[i]);
		}
		disp_draw(mainWin);
		
		
		
		currentEnergySys = 0;
		for (int i = 0; i < BALL_COUNT; ++i)
		{
			currentEnergySys += ball_getEnergy(&ballArr[i]);

			for (int j = 0; j < BALL_COUNT; ++j)
			{
				if ( i != j)
				{
					collBetweenBalls(&ballArr[i], &ballArr[j]);
				}
			}
		}
		
		float averadeOfPeriod = cpu_time_used;
		mvwprintw(gui, 1, 2, "Periodendauer der Logik: %.4fms", cpu_time_avg*1000); // is actually always of the previous frame
		mvwprintw(gui, 2, 2, "=> maximale Frequenz: %.2f Hz", 1/cpu_time_avg);
		mvwprintw(gui, 4, 2, "Neuer Ball | Radius: %.2f\t(hoch, runter)", currentRad);
		mvwprintw(gui, 5, 2, "Neuer Ball | Masse: %.2f\t(links, rechts)", currentM);
		mvwprintw(gui, 7, 2, "Gesamt Energie bei Start: %.2fJ", initEnergySys);
		mvwprintw(gui, 8, 2, "Momentane Gesamt Energie: %.2fJ", currentEnergySys);
		wrefresh(gui);
		wrefresh(mainWin);
		
		
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		cpu_time_used = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
		cpu_time_avg = exponential_moving_average(cpu_time_used, cpu_time_avg);
		
		usleep(1000000.0f/FPS);
	}
	printf(SHOWCURSOR_CODE);

	delwin(gui);
	endwin();
	return 0;
}

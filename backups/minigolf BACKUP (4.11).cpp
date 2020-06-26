#include <iostream>
#include <curses.h>
#include <stdlib.h>
#include <vector>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h> 
#include <string>
#include <fstream>

#define PI 3.14159265359

using namespace std;


struct point2d{int x, y;};

float oldStarX[4] = {0};
float oldStarY[4] = {0};

int kbhit(void) //moze sie przyda
{
    int ch = getch();

    if (ch != ERR) {
        ungetch(ch);
        return 1;
    } else {
        return 0;
    }
}
class Map
{
private:
    char **mapPointer, **mapPointerOriginal;
    int rows = 30;
    int cols = 64;
public:
    Map()
    {
        mapPointer = new char*[rows];
        mapPointerOriginal = new char*[rows];
        for(int i = 0; i < rows; i++)
        {
            mapPointer[i] = new char[cols];
            mapPointerOriginal[i] = new char[cols];
        }
    }
    ~Map()
    {
        for(int i = 0; i < rows; i++){
            delete[] mapPointer[i];
            delete[] mapPointerOriginal[i];
        } 
        delete[] mapPointer;
        delete[] mapPointerOriginal;
    }
    void readFromTextFile()
    {
        char ch;
        fstream textFile("map", fstream::in);
        textFile.unsetf(ios_base::skipws);
        for(int i = 0; i < rows; i++)
            for(int j = 0; j < cols; j++){
                textFile >> mapPointer[i][j];
                mapPointerOriginal[i][j] = mapPointer[i][j];
            }
                
        textFile.close();
    }
    char getCharAt(int x, int y)
    {
        if(x < 0 || y < 0) return '0';
        if(x > rows-1 || y > cols-1) return '0';
        return mapPointer[x][y];
    }
    char getOryginalCharAt(int x, int y)
    {
        if(x < 0 || y < 0) return '0';
        if(x > rows-1 || y > cols-1) return '0';
        return mapPointerOriginal[x][y];
    }
    void setCharAt(int x, int y, char ch){
        if(x>0 && y>0 && x < rows && y < cols)
            mapPointer[x][y] = ch;
    }
    void setCharAt(float x, float y, char ch){
        if(x>0 && y>0 && x < rows && y < cols)
            mapPointer[static_cast<int>(x)][static_cast<int>(y)] = ch;

    }
    void resetCharAt(float x, float y)
    {
        if(x>0 && y>0 && x < rows && y < cols)
        mapPointer[static_cast<int>(x)][static_cast<int>(y)] = mapPointerOriginal[static_cast<int>(x)][static_cast<int>(y)];
    }
    int getRows(){return rows;}
    int getCols(){return cols;}
    void drawMap()
    {
        for(int i = 0; i < rows; i++)
            for(int j = 0; j < cols; j++)
            {
                if(mapPointer[i][j] == '#')
                    mvaddch(i,j, ' ' | A_REVERSE);
                else if(mapPointer[i][j] == 'a')
                    mvaddch(i,j, ACS_LLCORNER);
                else if(mapPointer[i][j] == 'b')
                    mvaddch(i,j, ACS_LRCORNER);
                else if(mapPointer[i][j] == 'c')
                    mvaddch(i,j, ACS_VLINE);
                else if(mapPointer[i][j] == '.')
                    mvaddch(i,j, ' ');
                else
                    mvaddch(i,j, mapPointer[i][j]); 
            }
                
    }
};

class Point
{
public:
    float speed;
    float x, y; //x, y pos
    float oldX, oldY; //one step back;
    float stickX, stickY;
    float vx, vy; // x, y velocity
    float friction; // in percents
    float theta, thetaSpeed, oldTheta;
    bool ballHit;
    float ballTimer, ballTimerMax, ballTimerStep;
    float power, powerStep;
    enum State{aiming, rolling, hitting};
    State state;
    Map map;

    Point()
    {
        oldX = oldY = vx = vy =0;
        x = 7;
        y = 7;
        speed = 0.014;
        friction = 0.99980;
        theta = oldTheta = 0;
        thetaSpeed = 0.06;
        stickX = stickY = 0;
        ballTimer = ballTimerMax = 6;
        ballTimerStep = 0.01;
        power = 6;
        powerStep = 0.5;
        state = aiming;
        drawBall(x, y);
        map.readFromTextFile();
        

    }

    void updateState(char ch)
    {
        string log = to_string(vx+vy);
        mvaddstr(0,0, log.c_str());
        if(state == aiming && ch == ' ' )
        {
            state = hitting;
            
        }
        else if(state == rolling && fabs(vx+vy) < 0.00001)
        {
            state = aiming;
        }
        else if(state == hitting && ballTimer < 0.2)
        {
            map.setCharAt(stickX, stickY, '.');
            map.setCharAt(stickX-1, stickY, '.');
            vx += (power/4)*speed*sin(theta - PI/2);
            vy += (power/4)*speed*cos(theta + PI/2);
            ballTimer = power;
            state = rolling;   
        }
    }
    
    void updateGame(char ch)
    {
        map.drawMap();
        updateState(ch);
        if(state == aiming)
        {
            vx = vy = 0;
            switch(ch)
            {
                case 'a' : {
                    theta += thetaSpeed;
                    break;
                }
                case 'd' : {
                    theta -= thetaSpeed;
                    break;
                }
                case 'w' : {
                    if(power+powerStep < 10)
                        power += powerStep;
                    break;
                }
                case 's' : {
                    if(power-powerStep > 1)
                        power -= powerStep;
                    break;
                }
            }
            drawBall(x,y);
            drawStick(power);
        }
        else if(state == rolling)
        {

            collisionHandling();

            y += vy;
            x += vx;
            vx *= friction;
            vy *= friction;
            drawBall(static_cast<int>(x),static_cast<int>(y));
        }
        else if(state == hitting)
        {
            hitBall();
        }
        
    }

    void collisionHandling()
    {
        if( map.getCharAt(x+0.5, y) == '#' || map.getCharAt(x-0.5, y) == '#' ) vx = -vx;
        if( map.getCharAt(x, y+0.5) == '#' || map.getCharAt(x, y-0.5) == '#' ) vy = -vy;
    }

    void hitBall()
    {
        drawStick(ballTimer);
        ballTimer -= ballTimerStep;
    }

    void drawStick(float distance)
    {
        
        map.setCharAt(stickX, stickY, map.getOryginalCharAt(stickX, stickY));
        map.setCharAt(stickX-1, stickY, map.getOryginalCharAt(stickX-1, stickY));

        float ps = distance*sin(theta);
        float pc = distance*cos(theta);
        stickX = x+pc;
        stickY = y+ps;
        map.setCharAt(stickX-1, stickY, 'c'); //ACS_VLINE
        if(ps > 0)
            map.setCharAt(stickX, stickY, 'b'); //ACS_LRCORNER
        else
            map.setCharAt(stickX, stickY, 'a'); //ACS_LLCORNER
        
        for(auto starX : oldStarX)
            for(auto starY : oldStarY)
                map.setCharAt(starX, starY, map.getOryginalCharAt(starX, starY));

        for(int i = 1; i < 5; i++)
        {
            float starX = x+i*2*(float)sin(theta - PI/2);
            float starY = y+i*2*(float)cos(theta + PI/2);
            oldStarX[i-1] = starX;
            oldStarY[i-1] = starY;
            //map.setCharAt(x+i*(float)sin(oldTheta - PI/2), y+i*(float)cos(oldTheta + PI/2), ' ');
            map.setCharAt(starX, starY, '*');
            //oldTheta = theta;
        }
        
    }

    void drawBall(int x, int y)
    {
        map.setCharAt(oldX, oldY,map.getOryginalCharAt(oldX, oldY));
        map.setCharAt(x, y,'o');
        oldX = x;
        oldY = y;
    }
};



void mainloop(Point& point, char ch)
{
    point.updateGame(ch);
    usleep(100);
}

int main()
{
WINDOW * mainwin;
if ( (mainwin = initscr()) == NULL )
    {
        fprintf(stderr, "Error initialising ncurses.\n");
        exit(0);
    }
curs_set(0);
cbreak();
noecho();
nodelay(stdscr, TRUE);
scrollok(stdscr, TRUE);

Point point;

while(true)
{
    char ch = getch();
    if (ch == 'e') break;
    mainloop(point, ch);
}
    

delwin(mainwin);
endwin();
refresh();
cout<< "Thanks for playing!\n";
return 0;
}
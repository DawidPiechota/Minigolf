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

vector<point2d> points;

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
    char** mapPointer;
    int rows = 30;
    int cols = 64;
public:
    Map()
    {
        mapPointer = new char*[rows];
        for(int i = 0; i < rows; i++)
        {
            mapPointer[i] = new char[cols];
        }
    }
    ~Map()
    {
        for(int i = 0; i < rows; i++)
            delete[] mapPointer[i];
        delete[] mapPointer;
    }
    void readFromTextFile()
    {
        char ch;
        fstream textFile("map", fstream::in);
        textFile.unsetf(ios_base::skipws);
        for(int i = 0; i < rows; i++)
            for(int j = 0; j < cols; j++)
                textFile >> mapPointer[i][j];
        textFile.close();
    }
    char getCharAt(int x, int y)
    {
        if(x < 0 || y < 0) return '0';
        if(x > rows || y > cols) return '0';
        return mapPointer[x][y];
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
                else
                    mvaddch(i,j, '.'); 
            }
                
    }
};

class Point
{
public:
    float sleepTime;
    float speed;
    float x, y; //x, y pos
    float oldX, oldY; //one step back;
    float stickX, stickY;
    float vx, vy; // x, y velocity
    float friction; // in percents
    int floor, wall; //floor lvl
    float theta, thetaSpeed;
    bool ballHit;
    enum State{aiming, rolling};
    State state;
    Map map;

    Point()
    {
        oldX = oldY = vx = vy =0;
        x = 7;
        y = 7;
        speed = 0.014;
        sleepTime = 1/60;
        floor = 50;
        wall = 50;
        friction = 0.99980;
        theta = 0;
        thetaSpeed = 0.06;
        stickX = stickY = 0;
        state = aiming;
        drawBall(x, y);
        map.readFromTextFile();
        

    }

    void updateState(char ch)
    {
        string log = to_string(vx+vy);
        mvaddstr(0,0, log.c_str());
        if(ch == ' ' && state == aiming)
        {
            hitBall();
            state = rolling;
        }
        else if(state == rolling && fabs(vx+vy) < 0.00001)
        {
            state = aiming;
        }
    }
    
    void update(char ch)
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
            }
            drawBall(x,y);
            drawStick(6, false);
        }
        else if(state = rolling)
        {
            //switch(ch)
            //{
            //    case 'a': vy-= speed; break;
            //    case 'd': vy+= speed; break;
            //    case 'w': vx-= speed; break;
            //    case 's': vx+= speed; break;
            //    case 'e': exit(0);
            //}

            //if(x + vx < 0 || x + vx > floor) vx = -vx;
            //if(y + vy < 0 || y + vy > wall) vy = -vy;
            collisionHandling();

            y += vy;
            x += vx;
            vx *= friction;
            vy *= friction;
            drawBall(static_cast<int>(x),static_cast<int>(y));
        }
        
    }

    void collisionHandling()
    {
        //if( map.getCharAt(static_cast<int>(x+1), static_cast<int>(y)) == '#' || map.getCharAt(static_cast<int>(x-1), static_cast<int>(y)) == '#' ) vx = -vx;
        //if( map.getCharAt(static_cast<int>(x), static_cast<int>(y+1)) == '#' || map.getCharAt(static_cast<int>(x), static_cast<int>(y-1)) == '#' ) vx = -vx;
        int intX = static_cast<int>(x);
        int intY = static_cast<int>(y);
        //if( map.getCharAt(intX+1, intY) == '#' || map.getCharAt(intX-1, intY) == '#' ) vx = -vx;
        //if( map.getCharAt(intX, intY+1) == '#' || map.getCharAt(intX, intY-1) == '#' ) vy = -vy;
        //if( map.getCharAt(intX+1, intY+1) == '#' || map.getCharAt(intX+1, intY-1) == '#' ){ vx = -vx; vy = -vy; }
        //if( map.getCharAt(intX-1, intY+1) == '#' || map.getCharAt(intX-1, intY-1) == '#' ){ vx = -vx; vy = -vy; }
        if( map.getCharAt(x+0.5, y) == '#' || map.getCharAt(x-0.5, y) == '#' ) vx = -vx;
        if( map.getCharAt(x, y+0.5) == '#' || map.getCharAt(x, y-0.5) == '#' ) vy = -vy;
    }

    void hitBall()
    {
        //mvaddch(static_cast<int>(stickX), static_cast<int>(stickY), ' ');
        //mvaddch(static_cast<int>(stickX) - 1, static_cast<int>(stickY), ' ');
        for(int i = 6; i > 0; i--)
        {
            drawStick(i, true);
            drawBall(x,y);
            usleep(50000);
        }
        vx += speed*sin(theta - PI/2);
        vy += speed*cos(theta + PI/2);
    }

    void drawStick(int distance, bool clearFlag) //clear flag- if set, clear screen behind stick
    {
        
        if( clearFlag )
        {
            mvaddch(static_cast<int>(stickX), static_cast<int>(stickY), ' ');
            mvaddch(static_cast<int>(stickX) - 1, static_cast<int>(stickY), ' ');
        }
        float ps = distance*sin(theta);
        float pc = distance*cos(theta);
        stickX = x+pc;
        stickY = y+ps;
        mvaddch(static_cast<int>(stickX-1), static_cast<int>(stickY), ACS_VLINE);
        if(ps > 0)
            mvaddch(static_cast<int>(stickX), static_cast<int>(stickY), ACS_LRCORNER);
        else 
            mvaddch(static_cast<int>(stickX), static_cast<int>(stickY), ACS_LLCORNER);
        refresh();
    }

    void drawBall(int x, int y)
    {
        //mvaddch(oldX, oldY, ' ');
        mvaddch(x,y,'o');
        oldX = x;
        oldY = y;
        //for(int i = 0; i < floor + 1; i++)
        //    mvaddch(i, wall, '#');
        //mvaddstr(floor, 0, "####################################################################################################");
        refresh();
    }
};



void mainloop(Point& point, char ch)
{
    point.update(ch);
    //sleep(point.sleepTime);
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
points.push_back(point2d{0,0});

while(true)
{
    char ch = getch();
    if (ch == 'e') break;
    mainloop(point, ch);
}
    

delwin(mainwin);
endwin();
refresh();
//cout<< "Finished\n";
return 0;
}
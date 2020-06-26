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

float oldStarX[12] = {0};
float oldStarY[12] = {0};

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
    int rows = 38;
    int cols = 100;
    int xAt, yAt = 0;
public:
    vector<char> best;
    Map()
    {
        mapPointer = new char*[rows];
        mapPointerOriginal = new char*[rows];
        for(int i = 0; i < rows; i++)
        {
            mapPointer[i] = new char[cols];
            mapPointerOriginal[i] = new char[cols];
        }
        
        ifstream textFile("stages/best");
            if(!textFile.good()){
                textFile.close();
                fstream textFile2("stages/best", fstream::out);
                for(int i = 0; i < getStageCount(); i ++){
                    textFile2 << "z";
                    best.push_back('z');
                }
                textFile2.close();
            }
            else{
                textFile.close();
                ifstream textFile2("stages/best", fstream::in);
                for(int i = 0; i < getStageCount(); i ++){
                    char ch;
                    textFile2 >> ch;
                    best.push_back(ch);
                }
                textFile2.close();
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
    void readFromTextFile(int stageCounter, float& _x, float& _y)
    {
        fstream textFile("stages/Map" + to_string(stageCounter), fstream::in);
        textFile.unsetf(ios_base::skipws);
        for(int i = 0; i < rows; i++)
            for(int j = 0; j < cols; j++){
                textFile >> mapPointer[i][j];
                if(mapPointer[i][j] == 'o'){
                    mapPointerOriginal[i][j] = mapPointer[i][j] = '.';
                    _x = i;
                    _y = j;
                }
                else if(mapPointer[i][j] == '@'){
                    mapPointerOriginal[i][j] = mapPointer[i][j] = '@';
                    xAt = i;
                    yAt = j;
                }
                else mapPointerOriginal[i][j] = mapPointer[i][j];
            }
                
        textFile.close();
    }
    void saveScore(int stageCount, int hitCount)
    {
        best[stageCount] = hitCount;
        fstream textFile("stages/best", fstream::in);
        for(char ch : best){
            textFile << ch;
        }
    }
    int getStageCount()
    {
        int counter;
        for(counter = 0; counter < 20; counter++){
            ifstream textFile("stages/Map" + to_string(counter));
            if(!textFile.good()) break;
            textFile.close();
        }
        return counter;
    }
    int getXAt(){
        return xAt;
    }
    int getYAt(){
        return yAt;
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
                else if(mapPointer[i][j] == 'o')
                    mvaddch(i,j, 'o');
                else if(mapPointer[i][j] == '*')
                {
                    if(mapPointerOriginal[i][j] == '#' )
                        mvaddch(i,j, '.' | A_REVERSE); //albo po prostu ACS_CKBOARD
                    else if(mapPointerOriginal[i][j] == '@' )
                        mvaddch(i,j, '@' | A_REVERSE);
                    else mvaddch(i,j, '.');
                }
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
    float theta, thetaSpeed;
    bool ballHit;
    float ballTimer, ballTimerMax, ballTimerStep;
    float power, powerStep;
    int collisionTimerX, collisionTimerY;
    int winTimer, winTimerMax;
    int hitCounter;
    int stageCounter, stageCounterMax;
    float collisionDetectionRange, holeDetectionRange;
    enum State{aiming, rolling, hitting, win, finish};
    State state;
    Map map;

    Point()
    {
        oldX = oldY = vx = vy =0;
        x = 7;
        y = 7;
        speed = 0.010;
        friction = 0.99980;
        theta = 0;
        thetaSpeed = 0.06;
        stickX = stickY = 0;
        ballTimer = ballTimerMax = 6;
        ballTimerStep = 0.01;
        power = 6;
        powerStep = 0.5;
        collisionTimerX = collisionTimerY = 0;
        collisionDetectionRange = 0.7;
        holeDetectionRange = 0.5;
        winTimer = winTimerMax = 20000;
        state = aiming;
        stageCounter = 0;
        stageCounterMax = map.getStageCount();
        hitCounter = 0;
        map.readFromTextFile(0, x, y);
        drawBall(x, y);
    }

    void updateState(char ch)
    {
        char converted = map.best[stageCounter]-96;
        string log = "V: " + to_string(fabs(vx)+fabs(vy)) + 
        " | Win timer: " + to_string(winTimer) +
        " | Power: " + to_string(power) +
        " | Stage: " + to_string(stageCounter+1) + "/" + to_string(stageCounterMax) +
        " |Hits: " + to_string(hitCounter) + " |Record: ";
        if(converted == -51) log += "-";
        else log += to_string(converted);
        //for(char ch : map.best)
        //    log += ch;
        mvaddstr(0,0, log.c_str());
        log = "a/d- aiming | w/s- power | space- hit | e- exit";
        mvaddstr(map.getRows()+1,25,  log.c_str());
        if(state == aiming && ch == ' ' )
        {
            state = hitting;
            
        }
        else if(state == rolling)
        {
            if(fabs(vx)+fabs(vy) < 0.000050)
            {
                resetCollisionTimer();
                vx = vy = 0;
                state = aiming;
            }
            if(/*fabs(vx)+fabs(vy) < 0.1 && */map.getOryginalCharAt(x, y) == '@' ||
            map.getOryginalCharAt(x+holeDetectionRange, y) == '@' || map.getOryginalCharAt(x+holeDetectionRange, y+holeDetectionRange) == '@' || map.getOryginalCharAt(x+holeDetectionRange, y-holeDetectionRange) == '@'||
            map.getOryginalCharAt(x-holeDetectionRange, y) == '@' || map.getOryginalCharAt(x-holeDetectionRange, y+holeDetectionRange) == '@' || map.getOryginalCharAt(x-holeDetectionRange, y-holeDetectionRange) == '@')
            {
                vx = vy = 0;
                state = win;
            }
        }
        else if(state == hitting && ballTimer < 0.2)
        {
            hitCounter++;
            map.resetCharAt(stickX, stickY);
            map.resetCharAt(stickX-1, stickY);
            vx += (power/4)*speed*sin(theta - PI/2);
            vy += (power/4)*speed*cos(theta + PI/2);
            ballTimer = power;
            resetStars();
            state = rolling;   
        }
        else if(state == win)
        {
            map.resetCharAt(x,y);
            map.setCharAt(map.getXAt(), map.getYAt(), 'o');
            if(winTimer < 1)
            {
                if(++stageCounter == stageCounterMax){
                    hitCounter = 0;
                    winTimer = winTimerMax;
                    state = finish;
                }
                else{
                    map.readFromTextFile(stageCounter, x, y);
                    hitCounter = 0;
                    winTimer = winTimerMax;
                    state = aiming;
                }
            }
        }
    }
    
    void updateGame(char ch){
        map.drawMap();
        updateState(ch);
        if(state == aiming){
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
                    if(power+powerStep < 10.3)
                        power += powerStep;
                    break;
                }
                case 's' : {
                    if(power-powerStep > 0.7)
                        power -= powerStep;
                    break;
                }
            }
            drawBall(x,y);
            drawStick(power);
        }
        else if(state == rolling){

            collisionHandling();

            y += vy;
            x += vx;
            vx *= friction;
            vy *= friction;
            drawBall(static_cast<int>(x),static_cast<int>(y));
        }
        else if(state == hitting){
            hitBall();
        }
        else if(state == win){
            mvaddstr(9,15, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
            mvaddstr(10,15, ("|                    You won with " + to_string(hitCounter) + " hits!                     |").c_str());
            if(map.best[stageCounter] > (char)(hitCounter+96) || map.best[stageCounter == -51]){
                mvaddstr(11,15, "|                        New record!                          |");
                map.saveScore(stageCounter, hitCounter);
            }else{
                mvaddstr(11,15, ("|                    Record is: " + to_string(map.best[stageCounter]-96) + "                             |").c_str());
            }
            mvaddstr(12,15, "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
            winTimer--;
        }
        else if(state == finish){
            mvaddstr(9,15, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
            mvaddstr(10,15,"|                        Game completed!                      |");
            mvaddstr(11,15,"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
        }
        
    }

    void collisionHandling()
    {
        if(collisionTimerX > 0) collisionTimerX--;
        if(collisionTimerY > 0) collisionTimerY--;

        if( (map.getCharAt(x+collisionDetectionRange, y) == '#' && vx > 0 ) || ( map.getCharAt(x-collisionDetectionRange, y) == '#' && vx < 0 ))
            if(collisionTimerX < 1)
            {
                collisionTimerX = 50; //depends on speed. If the ball is too fast it may be too much to catch another close collision
                vx = -vx;              //however if timer is set too low, it may detect one collision as many
            }
                

        if( ( map.getCharAt(x, y+collisionDetectionRange) == '#' && vy > 0 ) || ( map.getCharAt(x, y-collisionDetectionRange) == '#' && vy < 0 ))
            if(collisionTimerY < 1)
            {
                collisionTimerY = 50;
                vy = -vy;
            }
                
        
    }

    void resetCollisionTimer()
    {
        collisionTimerX = 0;
        collisionTimerY = 0;
    }

    void hitBall()
    {
        drawStick(ballTimer);
        ballTimer -= ballTimerStep;
    }

    void drawStick(float distance)
    {
        //stick
        map.resetCharAt(stickX, stickY);
        map.resetCharAt(stickX-1, stickY);

        float ps = distance*sin(theta);
        float pc = distance*cos(theta);
        stickX = x+pc+0.5;
        stickY = y+ps+0.5;
        map.setCharAt(stickX-1, stickY, 'c'); //ACS_VLINE
        if(ps > 0)
            map.setCharAt(stickX, stickY, 'b'); //ACS_LRCORNER
        else
            map.setCharAt(stickX, stickY, 'a'); //ACS_LLCORNER
        
        //stars
        resetStars();
        for(int i = 1; i < 13; i++)
        {
            float starX = x+0.5+power/5*i*5*(float)sin(theta - PI/2);
            float starY = y+0.5+power/5*i*5*(float)cos(theta + PI/2);
            oldStarX[i-1] = starX;
            oldStarY[i-1] = starY;
            map.setCharAt(starX, starY, '*');
        }
        
    }

    void resetStars()
    {
        for(auto starX : oldStarX)
            for(auto starY : oldStarY)
                map.resetCharAt(starX, starY);
    }

    void drawBall(int x, int y)
    {
        map.resetCharAt(oldX, oldY);
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

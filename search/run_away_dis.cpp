#include<iostream>
#include<cmath>
#include<iomanip>
#include<time.h>

#define Max1(x,y) ((x>y)?(x):(y))
#define Min1(x,y) ((x<y)?(x):(y))

using namespace std;

double sqrt2 = sqrt(2)/2;
double dir[8][2] = {{1,0},{0,1},{-1,0},{0,-1},
    {sqrt2,sqrt2},{sqrt2, -sqrt2},{-sqrt2,sqrt2},{-sqrt2,-sqrt2}};

inline bool out(double x1, double y1, int x, int y) {
    if(x1<0 || x1>x) return true;
    if(y1<0 || y1>y) return true;
    return false;
}
int main() 
{
    int T; cin>>T;
    srand((unsigned)time(NULL));
    while(T--) {
        int X,Y,M; cin>>X>>Y>>M;
        int *a = new int [M];
        int *b = new int [M];
        for(int i=0; i<M; ++i) cin>>a[i]>>b[i];
        double S=0,xbest,ybest;
        for(int tc=0; tc<5; ++tc) {//随机重启
            double x0=rand()%(X+1), y0 = rand()%(Y+1);
            double curdis=X+Y;
            for(int k=0; k<M; ++k) {
                double dis = sqrt((x0-a[k])*(x0-a[k])+(y0-b[k])*(y0-b[k]));
                curdis = Min1(curdis, dis);
            }
            for(double t = 1000; t>1e-6; t*=0.9) {
                for(int i = 0; i<8; ++i) {
                    double x1 = x0 + dir[i][0]*t;
                    double y1 = y0 + dir[i][1]*t;
                    if(out(x1,y1,X,Y)) continue;
                    double mindis = X+Y;
                    for(int k=0; k<M; ++k) {
                        double dis = sqrt((x1-a[k])*(x1-a[k])+(y1-b[k])*(y1-b[k]));
                        mindis = Min1(mindis, dis);
                    }
                    if(mindis > curdis) {
                        curdis = mindis;
                        x0=x1; y0=y1;
                        break;
                    }
                }
            }
            if(S<curdis) {
                S=curdis;
                xbest=x0; ybest=y0;
            }
        }
        printf("The safest point is (%.1lf, %.1lf).\n", xbest,ybest);
        delete [] a;
        delete [] b;
    }
    //system("pause");
    return 0;
}
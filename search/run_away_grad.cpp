// 随即重启，梯度上升，为什么会超时？对比另一个离散化空间的程序
#include<iostream>
#include<cmath>
#include<time.h>
#include<cstdlib>
#include<iomanip>

#define Max1(x,y) ((x>y)?(x):(y))
#define Min1(x,y) ((x<y)?(x):(y))
#define dec 0.9

using namespace std;

int main() { 
    int T; cin>>T;
    srand((unsigned)time(NULL));
    while(T--) {
        int X, Y, M; cin>>X>>Y>>M;
        int *a = new int [M];
        int *b = new int [M];
        for(int i=0; i<M; ++i) {
            cin>>a[i]>>b[i];
        }
        double tx,ty;
        double S=0;
        for(int t=0; t<5; ++t){
            double x0 = rand()%(X+1), y0 = rand()%(Y+1);
            double eps = Max1(X,Y)/5;
            // int cnt=0;
            while(1) {
                double gradx=0, grady=0;
                double min_dis=X+Y;
                for(int i=0; i<M; ++i){
                    double dis = sqrt((x0-a[i])*(x0-a[i])+(y0-b[i])*(y0-b[i]));
                    if(dis<min_dis) {
                        min_dis=dis;
                        gradx = (x0-a[i])/dis;
                        grady = (y0-b[i])/dis;
                    }
                }
                double x1 = x0 + eps*gradx;
                if(x1<0) x1=0; else if(x1>X) x1=X;
                double y1 = y0 + eps*grady;
                if(y1<0) y1=0; else if(y1>Y) y1=Y;
                if(abs(x0-x1)<1e-3 && abs(y0-y1)<1e-3)
                    break;
                x0=x1; y0=y1;
                eps *= dec;
                //下面这个剪枝起到十分重要的作用
                if(eps<1e-6)
                    break;
            }
            double s1=X+Y;
            for(int i=0; i<M; ++i){
                double dis = sqrt((x0-a[i])*(x0-a[i])+(y0-b[i])*(y0-b[i]));
                s1 = Min1(s1, dis);
            }
            if(S<s1) {
                S=s1;
                tx=x0; ty=y0;
            }
        }
        printf("The safest point is (%.1lf, %.1lf).\n", tx, ty);
        
        delete[] a;
        delete[] b;
    }
    //system("pause");
    return 0;
}

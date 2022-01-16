#include<iostream>
#include<cstring>
#include<cstdio>
#include<vector>
#include<stack>
#include<cmath>
#include<algorithm>

using namespace std;
const double eps = 1e-6;
inline bool is_zero(double x) {
    return x>-eps&&x<eps;
}
struct point{
    double x,y;
}P[1001];
bool visited[1001];
bool operator<(const point& a, const point& b) {
    if(a.x==b.x) {
        return a.y < b.y;
    }
    return a.x<b.x;
}
struct vec{
    double x,y;
};
vec operator-(point &a, point &b) {
    vec v; v.x=a.x-b.x;v.y=a.y-b.y;
    return v;
}
double cross(vec a, vec b) {
    return a.x*b.y-a.y*b.x;
}
int N;
vector<point> Graham() {
    //if(N<6) return 0;
    vector<point> S;
    S.clear();
    S.push_back(P[0]); S.push_back(P[1]);
    for(int i=2; i<N; ++i) {
        while(S.size() > 1) {
            point p2 = *(S.end()-1);
            point p1 = *(S.end()-2);
            if(cross(p2-p1, P[i]-p2) < -eps) {
                S.pop_back();
            }
            else if(is_zero(cross(p2-p1, P[i]-p2))) {
                break;
            }
            else {
                break;
            }
        }
        S.push_back(P[i]);
    }
    int size = S.size();
    S.push_back(P[N-2]);
    for(int i=N-3; i>=0; --i) {
        while(S.size() > size) {
            point p2 = *(S.end()-1);
            point p1 = *(S.end()-2);
            if(cross(p2-p1, P[i]-p2) < -eps) {
                S.pop_back();
            }
            else if(is_zero(cross(p2-p1, P[i]-p2))) {
                break;
            }
            else {
                break;
            }
        }
        S.push_back(P[i]);
    }
    return S;
}
bool is_ok(vector<point> S) {
    point p1 = *(S.end()-1);
    S.pop_back();
    point p2 = *(S.end()-1);
    S.pop_back();
    bool turn_flag = false;
    while(!S.empty()) {
        point p3 = *(S.end()-1);
        S.pop_back();
        if(is_zero(cross(p2-p1, p3-p2))) {
            if(!turn_flag) {
                turn_flag = true;
            }
            p1=p2; p2=p3;
        }
        else {
            if(turn_flag) {
                turn_flag = false;
                p1=p2; p2=p3;
            }
            else {
                return false;
            }
        }
    }
    return true;
}

int main() {
    int t; cin>>t;
    for(int i=0; i<t; ++i) {
        cin>>N;
        memset(visited, 0, sizeof(visited));
        for(int j=0; j<N; ++j) {
            cin>>P[j].x>>P[j].y;
        }
        if(N<6) {
            printf("NO\n");
            continue;
        }
        sort(P, P+N);
        vector<point> S = Graham();
        if(is_ok(S)) {
            printf("YES\n");
        }
        else {
            printf("NO\n");
        }
    }
    //system("pause");
    return 0;
}
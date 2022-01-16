#include<iostream>
#include<cstdlib>

using namespace std;
const int MAX = 20000;
int a[MAX+10];
int c[MAX+10];
int N;

int lowbit(int x) {
    return (x&(-x));
}

void construct() {
    int S[MAX+10] = {0};
    S[0] = 0;
    for(int i=1; i<=N; ++i) {
        S[i] = S[i-1]+a[i];
        c[i] = S[i] - S[i-lowbit(i)];
    }
}

void update(int i, int x) {
    int t = a[i];
    a[i] = x;
    int n1 = i;
    while(n1 <= N) {
        c[n1] += (x-t);
        n1 += lowbit(n1);
    }
}

int Sum(int e) {
    int nm = e;
    int s = 0;
    while(nm>0) {
        s += c[nm];
        nm -= lowbit(nm);
    }
    return s;
}

int main() {


    system("pause");
    return 0;
}
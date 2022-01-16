#include<iostream>
#include<cstdlib>
#include<vector>
#include<memory.h>
#define Max(x,y) (x>y)?(x):(y)

using namespace std;
const int MAX = 300000;
int a[MAX+10];
// int n[MAX+10];
int pos[MAX+10];
int c[MAX+10];
int lis[MAX+10];
int N;

void merge_sort(int left, int right) {
    if(right - left == 1) {
        pos[left] = left;
        return;
    }
    else if (right - left == 2) {
        int i=left, j = left+1;
        if(a[i] >= a[j]) {
            pos[i] = j, pos[j] = i;
        }
        else if(a[i] < a[j]) {
            pos[i] = i, pos[j] = j;
        }
        // else {
        //     n[i] = a[i], n[j] = a[j];
        //     pos[i] = j, pos[j] = i;
        // }
        return;
    }
    int mid = (left+right)/2;
    merge_sort(left, mid);
    merge_sort(mid, right);
    int *tp = new int [right-left];
    int tl = left, tr = mid, k = 0;
    while(tl<mid || tr<right) {
        if(tl == mid) {
            tp[k++] = pos[tr];
            ++tr;
            continue;
        }
        else if(tr == right) {
            tp[k++] = pos[tl];
            ++tl;
            continue;
        }
        if(a[pos[tl]] > a[pos[tr]]) {
            tp[k++] = pos[tr];
            ++tr;
        }
        else if(a[pos[tl]] < a[pos[tr]]) {
            tp[k++] = pos[tl];
            ++tl;
        }
        else {
            if(pos[tl] > pos[tr]) {
                tp[k++] = pos[tl];
                ++tl;
            }
            else {
                tp[k++] = pos[tr];
                ++tr;
            }
        }
    }
    memcpy(pos+left, tp, sizeof(int)*(right-left));
}

int lowbit(int x) {
    return (x&(-x));
}
int query(int k) {
    int m=0;
    while(k > 0) {
        m = Max(c[k],m);
        k -= lowbit(k);
    }
    return m;
}
void update(int i, int x) {
    while(i<=N) {
        c[i] = Max(c[i], x);
        i += lowbit(i);
    }
}

int main() {
    scanf("%d", &N);
    for(int i=1; i<=N; ++i) {
        scanf("%d", a+i);
    }
    merge_sort(1, N+1);
    for(int i=1; i<=N; ++i) {
        lis[pos[i]] = query(pos[i]) +1;
        update(pos[i], lis[pos[i]]);
    }
    int M = 0;
    for(int i=1; i<=N; ++i) {
        M = Max(c[i], M);
    }
    printf("%d\n", M);
    //system("pause");
    return 0;
}
#include<iostream>
#include<cmath>
#include<cstdlib>

using namespace std;

const int MAX = 20000;
int parent[MAX+1];
int total[MAX+1];

void init() {
    for(int i = 0; i<=MAX; ++i) {
        parent[i] = i;
        total[i] = 1;
    }
}
int get_root(int a) {
    if (parent[a] != a)
        parent[a] = get_root(parent[a]);
    return parent[a];
}
void merge(int a, int b) {
    parent[get_root(b)] = get_root(a);
}

int main() { 
    
    system("pause");
    return 0;
}

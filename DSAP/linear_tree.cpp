#include<iostream>
#include<cstdlib>

using namespace std;
const int MAX = 10;
struct CNode{
    int L, R;
    int mes;
    // int adds;
    int Mid() {
        return (L+R)/2;
    }
    // CNode *p_left, *p_right;
} tree[4*MAX+1];

void bulid_tree(int root, int L, int R) {
    tree[root].L = L;
    tree[root].R = R;
    tree[root].mes = 0;//TODO
    if(L != R) {
        bulid_tree(2*root+1, L, (L+R)/2);
        bulid_tree(2*root+2, (L+R)/2+1, R);
    }
}

// 区间分解模板，具体问题需具体填充
void section_divide(int v, int L, int R) {
    // assert tree[v].L <= L <= R <= tree[v].R 
    if(tree[v].L == L && tree[v].R == R) {
        //终止节点处理
    }
    else {
        int mid = tree[v].Mid();
        if(R <= mid) {
            section_divide(2*v+1, L, R);
        }
        else if(L > mid) {
            section_divide(2*v+2, L, R);
        }
        else {
            section_divide(2*v+1, L, mid);
            section_divide(2*v+2, mid+1, R);
        }
    }
}

int main() {
    
    system("pause");
    return 0;
}
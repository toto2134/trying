#include<iostream>
#include<algorithm>

using namespace std;
struct info{
    int n;
};
bool operator<(const info &a, const info &b) {
    return a.n < b.n;
}
bool operator!=(const info &a, const info &b) {
    return a<b || b<a;
}

class RBnode {
    info data;
    RBnode *left, *right;
    RBnode *parent;//maybe useless?
    int rank;   // use for debug
    bool color; // 0 black, 1 red
public:
    RBnode(): color(1), left(nullptr), right(nullptr), parent(nullptr) {}
    void turn_color() {
        color = !color;
    }
    friend class RBtree;
};

class RBtree {
    RBnode *root;
    bool red_tag = 0; //用于调整的对象内全局变量
    bool dblack_tag = 0;
public:
    RBtree(): root(nullptr) {}
    RBnode *insert(info &a, RBnode *r);
    RBnode *rotate_RR(RBnode *r, RBnode *c, RBnode *gs);
    RBnode *rotate_RL(RBnode *r, RBnode *c, RBnode *gs);
    RBnode *rotate_LR(RBnode *r, RBnode *c, RBnode *gs);
    RBnode *rotate_LL(RBnode *r, RBnode *c, RBnode *gs);
    RBnode *parent(RBnode *cur);// indirection
    RBnode *find_info(info &a);
    RBnode *find_lower_upper(RBnode *r);
    RBnode *del_node(info &a) { return del_node(find_info(a));}
    RBnode *del_node(RBnode *t);
};

RBnode *RBtree::insert(info &a, RBnode *r) {
    if(r == nullptr) {
        RBnode *t = new RBnode;
        t->data = a;
        return t;
    }
    RBnode *cur_child = nullptr;
    if(a < r->data) {
        r->left = insert(a, r->left);
        cur_child = r->left;
    }
    else if (r->data < a) {
        r->right = insert(a, r->right);
        cur_child = r->right;
    }
    if(cur_child == nullptr) {
        return nullptr;   //error
    }
    if(r->color && cur_child->color) { 
        //连续红节点标记，退回上一层递归处理
        red_tag = 1;
        return r;
    }
    else if(red_tag) {
        if(cur_child == r->left) { //红黑旋转
            if(cur_child->left->color)
                r = rotate_LL(r, cur_child, cur_child->left);
            else 
                r = rotate_LR(r, cur_child, cur_child->right);
        }
        else {
            if(cur_child->left->color)
                r = rotate_RL(r, cur_child, cur_child->left);
            else 
                r = rotate_RR(r, cur_child, cur_child->right);
        }
        if(r->left->color && r->right->color) {//叔祖换色
            r->left->turn_color();
            r->right->turn_color();
            r->turn_color();
        }
        red_tag = 0;
        return r;
    }
}

RBnode *RBtree::del_node(RBnode *t) {
    if(t == nullptr) { //没有寻找到
        return nullptr;
    }
    if(t->left && t->right) { //两侧节点存在
        RBnode *p = find_lower_upper(t);
        t->data = p->data; //换值不换色
        del_node(p);
    }
    else if(!t->left && !t->right) {
        if(t->color) { //红色节点
            delete t;
            return nullptr;
        }
        //TODO:双黑调整
    }
    else { //有一个叶子节点，继承颜色上移
        if(t->left) {
            t->left->turn_color();
            RBnode *p = t->left;
            delete t;
            return p;
        }
        else if(t->right) {
            t->right->turn_color();
            RBnode *p = t->right;
            delete t;
            return p;
        }
    }
}

RBnode *RBtree::find_info(info &a) {
    RBnode *t = root;
    while(t != nullptr && t->data != a) {
        if(t->data < a)
            t = t->left;
        else
            t = t->right;
    }
    return t;
}
RBnode *RBtree::find_lower_upper(RBnode *r) {
    r = r->left;
    while(r->right != nullptr) {
        r = r->right;
    }
    return r;
}
// 右旋
RBnode *RBtree::rotate_RR(RBnode *r, RBnode *c, RBnode *gs) {
    r->right = c->left;
    c->left = r;
    //换色
    c->color = 0;
    r->color = 1; gs->color = 1;
    return c;
}
// 左旋
RBnode *RBtree::rotate_LL(RBnode *r, RBnode *c, RBnode *gs) {
    r->left = c->right;
    c->right = r;
    //换色
    c->color = 0;
    r->color = 1; gs->color = 1;
    return c;
}
// 提升操作
RBnode *RBtree::rotate_RL(RBnode *r, RBnode *c, RBnode *gs) {
    // r->right = rotate_LL(c, gs, gs->left);
    // return rotate_RR(c, c->right, c->right->right);
    c->left = gs->right;
    r->right = gs->left;
    gs->left = r;
    gs->right = c;
    //换色
    gs->color = 0;
    r->color = 1; c->color = 1;
    return gs;
}
RBnode *RBtree::rotate_LR(RBnode *r, RBnode *c, RBnode *gs) {
    // r->left = rotate_RR(c, gs, gs->right);
    // return rotate_LL(c, c->left, c->left->left);
    c->right = gs->left;
    r->left = gs->right;
    gs->right = r;
    gs->left = c;
    //换色
    gs->color = 0;
    r->color = 1; c->color = 1;
    return gs;
}

int main() {
    return 0;
}

//(L1:(L2:(a,L1)),Lx:(L2 ,L3:(b)),Ly:(L3, c),L4:(d, L4))
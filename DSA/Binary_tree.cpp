/*
 * --n!种排列中，只有C(n)(卡特兰数)种不同的树
 * 
 * 最佳BST -- 基于用户访问习惯 降低ASL
 * 平衡BST -- 基于树高平衡约束 AVL树
 * 伸展树  -- 基于用户动态访问特征 统计意义上的最优
 */
#include<iostream>
#include<algorithm>
#include<memory.h>

using namespace std;
struct info {
    int n;
};

class best_BST_node {
    info data;
    int type; // 1 internal 0 external
    int cnt;  // count the frequency
    best_BST_node *left, *right; //best_BST *parent;
public:
    
};

class best_BST_tree{
    ;
};

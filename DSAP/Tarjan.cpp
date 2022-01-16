#include<iostream>
#include<cstdio>
#include<stack>
#include<vector>
#include<algorithm>

using namespace std;
int adj[1000][1000];//邻接矩阵
int dfn[1000];
bool visited[1000];
int low[1000];// 初始为INF
int index; //开始时间
int N,M;
stack<int> nodes; //
int col[1000], color_cnt=1;//最终节点分类情况，染色状况

void Tarjan(int u) {
    dfn[u]=low[u]=++index;
    nodes.push(u);
    visited[u] = true;
    for(int v=0; v<1000; ++v) {
        if(col[v] != 0) continue; //防止改变已经探索过的图
        if(adj[u][v] == 0) continue;
        if(!visited[v]) {
            Tarjan(v);
            low[u] = min(low[u], low[v]);
        }
        else {
            low[u] = min(low[u],dfn[v]);
        }
    }
    if(dfn[u] == low[u]) {// 强连通分量的根
        int t = nodes.top();
        nodes.pop();
        col[t] = color_cnt;
        while(t!= u) {
            //输出或记录强连通分量
            t = nodes.top();
            nodes.pop();
            col[t] = color_cnt;
        }
        color_cnt += 1;
    }
    visited[u] = false;
}

int main() {
    
    for(int i=1; i<=N; ++i) {
        if(dfn[i] == 0) { 
            Tarjan(i);
        }
    }
    system("pause");
    return 0;
}
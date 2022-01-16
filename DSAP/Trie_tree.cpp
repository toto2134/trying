#include<iostream>
#include<cstdlib>
#include<algorithm>
#include<cstring>
#include<memory.h>
#include<queue>

using namespace std;
const int MAXN = 1010;
char str[MAXN];
const int LETTERS = 26;
int nNodesCount = 0;
struct CNode {
    CNode *pChilds[LETTERS];
    CNode *pPrev;
    bool bBadNode;
    CNode() {
        memset(pChilds, 0, sizeof(pChilds));
        bBadNode = false;
        pPrev = nullptr;
    }
} Tree[20000];

void Insert(CNode *pRoot, char * s) {
    for(int i=0; s[i]; ++i) {
        if(pRoot->pChilds[s[i]-'a'] == nullptr) {
            pRoot->pChilds[s[i]-'a'] = Tree + nNodesCount;
            ++nNodesCount;
        }
        pRoot = pRoot->pChilds[s[i]-'a'];
    }
    pRoot->bBadNode = true;
}

void BuildDfa() {
    for(int i=0; i<LETTERS; ++i) {
        Tree[0].pChilds[i] = Tree+1;
    }
    Tree[0].pPrev = nullptr;
    Tree[1].pPrev = Tree;
    queue<CNode*> Q;
    Q.push(Tree+1);
    while(!Q.empty()) {
        CNode *cur = Q.front();
        Q.pop();
        for(int i=0; i<LETTERS; ++i) {
            CNode *tp = cur->pChilds[i];
            if(!tp)
                continue;
            CNode *pPrev = cur->pPrev;
            while(pPrev->pChilds[i] == NULL)
                pPrev = pPrev->pPrev;
            tp->pPrev = pPrev->pChilds[i];
            if(tp->pPrev->bBadNode)
                tp->bBadNode = true;
            Q.push(tp);
        }
    }
}

bool searchDfa(char *s) {
    CNode *p = Tree+1;
    for(int i=0; s[i]; ++i) {
        while(p->pChilds[s[i]-'a'] == nullptr)
            p = p->pPrev;
        p = p->pChilds[s[i]-'a'];
        if(p->bBadNode)
            return true;
    }
    return false;
}

int main() {
    int n, m;
    nNodesCount = 2;
    scanf("%d", &n);
    for(int i=0; i<n; ++i) {
        scanf("%s", str);
        Insert(Tree+1, str);
    }
    BuildDfa();
    scanf("%d", &m);
    for(int i=0; i<m; ++i) {
        scanf("%s", str);
        if(searchDfa(str)) 
            printf("YES\n");
        else 
            printf("NO\n");
    }
    //system("pause");
    return 0;
}

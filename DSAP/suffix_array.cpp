#include<iostream>
#include<cstring>
#include<vector>
#include<cmath>
#include<algorithm>

using namespace std;

const int MAXN=1001;
int wa[MAXN], wb[MAXN], wv[MAXN], Ws[MAXN];

int sa[MAXN];
void build_sa(const char *s, int sa[], int n, int m){
    // n，字符串s长度
    // m，初始为字符种类数，后来为不同j-后缀数目
    int i,j,p, *pm=wa, *k2sa=wb, *t;
    for(i=0;i<m; ++i) Ws[i]=0;
    for(i=0;i<n; ++i) Ws[pm[i]=s[i]]++;
    //Ws[i] 是字符i出现的次数，pm是原字符串的复制品
    //也可以看作字符的排名
    for(i=1;i<m; ++i) Ws[i]+=Ws[i-1];
    //Ws[i]变为不大于字符i的字符出现的次数
    for(i=n-1;i>=0; --i) sa[--Ws[pm[i]]] = i;
    //sa[i]为位置i的字符pm[i]的名次
    for(j=p=1; p<n; j<<=1, m=p) {
        //在一次循环中，已知j-后缀，求2j-后缀过程
        //此时,sa[i]是名次i的j-后缀的位置，pm[i]是位置i的j-后缀的排名

        //k2sa[i]是所有2j=后缀按照第二个关键字排序后，名次为i的2j-后缀的位置
        //从n-j开始的2j-后缀第二关键字为NULL，因此排名为靠前的记录后面位置
        for(p=0,i=n-j;i<n;++i) k2sa[p++]=i;
        //对于之后的排名要根据sa[i]来进行判断
        for(i=0;i<n;++i) if(sa[i]>=j) k2sa[p++]=sa[i]-j;

        for(i=0;i<m;++i) Ws[i]=0;
        //第二关键字名次便利2j-后缀的第一关键字
        for(i=0;i<n;++i) Ws[wv[i]=pm[k2sa[i]]]++;
        //循环结束后Ws[i]表示排名为i的j-后缀的个数
        for(i=1;i<m;++i) Ws[i]+=Ws[i-1];
        for(i=n-1;i>=0;--i) sa[--Ws[wv[i]]]=k2sa[i];
        //以上类似初始化中计算收集操作

        for(t=pm,pm=k2sa,k2sa=t,pm[sa[0]]=0,p=i=1;i<n;++i) {
            //将pm[i]变为位置为i的2j-后缀的排名，从0开始
            int a=sa[i-1], b=sa[i];
            if(k2sa[a]==k2sa[b]&&a+j<n&&b+j<n&&k2sa[a+j]==k2sa[b+j]) {
                //看名次为i的2j-后缀是不是新的，即和名次为i-1的2j-后缀是否一样
                pm[sa[i]] = p-1;
            }
            else {
                pm[sa[i]] = p++;
            }
        }
    }
    return;
}

int height[MAXN],Rank[MAXN];
void build_height(char *str, int n, int *sa, int *rank) {
    int i,j,k;
    for(int i=0; i<n; ++i) 
        rank[sa[i]] = i;
    height[0] = 0;
    for(i=k=0; i<n-1; height[rank[i++]] = k)
        for(k?k--:0,j=sa[rank[i]-1]; str[i+k]==str[j+k]; k++) ;
    
}

// RMQ
int dp[MAXN][20];
void build_dp(int *a, int n) {
    for(int i=0; i<n; ++i)
        dp[i][0] = a[i];
    for(int j=1; (1<<j) < n; ++j) {
        for(int i=0; i+(1<<(j-1))<n; ++i) {
            dp[i][j]=min(dp[i][j-1], dp[i+(1<<(j-1))][j-1]);
        }
    }
}
int search(int i, int j) {
    int k=31;
    while((1<<k)>j-i) {
        k--;
    }
    return min(dp[i][k], dp[j-(1<<k)][k]);
}
int main() {
    // e.g.
    build_sa("abcd", sa, 5, 130);
    build_height("abcd", 5, sa, Rank);
    build_dp(height, 5);

}
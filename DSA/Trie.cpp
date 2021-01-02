/*
 * 
 * 
 */
#include<iostream>
#include<algorithm>
#include<memory.h>

using namespace std;

class Trie_tree {
    bool character[26];
    bool type;  // 0 internal, 1 leaf
    int cnt;    // frequency of words
    Trie_tree *child[26];
public:
    Trie_tree():type(1), cnt(0) {
        memset(character, 0, sizeof(character));
        memset(child, 0, sizeof(child));
    }
};

#include <stdlib.h>
#include <string.h>
#include "../../include/string_utils.h"

int levenshteinDistance(const char* s1, const char* s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    int* prev = (int*)malloc((len2 + 1) * sizeof(int));
    int* curr = (int*)malloc((len2 + 1) * sizeof(int));
    if (!prev || !curr) {
        free(prev);
        free(curr);
        return 0;
    }
    for (size_t j = 0; j <= len2; j++) prev[j] = (int)j;
    for (size_t i = 1; i <= len1; i++) {
        curr[0] = (int)i;
        for (size_t j = 1; j <= len2; j++) {
            int cost = s1[i - 1] == s2[j - 1] ? 0 : 1;
            int del = prev[j] + 1;
            int ins = curr[j - 1] + 1;
            int sub = prev[j - 1] + cost;
            int min = del < ins ? del : ins;
            if (sub < min) min = sub;
            curr[j] = min;
        }
        int* tmp = prev;
        prev = curr;
        curr = tmp;
    }
    int dist = prev[len2];
    free(prev);
    free(curr);
    return dist;
}

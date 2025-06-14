#include "../../include/builtin_stdlib.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

const EmbeddedModule embeddedStdlib[] = {
    {"std/math.orus", "pub const PI: f64 = 3.141592653589793\npub const E: f64 = 2.718281828459045\npub const TAU: f64 = 6.283185307179586\n\npub fn abs(x: f64) -> f64 {\n    if x < 0.0 {\n        return -x\n    }\n    return x\n}\n\npub fn clamp(x: f64, min_val: f64, max_val: f64) -> f64 {\n    if x < min_val {\n        return min_val\n    }\n    if x > max_val {\n        return max_val\n    }\n    return x\n}\n\npub fn pow(base: f64, exponent: i32) -> f64 {\n    let mut b = base\n    let mut e = exponent\n    let mut result: f64 = 1.0\n    if e < 0 {\n        b = 1.0 / b\n        e = -e\n    }\n    while e > 0 {\n        if (e & 1) == 1 {\n            result = result * b\n        }\n        e = e >> 1\n        b = b * b\n    }\n    return result\n}\n\npub fn sqrt(x: f64) -> f64 {\n    if x <= 0.0 {\n        return 0.0\n    }\n    let mut guess = x / 2.0\n    let mut last = 0.0\n    while abs(guess - last) > 0.0000001 {\n        last = guess\n        guess = (guess + x / guess) / 2.0\n    }\n    return guess\n}\n\npub fn floor(x: f64) -> i32 {\n    let i: i32 = x as i32\n    if x < 0.0 and (x != (i as f64)) {\n        return i - 1\n    }\n    return i\n}\n\npub fn ceil(x: f64) -> i32 {\n    let i: i32 = x as i32\n    if x > 0.0 and (x != (i as f64)) {\n        return i + 1\n    }\n    return i\n}\n\npub fn round(x: f64) -> i32 {\n    return floor(x + 0.5)\n}\n\npub fn sign(x: f64) -> i32 {\n    if x > 0.0 {\n        return 1\n    }\n    if x < 0.0 {\n        return -1\n    }\n    return 0\n}\n\npub fn average(values: [f64]) -> f64 {\n    if len(values) == 0 {\n        return 0.0\n    }\n    return sum(values) / (len(values) as f64)\n}\n\npub fn median(values: [f64]) -> f64 {\n    if len(values) == 0 {\n        return 0.0\n    }\n    let sortedVals = sorted(values)\n    let mid = len(sortedVals) / 2\n    if (len(sortedVals) & 1) == 1 {\n        return sortedVals[mid]\n    }\n    return (sortedVals[mid - 1] + sortedVals[mid]) / 2.0\n}\n\npub fn mod(a: i32, b: i32) -> i32 {\n    let r = a % b\n    if r < 0 {\n        return r + b\n    }\n    return r\n}\n"},
    {"std/random.orus", "static mut STATE: i64 = 1\n\nfn next_i64() -> i64 {\n    // ANSI C LCG parameters\n    let a: i64 = 1103515245\n    let c: i64 = 12345\n    let m: i64 = 2147483648\n    STATE = (STATE * a + c) % m\n    return STATE\n}\n\npub fn rand() -> f64 {\n    return (next_i64() as f64) / 2147483648.0\n}\n\npub fn rand_int(min: i32, max: i32) -> i32 {\n    let range: i64 = (max - min + 1) as i64\n    return min + (next_i64() % range) as i32\n}\n\npub fn choice<T>(items: [T]) -> T {\n    let idx: i32 = rand_int(0, len(items) - 1)\n    return items[idx]\n}\n\npub fn shuffle<T>(items: [T]) {\n    let mut i: i32 = len(items) - 1\n    while i > 0 {\n        let j: i32 = rand_int(0, i)\n        let temp = items[i]\n        items[i] = items[j]\n        items[j] = temp\n        i = i - 1\n    }\n}\n"},
};
const int embeddedStdlibCount = sizeof(embeddedStdlib)/sizeof(EmbeddedModule);

const char* getEmbeddedModule(const char* name){
    for(int i=0;i<embeddedStdlibCount;i++){
        if(strcmp(embeddedStdlib[i].name,name)==0) return embeddedStdlib[i].source;
    }
    return NULL;
}

static void ensure_dir(const char* path){
    char tmp[512];
    strncpy(tmp,path,sizeof(tmp)-1);
    tmp[sizeof(tmp)-1]=0;
    for(char* p=tmp+1; *p; p++){ if(*p=="/"){ *p=0; mkdir(tmp,0755); *p="/"; } }
    mkdir(tmp,0755);
}

void dumpEmbeddedStdlib(const char* dir){
    char full[512];
    for(int i=0;i<embeddedStdlibCount;i++){
        snprintf(full,sizeof(full),"%s/%s",dir,embeddedStdlib[i].name);
        char* slash=strrchr(full,'/');
        if(slash){ *slash=0; ensure_dir(full); *slash='/'; } else { ensure_dir(full); }
        FILE* f=fopen(full,"w"); if(f){ fputs(embeddedStdlib[i].source,f); fclose(f); }
    }
}

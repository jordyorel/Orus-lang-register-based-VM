import os, sys

def escape_c(s):
    return s.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\n')

if len(sys.argv) != 4:
    print('Usage: gen_stdlib.py <src_dir> <out_c> <out_h>')
    sys.exit(1)

src_dir, out_c, out_h = sys.argv[1:4]
modules = []
for root, _, files in os.walk(src_dir):
    for f in files:
        if f.endswith('.orus'):
            path = os.path.join(root, f)
            rel = os.path.relpath(path, src_dir)
            with open(path, 'r') as fh:
                code = fh.read()
            modules.append((os.path.join('std', rel).replace('\\', '/'), code))

with open(out_h, 'w') as h:
    h.write('#ifndef BUILTIN_STDLIB_H\n#define BUILTIN_STDLIB_H\n\n')
    h.write('typedef struct {\n    const char* name;\n    const char* source;\n} EmbeddedModule;\n\n')
    h.write('extern const EmbeddedModule embeddedStdlib[];\n')
    h.write('extern const int embeddedStdlibCount;\n')
    h.write('const char* getEmbeddedModule(const char* name);\n')
    h.write('void dumpEmbeddedStdlib(const char* dir);\n')
    h.write('\n#endif\n')

with open(out_c, 'w') as c:
    c.write('#include "../../include/builtin_stdlib.h"\n')
    c.write('#include <string.h>\n#include <stdio.h>\n#include <sys/stat.h>\n')
    c.write('\nconst EmbeddedModule embeddedStdlib[] = {\n')
    for name, code in modules:
        c.write('    {"%s", "%s"},\n' % (name, escape_c(code)))
    c.write('};\n')
    c.write('const int embeddedStdlibCount = sizeof(embeddedStdlib)/sizeof(EmbeddedModule);\n')
    c.write('\nconst char* getEmbeddedModule(const char* name){\n')
    c.write('    for(int i=0;i<embeddedStdlibCount;i++){\n')
    c.write('        if(strcmp(embeddedStdlib[i].name,name)==0) return embeddedStdlib[i].source;\n')
    c.write('    }\n    return NULL;\n}\n')
    c.write('\nstatic void ensure_dir(const char* path){\n')
    c.write('    char tmp[512];\n    strncpy(tmp,path,sizeof(tmp)-1);\n    tmp[sizeof(tmp)-1]=0;\n')
    c.write('    for(char* p=tmp+1; *p; p++){ if(*p==\'/\'){ *p=0; mkdir(tmp,0755); *p=\'/\'; } }\n')
    c.write('    mkdir(tmp,0755);\n}\n')
    c.write('\nvoid dumpEmbeddedStdlib(const char* dir){\n    char full[512];\n    for(int i=0;i<embeddedStdlibCount;i++){\n')
    c.write('        snprintf(full,sizeof(full),"%s/%s",dir,embeddedStdlib[i].name);\n')
    c.write('        char* slash=strrchr(full,\'/\');\n        if(slash){ *slash=0; ensure_dir(full); *slash=\'/\'; } else { ensure_dir(full); }\n')
    c.write('        FILE* f=fopen(full,"w"); if(f){ fputs(embeddedStdlib[i].source,f); fclose(f); }\n')
    c.write('    }\n}\n')

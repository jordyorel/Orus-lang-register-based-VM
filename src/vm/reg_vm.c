#include "../../include/reg_vm.h"
#include <string.h>

void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk) {
    vm->chunk = chunk;
    vm->ip = chunk->code;
    memset(vm->registers, 0, sizeof(vm->registers));
}

void freeRegisterVM(RegisterVM* vm) {
    (void)vm;
}

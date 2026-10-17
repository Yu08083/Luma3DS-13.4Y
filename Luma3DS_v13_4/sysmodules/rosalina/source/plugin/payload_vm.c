#include "plugin/payload_vm.h"
#include <string.h>

#define VM_DATA 0x10000000u
#define VM_ARGS 0x20000000u
#define VM_STACK 0x30000000u
#define VM_CODE 0x40000000u
#define VM_RETURN 0x50000000u

typedef struct {
    uint32_t registers[16];
    uint8_t stack[256];
    uint8_t *buffer;
    uint32_t size;
    const PluginPayload *program;
    const uint32_t *arguments;
    bool n, z, c, v;
} PayloadVm;

static uint32_t vmRotate(uint32_t value, unsigned shift)
{
    shift &= 31;
    return shift ? (value >> shift) | (value << (32 - shift)) : value;
}

static bool vmCondition(const PayloadVm *vm, unsigned condition)
{
    switch(condition)
    {
        case 0: return vm->z;
        case 1: return !vm->z;
        case 2: return vm->c;
        case 3: return !vm->c;
        case 4: return vm->n;
        case 5: return !vm->n;
        case 6: return vm->v;
        case 7: return !vm->v;
        case 8: return vm->c && !vm->z;
        case 9: return !vm->c || vm->z;
        case 10: return vm->n == vm->v;
        case 11: return vm->n != vm->v;
        case 12: return !vm->z && vm->n == vm->v;
        case 13: return vm->z || vm->n != vm->v;
        case 14: return true;
        default: return false;
    }
}

static uint32_t vmRegister(const PayloadVm *vm, unsigned index)
{
    return vm->registers[index] + (index == 15 ? 8 : 0);
}

static bool vmSpan(uint32_t address, uint32_t base, uint32_t length, uint32_t size)
{
    return address >= base && size <= length && address - base <= length - size;
}

static bool vmMemory(PayloadVm *vm, uint32_t address, uint32_t *value, unsigned size, bool write)
{
    uint8_t *memory = NULL;
    if(size == 4 && (address & 3)) return false;
    if(vmSpan(address, VM_DATA, vm->size, size)) memory = vm->buffer + address - VM_DATA;
    else if(vmSpan(address, VM_STACK, sizeof(vm->stack), size)) memory = vm->stack + address - VM_STACK;
    else if(!write && vmSpan(address, VM_ARGS, 16, size)) memory = (uint8_t *)vm->arguments + address - VM_ARGS;
    else if(!write && vmSpan(address, VM_CODE, vm->program->count * 4, size)) memory = (uint8_t *)vm->program->code + address - VM_CODE;
    if(!memory) return false;
    if(write)
    {
        for(unsigned i = 0; i < size; ++i) memory[i] = (uint8_t)(*value >> (i * 8));
    }
    else
    {
        *value = 0;
        for(unsigned i = 0; i < size; ++i) *value |= (uint32_t)memory[i] << (i * 8);
    }
    return true;
}

static uint32_t vmShift(uint32_t value, unsigned type, unsigned amount, bool immediate, bool *carry)
{
    if(!amount && !immediate) return value;
    if(type == 0)
    {
        if(!amount) return value;
        *carry = amount <= 32 ? (value >> (32 - amount)) & 1 : false;
        return amount < 32 ? value << amount : 0;
    }
    if(type == 1)
    {
        if(!amount) amount = 32;
        *carry = amount <= 32 ? (value >> (amount - 1)) & 1 : false;
        return amount < 32 ? value >> amount : 0;
    }
    if(type == 2)
    {
        if(!amount) amount = 32;
        *carry = amount < 32 ? (value >> (amount - 1)) & 1 : value >> 31;
        return amount < 32 ? (uint32_t)((int32_t)value >> amount) : (uint32_t)-(int32_t)(value >> 31);
    }
    if(!amount)
    {
        bool old = *carry;
        *carry = value & 1;
        return (value >> 1) | ((uint32_t)old << 31);
    }
    uint32_t rotated = vmRotate(value, amount);
    *carry = rotated >> 31;
    return rotated;
}

static bool vmOperand(const PayloadVm *vm, uint32_t instruction, bool immediate, uint32_t *value, bool *carry)
{
    if(immediate)
    {
        unsigned rotation = ((instruction >> 8) & 15) * 2;
        *value = vmRotate(instruction & 255, rotation);
        if(rotation) *carry = *value >> 31;
        return true;
    }
    if((instruction & 0x90) == 0x90) return false;
    unsigned amount = (instruction >> 7) & 31;
    bool registerShift = instruction & 16;
    if(registerShift) amount = vmRegister(vm, (instruction >> 8) & 15) & 255;
    *value = vmShift(vmRegister(vm, instruction & 15), (instruction >> 5) & 3, amount, !registerShift, carry);
    return true;
}

static uint32_t vmAdd(PayloadVm *vm, uint32_t a, uint32_t b, bool carry, bool set)
{
    uint64_t result = (uint64_t)a + b + carry;
    uint32_t value = (uint32_t)result;
    if(set)
    {
        vm->c = result >> 32;
        vm->v = (~(a ^ b) & (a ^ value)) >> 31;
    }
    return value;
}

bool PluginPayload_Set(PluginPayload *program, const uint32_t code[32])
{
    if(!program || !code) return false;
    memset(program, 0, sizeof(*program));
    for(unsigned i = 0; i < 32; ++i)
    {
        if(code[i] == 0xE320F000u)
        {
            program->count = i;
            return i != 0;
        }
        program->code[i] = code[i];
    }
    memset(program, 0, sizeof(*program));
    return false;
}

bool PluginPayload_Run(const PluginPayload *program, void *buffer, uint32_t size, const uint32_t arguments[4], uint32_t *checksum)
{
    if(!program || !program->count || program->count >= 32 || !buffer || !size || size > 16 * 1024 * 1024 || !arguments || !checksum) return false;
    static const uint32_t sumProgram[] = {0xE92D40C0u, 0xE3A07000u, 0xE4906004u, 0xE0877006u, 0xE1500001u, 0x1AFFFFFBu, 0xE1A00007u, 0xE8BD80C0u};
    if(program->count == 8 && !(size & 3) && !memcmp(program->code, sumProgram, sizeof(sumProgram)))
    {
        const uint8_t *bytes = buffer;
        uint32_t sum = 0;
        for(uint32_t i = 0; i < size; i += 4)
            sum += (uint32_t)bytes[i] | (uint32_t)bytes[i + 1] << 8 | (uint32_t)bytes[i + 2] << 16 | (uint32_t)bytes[i + 3] << 24;
        *checksum = sum;
        return true;
    }
    PayloadVm vm = { .buffer = buffer, .size = size, .program = program, .arguments = arguments };
    vm.registers[0] = VM_DATA;
    vm.registers[1] = VM_DATA + size;
    vm.registers[2] = VM_ARGS;
    vm.registers[13] = VM_STACK + sizeof(vm.stack);
    vm.registers[14] = VM_RETURN;
    vm.registers[15] = VM_CODE;
    uint64_t budget = ((uint64_t)size / 4 + 1) * 64 + 512;
    while(budget--)
    {
        uint32_t pc = vm.registers[15];
        if(pc == VM_RETURN) { *checksum = vm.registers[0]; return true; }
        if((pc & 3) || !vmSpan(pc, VM_CODE, program->count * 4, 4)) return false;
        uint32_t instruction = program->code[(pc - VM_CODE) / 4];
        uint32_t next = pc + 4;
        if(instruction >> 28 == 15) return false;
        if(!vmCondition(&vm, instruction >> 28)) { vm.registers[15] = next; continue; }
        unsigned rn = (instruction >> 16) & 15, rd = (instruction >> 12) & 15;
        bool set = instruction & (1u << 20);
        if((instruction & 0x0FFFFFF0u) == 0x012FFF10u)
        {
            next = vmRegister(&vm, instruction & 15);
        }
        else if((instruction & 0x0E000000u) == 0x0A000000u)
        {
            int32_t displacement = (int32_t)(instruction << 8) >> 6;
            if(instruction & (1u << 24)) vm.registers[14] = pc + 4;
            next = pc + 8 + displacement;
        }
        else if((instruction & 0x0FC000F0u) == 0x00000090u)
        {
            uint32_t value = vmRegister(&vm, instruction & 15) * vmRegister(&vm, (instruction >> 8) & 15);
            if(instruction & (1u << 21)) value += vmRegister(&vm, rd);
            if(rn == 15) return false;
            vm.registers[rn] = value;
            if(set) { vm.n = value >> 31; vm.z = value == 0; }
        }
        else if((instruction & 0x0E000000u) == 0x08000000u)
        {
            unsigned count = 0;
            for(unsigned i = 0; i < 16; ++i) if(instruction & (1u << i)) ++count;
            if(!count || rn == 15 || (instruction & (1u << 22))) return false;
            bool up = instruction & (1u << 23), before = instruction & (1u << 24);
            uint32_t base = vmRegister(&vm, rn);
            uint32_t address = up ? base + (before ? 4 : 0) : base - count * 4 + (before ? 0 : 4);
            for(unsigned i = 0; i < 16; ++i) if(instruction & (1u << i))
            {
                uint32_t value = vmRegister(&vm, i);
                if(!vmMemory(&vm, address, &value, 4, !set)) return false;
                if(set) { if(i == 15) next = value; else vm.registers[i] = value; }
                address += 4;
            }
            if(instruction & (1u << 21)) vm.registers[rn] = up ? base + count * 4 : base - count * 4;
        }
        else if((instruction & 0x0C000000u) == 0x04000000u)
        {
            bool pre = instruction & (1u << 24), up = instruction & (1u << 23);
            uint32_t offset = instruction & 0xFFF;
            if(instruction & (1u << 25))
            {
                bool carry = vm.c;
                if(instruction & 16 || !vmOperand(&vm, instruction, false, &offset, &carry)) return false;
            }
            uint32_t base = vmRegister(&vm, rn), adjusted = up ? base + offset : base - offset;
            uint32_t value = vmRegister(&vm, rd);
            if(!vmMemory(&vm, pre ? adjusted : base, &value, instruction & (1u << 22) ? 1 : 4, !set)) return false;
            if(set) { if(rd == 15) next = value; else vm.registers[rd] = value; }
            if(!pre || (instruction & (1u << 21)))
            {
                if(rn == 15) return false;
                vm.registers[rn] = adjusted;
            }
        }
        else if((instruction & 0x0C000000u) == 0)
        {
            uint32_t a = vmRegister(&vm, rn), b, value;
            bool carry = vm.c;
            if(!vmOperand(&vm, instruction, instruction & (1u << 25), &b, &carry)) return false;
            unsigned operation = (instruction >> 21) & 15;
            bool test = operation >= 8 && operation <= 11;
            if(test && !set) return false;
            bool arithmetic = (operation >= 2 && operation <= 7) || operation == 10 || operation == 11;
            switch(operation)
            {
                case 0: case 8: value = a & b; break;
                case 1: case 9: value = a ^ b; break;
                case 2: case 10: value = vmAdd(&vm, a, ~b, true, set); break;
                case 3: value = vmAdd(&vm, b, ~a, true, set); break;
                case 4: case 11: value = vmAdd(&vm, a, b, false, set); break;
                case 5: value = vmAdd(&vm, a, b, vm.c, set); break;
                case 6: value = vmAdd(&vm, a, ~b, vm.c, set); break;
                case 7: value = vmAdd(&vm, b, ~a, vm.c, set); break;
                case 12: value = a | b; break;
                case 13: value = b; break;
                case 14: value = a & ~b; break;
                default: value = ~b; break;
            }
            if(set)
            {
                if(rd == 15 && !test) return false;
                vm.n = value >> 31; vm.z = value == 0;
                if(!arithmetic) vm.c = carry;
            }
            if(!test) { if(rd == 15) next = value; else vm.registers[rd] = value; }
        }
        else return false;
        vm.registers[15] = next;
    }
    return false;
}

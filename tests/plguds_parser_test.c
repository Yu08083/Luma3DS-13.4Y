#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "plguds_ipc.h"

static bool range(void *context, uint32_t address, uint32_t size, bool write)
{
    (void)context;
    if (write && address >= 0x00300000) return false;
    return address >= 0x00100000 && address + size <= 0x00400000;
}

static bool handle(void *context, uint32_t value, bool sharedMemory)
{
    (void)context;
    return value == (sharedMemory ? 12u : 11u);
}

static uint32_t descriptor(uint32_t size, uint32_t id) { return (size << 14) | (id << 10) | 2; }

static void packet(uint32_t id, uint32_t command[64], uint32_t statics[32])
{
    memset(command, 0, 256);
    memset(statics, 0, 128);
    statics[0] = descriptor(0x1000, 0);
    statics[1] = 0x00200000;
    switch (id)
    {
        case 14: case 31:
            command[0] = PlgUdsHeader(id, 0, 6);
            command[1] = descriptor(0x108, 1); command[2] = 0x00100000;
            command[3] = descriptor(0xFE, 2); command[4] = 0x00101000;
            command[5] = descriptor(0xFE, 3); command[6] = 0x00102000;
            break;
        case 15:
            command[0] = PlgUdsHeader(id, 16, 4); command[1] = 0x1000;
            command[18] = 11; command[19] = (0x1000 << 4) | 12; command[20] = 0x00200000;
            break;
        case 16:
            command[0] = PlgUdsHeader(id, 1, 2); command[1] = 0xC8;
            command[2] = descriptor(0xC8, 4); command[3] = 0x00100000;
            break;
        case 17:
            command[0] = PlgUdsHeader(id, 1, 0); command[1] = 0xC8;
            break;
        case 20:
            command[0] = PlgUdsHeader(id, 3, 0); command[2] = 0x200; command[3] = 0x800;
            break;
        case 23:
            command[0] = PlgUdsHeader(id, 6, 2); command[4] = 0x200; command[5] = 0x800;
            command[7] = descriptor(0x800, 5); command[8] = 0x00100000;
            break;
        case 27:
            command[0] = PlgUdsHeader(id, 12, 2); command[1] = 0x1000; command[12] = 0x400; command[14] = 12;
            break;
        case 29: case 30:
        {
            uint32_t normal = id == 29 ? 1 : 2;
            command[0] = PlgUdsHeader(id, normal, 4); command[normal] = 8;
            command[normal + 1] = descriptor(0x108, 1); command[normal + 2] = 0x00100000;
            command[normal + 3] = descriptor(8, 0); command[normal + 4] = 0x00101000;
            break;
        }
    }
}

int main(void)
{
    PlgUdsValidation validation = {NULL, range, handle, 8};
    const uint32_t ids[] = {14, 15, 16, 17, 20, 23, 27, 29, 30, 31};
    const uint32_t server_headers[] = {0x1F01C0, 0x0F0500, 0x1000C0, 0x110080, 0x140100, 0x170200, 0x1B0380, 0x1D0140, 0x1E0180, 0x1F01C0};
    uint32_t storage[66], statics[34], original[64];
    uint32_t *command = storage + 1, *buffers = statics + 1;
    storage[0] = storage[65] = 0x98765432;
    statics[0] = statics[33] = 0x13572468;
    for (unsigned index = 0; index < sizeof(ids) / sizeof(ids[0]); index++)
    {
        uint32_t id = ids[index];
        packet(id, command, buffers);
        memcpy(original, command, sizeof(original));
        assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsForward);
        assert(command[0] == server_headers[index]);
        assert(!(command[0] & 63));
        for (uint32_t shape = 0; shape < 0x10000; shape++)
        {
            packet(id, command, buffers);
            command[0] = (id << 16) | shape;
            if (command[0] == original[0]) continue;
            memcpy(original + 1, command + 1, 63 * sizeof(uint32_t));
            assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
            assert(command[0] == ((id << 16) | shape));
            assert(!memcmp(original + 1, command + 1, 63 * sizeof(uint32_t)));
        }
        assert(storage[0] == 0x98765432 && storage[65] == 0x98765432);
        assert(statics[0] == 0x13572468 && statics[33] == 0x13572468);
    }
    packet(17, command, buffers); buffers[1] = 0x00300000;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(17, command, buffers); buffers[1] = 0xFFFFF000;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(23, command, buffers); command[4] = 0x40000000;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(20, command, buffers); command[2] = 0x40000000;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(27, command, buffers); command[13] = 0x10;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(27, command, buffers); command[14] = 0xFFFF8001;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(15, command, buffers); command[19] ^= 2;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    packet(31, command, buffers); command[3] = descriptor(1, 2);
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    memset(command, 0, 256); command[0] = PlgUdsHeader(100, 2, 0); command[1] = 8;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsInitialize);
    command[1] = 9;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    command[1] = 8; command[2] = 1;
    assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    for (uint32_t id = 104; id <= 105; id++)
    {
        command[0] = PlgUdsHeader(id, 1, 0); command[1] = 1;
        assert(PlgUdsPrepare(command, buffers, &validation) == (id == 104 ? PlgUdsSignalState : PlgUdsPause));
        command[1] = 2;
        assert(PlgUdsPrepare(command, buffers, &validation) == PlgUdsReject);
    }
    puts("PLGUDS: 655350 malformed IPC shapes, descriptor bounds, permissions and ownership passed");
}

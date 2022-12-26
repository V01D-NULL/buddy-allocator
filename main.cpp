#include <chrono>
#include <iostream>

#include "buddy.hpp"

int main(void) {
    constexpr auto max_order = 30;
    constexpr auto pages = ((1 << max_order) >> 12);  // Number of 4kib large pages in 1GiB
    constexpr auto memory_size = 1 << max_order;

    auto base = new uint64_t[1 << max_order];
    BuddyAllocator instance(base);

    std::cout << "Buddy test: Allocating " << pages << " pages which totals " << memory_size << " bytes.\n";
    std::cout << "Base: 0x" << std::hex << (size_t)base << '\n';

    auto start = std::chrono::high_resolution_clock::now();

    // Allocate 1GiB in 4kib chunks for testing purposes
    for (auto i = 0; i < pages; i++) {
        // TODO: Order 1 returns the size of order 0 for some reason... every other order is fine... look into that
        auto ptr = instance.alloc(13);
        if (!ptr) {
            // std::cout << "bad ptr ig\n";
            break;
        }
        // auto ptr = instance.alloc(4096);
        // std::cout << "#" << std::dec << i << ": " << std::hex << ptr << '\n';
    }

    // Expectedly fails. Allocator is OOM.
    // std::cout << "Trying again..." << instance.alloc(9) << '\n';

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    std::chrono::duration<double> sec = elapsed;

    std::cout << "Allocation of " << std::dec << pages << " 4KiB large pages took "
              << usec.count() << " usec (" << sec.count() << " seconds)" << std::endl;

    delete[] base;
}

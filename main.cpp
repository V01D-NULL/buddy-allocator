#include <chrono>
#include <iostream>
#include <vector>

#include "buddy.hpp"

#define MiB(o) (1 << 20) * o

class BuddyZone {
    using RedBlackTree = frg::rbtree<BuddyAllocator, &BuddyAllocator::tree_hook, BuddyAllocator::traversal>;
    static constexpr int AllocationPathThreshold = MiB(16);  // Allocators below 16MiB are slow_path, those above are fast_path

public:
    void init(AddressType base) {
        uint64_t _base = (uint64_t)base;

        for (int i = 0; i < 32; i++, _base += ((1 << 12) * 4096))
            fast_path.insert(new BuddyAllocator{ AddressType(_base) });
    }

    AllocationResult allocate(Order order) {
        return fast_path.first()->alloc(order);
    }

    void deallocate() {
        // TODO: How to find the corresponding buddy?
        // could probably just check the address...
        // fast_path.
    }

private:
    RedBlackTree slow_path;
    RedBlackTree fast_path;
};


BuddyZone zone;

int main(void) {
    constexpr auto max_order = 30;
    constexpr auto pages = ((1 << max_order) >> 12);  // Number of 4kib large pages in 1GiB
    constexpr auto memory_size = 1 << max_order;

    auto base = new uint64_t[1 << max_order];
    BuddyAllocator instance(base);

    std::cout << "Buddy test: Allocating " << pages << " pages which totals " << memory_size << " bytes.\n";
    std::cout << "Base: 0x" << std::hex << (size_t)base << '\n';
    zone.init(base);

    auto start = std::chrono::high_resolution_clock::now();

    // Allocate 1GiB in 4kib chunks for testing purposes
    for (auto i = 0; i < pages; i++) {
        auto res = zone.allocate(12);
        if (!res.address) {
            break;
        }
        // instance.free(res);
        // auto ptr = instance.alloc(4096);
        // std::cout << "#" << std::dec << i << ": " << std::hex << res.address << '\n';
    }

    // Expectedly fails. Allocator is OOM.
    instance.alloc(12);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    std::chrono::duration<double> sec = elapsed;

    std::cout << "Allocation of " << std::dec << pages << " 4KiB large pages took "
              << usec.count() << " usec (" << sec.count() << " seconds)" << std::endl;

    delete[] base;
}

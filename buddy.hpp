#pragma once

#include <stdint.h>

#include <algorithm>  // std::min, std::max
#include <cassert>    // assert
#include <cstring>    // std::memset
#include <iostream>   // std::cout, std::endl

namespace {
static constexpr bool verbose = false;
}

enum FillMode : char {
    ZERO = 0,
    NONE = 1  // Don't fill
};

template <int highest_order>
class BuddyAllocator {
public:
    using Order = int;
    using PhysicalAddress = uint64_t *;

    constexpr static Order min_order = 9;
    constexpr static Order max_order = highest_order - 3;

    BuddyAllocator(PhysicalAddress base)
        : base(base) {
        if constexpr (verbose)
            std::cout << "min-order: " << min_order << ", max-order: " << max_order << std::endl;
    }

    PhysicalAddress alloc_order(Order order, FillMode fill = FillMode::NONE) {
        return alloc(1 << order, fill);
    }

    PhysicalAddress alloc(uint64_t order, FillMode fill = FillMode::NONE) {
        int idx_begin, idx_end;

        if (order > freelist.min_order or order < freelist.max_order) {
            std::cout << "Bad order: " << order << "\n";
            return nullptr;
        }

        // # Step 1.
        // # Calculate the range of nodes we need to check.
        if (order == 0) {
            idx_begin = idx_end = 0;
        } else {
            idx_begin = freelist.nodes_at_order(order) - 1;
            idx_end = idx_begin + freelist.nodes_at_order(order) - 1;
        }

        // # Step 2.
        // # Find a free node at this order.
        // # Bit position of the node that was allocated (-1 indicates allocation failure)
        int allocated_node_idx = -1;

        // # Calculate the number of bit-shifts to get the left-most node of this order
        int shift = (1 << order) - 1;
        // std::cout << "begin, end: " << idx_begin << "," << idx_end << '\n';
        for (int x = idx_begin; x <= idx_end; x++) {
            // std::cout << "test: " << freelist.test_bit(shift) << " (shift: " << shift << ")\n";
            if (freelist.test_and_set_bit(shift) == 1) {
                allocated_node_idx = x;
                // std::cout << "Allocated node at index " << std::dec << x << " (range: " << idx_begin << "," << idx_end << ")\n";
                // std::exit(0);
                // # print(f"Allocated node at index {x} (Range: {idx_begin}-{idx_end})")

                break;
            }
            shift += 1;
        }


        // # Step 3.
        // # Allocate child and parent nodes
        if (allocated_node_idx == -1) {
            std::cout << "Failed to allocate memory at order " << order << '\n';
            return nullptr;
        }

        recursive_allocate_parent(allocated_node_idx);
        recursive_allocate_children(allocated_node_idx);

        if (allocated_node_idx == 1)
            allocated_node_idx = 0;

        // return (PhysicalAddress)(((uintptr_t)this->base + allocated_node_idx) );
        return (PhysicalAddress)((uintptr_t)(this->base) + (allocated_node_idx * freelist.order_to_size(order)));
        // return (BASE_ADDRESS + (allocated_node_idx * freelist.order_to_size(order)));
        return nullptr;
    }

    bool scan_order(int order) {
        int begin = 0;
        int end = freelist.nodes_at_order(order);
        for (int x = begin; x <= end; x++) {
            if (freelist.test_and_set_bit(x))
                return true;
        }

        return false;
    }

    void recursive_allocate_parent(int current_idx) {
        freelist.set_bit(current_idx);
        if (current_idx == 0) return;

        recursive_allocate_parent(freelist.parent_of(current_idx));
    }


    void recursive_allocate_children(int current_idx) {
        if (current_idx > freelist.last_bit) return;

        freelist.set_bit(current_idx);
        recursive_allocate_children(freelist.left_child(current_idx));
        recursive_allocate_children(freelist.right_child(current_idx));
    }


    void free(PhysicalAddress block, Order order) {
        if (block == nullptr)
            return;

        order -= 3;

        // There are no buddies at max_order
        if (order == max_order) {
            freelist.add(block, max_order - min_order);
            return;
        }

        coalesce(block, order);
    }

private:
    template <typename T, int orders>
    class Freelist {
        long bitmap[1024]{ 0 };
        static constexpr int BITMAP_BLOCK_SIZE = sizeof(int) * 8;

    public:
        static constexpr int page_size = 4096;
        static constexpr int TREE_DEPTH = 9;

        // # Represents the number of bit-shifts required to get the last bit
        // # (i.e. the right-most node at the bottom of the tree)
        static constexpr int last_bit = (1 << (TREE_DEPTH + 1)) - 1;
        static constexpr int first_bit = 0;

        static constexpr int min_order = TREE_DEPTH;
        static constexpr int max_order = 0;

    public:
        // # Util functions
        // #
        int order_to_size(int order) {
            return page_size * (1 << (min_order - order));
        }

        int nodes_at_order(int order) {
            return 1 << order;
        }

        bool test_bit(int idx) {
            return (bitmap[idx / BITMAP_BLOCK_SIZE] & (1 << (idx % BITMAP_BLOCK_SIZE)));
            // return (bitmap >> idx) & 1;
        }

        void set_bit(int idx) {
            bitmap[idx / BITMAP_BLOCK_SIZE] |= (1 << (idx % BITMAP_BLOCK_SIZE));
            // bitmap |= 1 << idx;
        }

        // # Set bit if clear
        // # Returns True if the bit could be set.
        // # If not, it returns False
        bool test_and_set_bit(int idx) {
            if (test_bit(idx) == 0) {
                set_bit(idx);
                return true;
            }

            return false;
        }


        int buddy_of(int idx) {
            return ((idx - 1) ^ 1) + 1;
        }


        // Return the parent's bit index of a given bit index
        int parent_of(int idx) {
            return (idx - 1) / 2;
        }


        int left_child(int idx) {
            return (idx * 2) + 1;
        }


        int right_child(int idx) {
            return (idx * 2) + 2;
        }


        T list[orders + 1]{};  // Todo: Think of a way to allocate enough memory for this array dynamically

    public:
        void add(const T &block, Order order) {
            if (block)
                *(T *)block = list[order];

            list[order] = block;
        }

        T remove(Order order) {
            T element = list[order];

            if (element == nullptr)
                return nullptr;

            list[order] = *(T *)list[order];

            return element;
        }

        T next(const T &block) const {
            return *reinterpret_cast<T *>(block);
        }

        bool find(const T &block, Order order) const {
            T element = list[order];
            while (element != nullptr && element != block) {
                element = next(element);
            }

            return element != nullptr;
        }
    };

    static constexpr int log2(int size) {
        int result = 0;
        while ((1U << result) < size) {
            ++result;
        }
        return result;
    }

    inline PhysicalAddress buddy_of(PhysicalAddress block, Order order) {
        return base + ((block - base) ^ (1 << order));
    }

    void coalesce(PhysicalAddress block, Order order) {
        // Description:
        // Try to merge 'block' and it's buddy into one larger block at 'order + 1'
        // If both block are free, remove them and insert the smaller of
        // the two blocks into the next highest order and repeat that process.

        if (order == max_order)
            return;

        PhysicalAddress buddy = buddy_of(block, order);
        if (buddy == nullptr)
            return;

        // Scan freelist for the buddy. (This isn't really efficient)
        // Could be optimized with a bitmap to track the state of each node.
        // The overhead of a bitmap would be minimal + the allocator itself has little to no overhead so size complexity should not be of concern.
        bool is_buddy_free = freelist.find(buddy, order - min_order);

        // Buddy block is free, merge them together
        if (is_buddy_free) {
            freelist.remove(order);
            coalesce(std::min(block, buddy), order + 1);  // std::min ensures that the smaller block of memory is merged with a larger and not vice-versa (which wouldn't work)
        }

        // The buddy is not free and merging is not possible.
        // Therefore it is simply put back onto the freelist.
        freelist.add(block, order - min_order);
    }

private:
    Freelist<PhysicalAddress, max_order - min_order> freelist;
    PhysicalAddress base;
};

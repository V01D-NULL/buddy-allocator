#pragma once

#include <stdint.h>

#include <algorithm>  // std::min, std::max
#include <cassert>    // assert
#include <cstring>    // std::memset
#include <iostream>   // std::cout, std::endl

namespace {
static constexpr bool verbose = false;
static constexpr bool safety_checks = false;
}  // namespace

enum FillMode : char {
    ZERO = 0,
    NONE = 1  // Don't fill
};

class BuddyAllocator {
public:
    using Order = int;
    using PhysicalAddress = uint64_t *;

    BuddyAllocator(PhysicalAddress base)
        : base(base) {
    }

    PhysicalAddress alloc(uint64_t order, FillMode fill = FillMode::NONE) {
        int idx_begin, idx_end;

        if constexpr (safety_checks) {
            if (order > helper.min_order || order < helper.max_order) {
                std::cout << "Bad order: " << order << "\n";
                return nullptr;
            }
        }

        // Step 1.
        // Calculate the range of nodes we need to check.
        if (order == 0) {
            idx_begin = idx_end = 0;
        } else {
            idx_begin = helper.nodes_at_order(order) - 1;
            idx_end = idx_begin + helper.nodes_at_order(order) - 1;
        }

        // Step 2.
        // Find a free node at this order.
        // Bit position of the node that was allocated (-1 indicates allocation failure)
        int allocated_node_idx = -1;

        // Calculate the number of bit-shifts to get the left-most node of this order
        int shift = (1 << order) - 1;
        for (int x = idx_begin; x <= idx_end; x++) {
            if (helper.test_and_set_bit(shift) == 1) {
                allocated_node_idx = x;
                break;
            }
            shift += 1;
        }

        // Step 3.
        // Allocate child and parent nodes
        if (allocated_node_idx == -1) {
            std::cout << "Failed to allocate memory at order " << order << '\n';
            return nullptr;
        }

        recursive_allocate_parent(allocated_node_idx);
        recursive_allocate_children(allocated_node_idx);

        if (allocated_node_idx == 1)
            allocated_node_idx = 0;

        // return (PhysicalAddress)(((uintptr_t)this->base + allocated_node_idx) );
        return (PhysicalAddress)((uintptr_t)(this->base) + (allocated_node_idx * helper.order_to_size(order)));
        // return (BASE_ADDRESS + (allocated_node_idx * helper.order_to_size(order)));
        return nullptr;
    }

    bool scan_order(int order) {
        int begin = 0;
        int end = helper.nodes_at_order(order);
        for (int x = begin; x <= end; x++) {
            if (helper.test_and_set_bit(x))
                return true;
        }

        return false;
    }

    void recursive_allocate_parent(int current_idx) {
        helper.set_bit(current_idx);
        if (current_idx == 0) return;

        recursive_allocate_parent(helper.parent_of(current_idx));
    }


    void recursive_allocate_children(int current_idx) {
        if (current_idx > helper.last_bit) return;

        helper.set_bit(current_idx);
        recursive_allocate_children(helper.left_child(current_idx));
        recursive_allocate_children(helper.right_child(current_idx));
    }


    void free(PhysicalAddress block, Order order) {
    }

private:
    class Helper {
        long bitmap[16363]{ 0 };
        static constexpr int bitmap_block_size = sizeof(int) * 8;

    public:
        static constexpr int page_size = 4096;
        static constexpr int tree_depth = 13;

        // Represents the number of bit-shifts required to get the last bit (aka number of bits in the bitmap)
        // (i.e. the right-most node at the bottom of the tree)
        static constexpr int last_bit = (1 << (tree_depth + 1)) - 1;
        static constexpr int first_bit = 0;

        static constexpr int min_order = tree_depth;
        static constexpr int max_order = 0;

    public:
        // Util functions
        //
        int order_to_size(int order) {
            return page_size * (1 << (min_order - order));
        }

        int nodes_at_order(int order) {
            return 1 << order;
        }

        bool test_bit(int idx) {
            return (bitmap[idx / bitmap_block_size] & (1 << (idx % bitmap_block_size)));
        }

        void set_bit(int idx) {
            bitmap[idx / bitmap_block_size] |= (1 << (idx % bitmap_block_size));
        }

        // Set bit if clear
        // Returns True if the bit could be set.
        // If not, it returns False
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
    };

    void coalesce(PhysicalAddress block, Order order) {
    }

private:
    Helper helper;
    PhysicalAddress base;
};

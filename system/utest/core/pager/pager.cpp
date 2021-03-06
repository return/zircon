// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/algorithm.h>
#include <fbl/function.h>
#include <unittest/unittest.h>

#include "test_thread.h"
#include "userpager.h"

namespace pager_tests {

// Simple test that checks that a single thread can access a single page
bool single_page_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(1, &vmo));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });

    ASSERT_TRUE(t.Start());

    ASSERT_TRUE(pager.WaitForPageRead(vmo, 0, 1, ZX_TIME_INFINITE));

    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));

    ASSERT_TRUE(t.Wait());

    END_TEST;
}

// Tests that pre-supplied pages don't result in requests
bool presupply_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(1, &vmo));

    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });

    ASSERT_TRUE(t.Start());

    ASSERT_TRUE(t.Wait());

    ASSERT_FALSE(pager.WaitForPageRead(vmo, 0, 1, 0));

    END_TEST;
}

// Tests that supplies between the request and reading the port
// causes the request to be aborted
bool early_supply_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(2, &vmo));

    TestThread t1([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });
    // Use a second thread to make sure the queue of requests is flushed
    TestThread t2([vmo]() -> bool {
        return vmo->CheckVmar(1, 1);
    });

    ASSERT_TRUE(t1.Start());
    ASSERT_TRUE(t1.WaitForBlocked());
    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));
    ASSERT_TRUE(t1.Wait());

    ASSERT_TRUE(t2.Start());
    ASSERT_TRUE(t2.WaitForBlocked());
    ASSERT_TRUE(pager.WaitForPageRead(vmo, 1, 1, ZX_TIME_INFINITE));
    ASSERT_TRUE(pager.SupplyPages(vmo, 1, 1));
    ASSERT_TRUE(t2.Wait());

    ASSERT_FALSE(pager.WaitForPageRead(vmo, 0, 1, 0));

    END_TEST;
}

// Checks that a single thread can sequentially access multiple pages
bool sequential_multipage_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    constexpr uint32_t kNumPages = 32;
    ASSERT_TRUE(pager.CreateVmo(kNumPages, &vmo));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, kNumPages);
    });

    ASSERT_TRUE(t.Start());

    for (unsigned i = 0; i < kNumPages; i++) {
        ASSERT_TRUE(pager.WaitForPageRead(vmo, i, 1, ZX_TIME_INFINITE));
        ASSERT_TRUE(pager.SupplyPages(vmo, i, 1));
    }

    ASSERT_TRUE(t.Wait());

    END_TEST;
}

// Tests that multiple threads can concurrently access different pages
bool concurrent_multipage_access_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(2, &vmo));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });
    TestThread t2([vmo]() -> bool {
        return vmo->CheckVmar(1, 1);
    });

    ASSERT_TRUE(t.Start());
    ASSERT_TRUE(t2.Start());

    ASSERT_TRUE(pager.WaitForPageRead(vmo, 0, 1, ZX_TIME_INFINITE));
    ASSERT_TRUE(pager.WaitForPageRead(vmo, 1, 1, ZX_TIME_INFINITE));
    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 2));

    ASSERT_TRUE(t.Wait());
    ASSERT_TRUE(t2.Wait());

    END_TEST;
}

// Tests that multiple threads can concurrently access a single page
bool concurrent_overlapping_access_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(1, &vmo));

    constexpr uint64_t kNumThreads = 32;
    fbl::unique_ptr<TestThread> threads[kNumThreads];
    for (unsigned i = 0; i < kNumThreads; i++) {
        threads[i] = fbl::make_unique<TestThread>([vmo]() -> bool {
            return vmo->CheckVmar(0, 1);
        });

        threads[i]->Start();
        ASSERT_TRUE(threads[i]->WaitForBlocked());
    }

    ASSERT_TRUE(pager.WaitForPageRead(vmo, 0, 1, ZX_TIME_INFINITE));
    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));

    for (unsigned i = 0; i < kNumThreads; i++) {
        ASSERT_TRUE(threads[i]->Wait());
    }

    ASSERT_FALSE(pager.WaitForPageRead(vmo, 0, 1, 0));

    END_TEST;
}

// Tests that multiple threads can concurrently access multiple pages and
// be satisfied by a single supply operation.
bool bulk_single_supply_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    constexpr uint32_t kNumPages = 8;
    ASSERT_TRUE(pager.CreateVmo(kNumPages, &vmo));

    fbl::unique_ptr<TestThread> ts[kNumPages];
    for (unsigned i = 0; i < kNumPages; i++) {
        ts[i] = fbl::make_unique<TestThread>([vmo, i]() -> bool {
            return vmo->CheckVmar(i, 1);
        });
        ASSERT_TRUE(ts[i]->Start());
        ASSERT_TRUE(pager.WaitForPageRead(vmo, i, 1, ZX_TIME_INFINITE));
    }

    ASSERT_TRUE(pager.SupplyPages(vmo, 0, kNumPages));

    for (unsigned i = 0; i < kNumPages; i++) {
        ASSERT_TRUE(ts[i]->Wait());
    }

    END_TEST;
}

// Test body for odd supply tests
bool bulk_odd_supply_test_inner(bool use_src_offset) {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    // Interesting supply lengths that will exercise splice logic.
    constexpr uint64_t kSupplyLengths[] = {
        2, 3, 5, 7, 37, 5, 13, 23
    };
    uint64_t sum = 0;
    for (unsigned i = 0; i < fbl::count_of(kSupplyLengths); i++) {
        sum += kSupplyLengths[i];
    }

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(sum, &vmo));

    uint64_t page_idx = 0;
    for (unsigned supply_idx = 0; supply_idx < fbl::count_of(kSupplyLengths); supply_idx++) {
        uint64_t supply_len = kSupplyLengths[supply_idx];
        uint64_t offset = page_idx;

        fbl::unique_ptr<TestThread> ts[kSupplyLengths[supply_idx]];
        for (uint64_t j = 0; j < kSupplyLengths[supply_idx]; j++) {
            uint64_t thread_offset = offset + j;
            ts[j] = fbl::make_unique<TestThread>([vmo, thread_offset]() -> bool {
                return vmo->CheckVmar(thread_offset, 1);
            });
            ASSERT_TRUE(ts[j]->Start());
            ASSERT_TRUE(pager.WaitForPageRead(vmo, thread_offset, 1, ZX_TIME_INFINITE));
        }

        uint64_t src_offset = use_src_offset ? offset : 0;
        ASSERT_TRUE(pager.SupplyPages(vmo, offset, supply_len, src_offset));

        for (unsigned i = 0; i < kSupplyLengths[supply_idx]; i++) {
            ASSERT_TRUE(ts[i]->Wait());
        }

        page_idx += kSupplyLengths[supply_idx];
    }

    END_TEST;
}

// Test which exercises supply logic by supplying data in chunks of unusual length
bool bulk_odd_length_supply_test() {
    return bulk_odd_supply_test_inner(false);
}

// Test which exercises supply logic by supplying data in chunks of
// unusual lengths and offsets
bool bulk_odd_offset_supply_test() {
    return bulk_odd_supply_test_inner(true);
}

// Tests that supply doesn't overwrite existing content
bool overlap_supply_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(2, &vmo));

    zx::vmo alt_data_vmo;
    ASSERT_EQ(zx::vmo::create(ZX_PAGE_SIZE, 0, &alt_data_vmo), ZX_OK);
    uint8_t alt_data[ZX_PAGE_SIZE];
    vmo->GenerateBufferContents(alt_data, 1, 2);
    ASSERT_EQ(alt_data_vmo.write(alt_data, 0, ZX_PAGE_SIZE), ZX_OK);

    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1, std::move(alt_data_vmo)));
    ASSERT_TRUE(pager.SupplyPages(vmo, 1, 1));

    TestThread t([vmo, alt_data]() -> bool {
        return vmo->CheckVmar(0, 1, alt_data) && vmo->CheckVmar(1, 1);
    });

    ASSERT_TRUE(t.Start());

    ASSERT_TRUE(t.Wait());

    ASSERT_FALSE(pager.WaitForPageRead(vmo, 0, 1, 0));

    END_TEST;
}

// Tests that a pager can handle lots of pending page requests
bool many_request_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    constexpr uint32_t kNumPages = 257; // Arbitrary large number
    ASSERT_TRUE(pager.CreateVmo(kNumPages, &vmo));

    fbl::unique_ptr<TestThread> ts[kNumPages];
    for (unsigned i = 0; i < kNumPages; i++) {
        ts[i] = fbl::make_unique<TestThread>([vmo, i]() -> bool {
            return vmo->CheckVmar(i, 1);
        });
        ASSERT_TRUE(ts[i]->Start());
        ASSERT_TRUE(ts[i]->WaitForBlocked());
    }

    for (unsigned i = 0; i < kNumPages; i++) {
        ASSERT_TRUE(pager.WaitForPageRead(vmo, i, 1, ZX_TIME_INFINITE));
        ASSERT_TRUE(pager.SupplyPages(vmo, i, 1));
        ASSERT_TRUE(ts[i]->Wait());
    }

    END_TEST;
}

// Tests that a pager can support creating and destroying successive vmos
bool successive_vmo_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    constexpr uint32_t kNumVmos = 64;
    for (unsigned i = 0; i < kNumVmos; i++) {
        PagedVmo* vmo;
        ASSERT_TRUE(pager.CreateVmo(1, &vmo));

        TestThread t([vmo]() -> bool {
            return vmo->CheckVmar(0, 1);
        });

        ASSERT_TRUE(t.Start());

        ASSERT_TRUE(pager.WaitForPageRead(vmo, 0, 1, ZX_TIME_INFINITE));

        ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));

        ASSERT_TRUE(t.Wait());

        pager.ReleaseVmo(vmo);
    }

    END_TEST;
}

// Tests that a pager can support multiple concurrent vmos
bool multiple_concurrent_vmo_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    constexpr uint32_t kNumVmos = 8;
    PagedVmo* vmos[kNumVmos];
    fbl::unique_ptr<TestThread> ts[kNumVmos];
    for (unsigned i = 0; i < kNumVmos; i++) {
        ASSERT_TRUE(pager.CreateVmo(1, vmos + i));

        ts[i] = fbl::make_unique<TestThread>([vmo = vmos[i]]() -> bool {
            return vmo->CheckVmar(0, 1);
        });

        ASSERT_TRUE(ts[i]->Start());

        ASSERT_TRUE(pager.WaitForPageRead(vmos[i], 0, 1, ZX_TIME_INFINITE));
    }

    for (unsigned i = 0; i < kNumVmos; i++) {
        ASSERT_TRUE(pager.SupplyPages(vmos[i], 0, 1));

        ASSERT_TRUE(ts[i]->Wait());
    }

    END_TEST;
}

// Tests that unmapping a vmo while threads are blocked on a pager read
// eventually results in pagefaults.
bool vmar_unmap_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(1, &vmo));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });
    ASSERT_TRUE(t.Start());
    ASSERT_TRUE(t.WaitForBlocked());

    ASSERT_TRUE(pager.UnmapVmo(vmo));
    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));

    ASSERT_TRUE(t.WaitForCrash(vmo->GetBaseAddr()));

    END_TEST;
}

// Tests that replacing a vmar mapping while threads are blocked on a
// pager read results in reads to the new mapping.
bool vmar_remap_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    constexpr uint32_t kNumPages = 8;
    ASSERT_TRUE(pager.CreateVmo(kNumPages, &vmo));

    fbl::unique_ptr<TestThread> ts[kNumPages];
    for (unsigned i = 0; i < kNumPages; i++) {
        ts[i] = fbl::make_unique<TestThread>([vmo, i]() -> bool {
            return vmo->CheckVmar(i, 1);
        });
        ASSERT_TRUE(ts[i]->Start());
    }
    for (unsigned i = 0; i < kNumPages; i++) {
        ASSERT_TRUE(ts[i]->WaitForBlocked());
    }

    zx::vmo old_vmo;
    ASSERT_TRUE(pager.ReplaceVmo(vmo, &old_vmo));

    zx::vmo tmp;
    ASSERT_EQ(zx::vmo::create(kNumPages * ZX_PAGE_SIZE, 0, &tmp), ZX_OK);
    ASSERT_EQ(tmp.op_range(ZX_VMO_OP_COMMIT, 0, kNumPages * ZX_PAGE_SIZE, nullptr, 0), ZX_OK);
    ASSERT_EQ(zx_pager_supply_pages(pager.pager(), old_vmo.get(),
                                    0, kNumPages * ZX_PAGE_SIZE, tmp.get(), 0),
              ZX_OK);

    for (unsigned i = 0; i < kNumPages; i++) {
        uint64_t offset, length;
        ASSERT_TRUE(pager.GetPageReadRequest(vmo, ZX_TIME_INFINITE, &offset, &length));
        ASSERT_EQ(length, 1);
        ASSERT_TRUE(pager.SupplyPages(vmo, offset, 1));
        ASSERT_TRUE(ts[offset]->Wait());
    }

    END_TEST;
}

// Checks that a thread blocked on accessing a paged vmo can be safely killed.
bool thread_kill_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(2, &vmo));

    TestThread t1([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });
    TestThread t2([vmo]() -> bool {
        return vmo->CheckVmar(1, 1);
    });

    ASSERT_TRUE(t1.Start());
    ASSERT_TRUE(t1.WaitForBlocked());

    ASSERT_TRUE(t2.Start());
    ASSERT_TRUE(t2.WaitForBlocked());

    ASSERT_TRUE(t1.Kill());
    ASSERT_TRUE(t1.WaitForTerm());

    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 2));

    ASSERT_TRUE(t2.Wait());

    END_TEST;
}

// Checks that a thread blocked on accessing a paged vmo can be safely killed
// when there is a second thread waiting for the same address.
bool thread_kill_overlap_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(1, &vmo));

    TestThread t1([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });
    TestThread t2([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });

    ASSERT_TRUE(t1.Start());
    ASSERT_TRUE(t1.WaitForBlocked());

    ASSERT_TRUE(t2.Start());
    ASSERT_TRUE(t2.WaitForBlocked());

    ASSERT_TRUE(pager.WaitForPageRead(vmo, 0, 1, ZX_TIME_INFINITE));

    ASSERT_TRUE(t1.Kill());
    ASSERT_TRUE(t1.WaitForTerm());

    ASSERT_TRUE(pager.SupplyPages(vmo, 0, 1));

    ASSERT_TRUE(t2.Wait());

    END_TEST;
}

// Tests that closing a pager while a thread is accessing it doesn't cause
// problems (other than a page fault in the accessing thread).
bool close_pager_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(2, &vmo));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });
    ASSERT_TRUE(pager.SupplyPages(vmo, 1, 1));

    ASSERT_TRUE(t.Start());
    ASSERT_TRUE(t.WaitForBlocked());

    pager.ClosePagerHandle();

    ASSERT_TRUE(t.WaitForCrash(vmo->GetBaseAddr()));
    ASSERT_TRUE(vmo->CheckVmar(1, 1));

    END_TEST;
}

// Tests that closing an in use port doesn't cause issues (beyond no
// longer being able to receive requests).
bool close_port_test() {
    BEGIN_TEST;

    UserPager pager;

    ASSERT_TRUE(pager.Init());

    PagedVmo* vmo;
    ASSERT_TRUE(pager.CreateVmo(2, &vmo));

    TestThread t([vmo]() -> bool {
        return vmo->CheckVmar(0, 1);
    });

    ASSERT_TRUE(t.Start());
    ASSERT_TRUE(t.WaitForBlocked());

    pager.ClosePortHandle();

    ASSERT_TRUE(pager.SupplyPages(vmo, 1, 1));
    ASSERT_TRUE(vmo->CheckVmar(1, 1));

    pager.ClosePagerHandle();

    ASSERT_TRUE(t.WaitForCrash(vmo->GetBaseAddr()));

    END_TEST;
}

// Tests focused on reading a paged vmo

BEGIN_TEST_CASE(pager_read_tests)
RUN_TEST(single_page_test);
RUN_TEST(presupply_test);
RUN_TEST(early_supply_test);
RUN_TEST(sequential_multipage_test);
RUN_TEST(concurrent_multipage_access_test);
RUN_TEST(concurrent_overlapping_access_test);
RUN_TEST(bulk_single_supply_test);
RUN_TEST(bulk_odd_length_supply_test);
RUN_TEST(bulk_odd_offset_supply_test);
RUN_TEST(overlap_supply_test);
RUN_TEST(many_request_test);
RUN_TEST(successive_vmo_test);
RUN_TEST(multiple_concurrent_vmo_test);
RUN_TEST(vmar_unmap_test);
RUN_TEST(vmar_remap_test);
END_TEST_CASE(pager_read_tests)

// Tests focused on lifecycle of pager and paged vmos

BEGIN_TEST_CASE(lifecycle_tests)
RUN_TEST(thread_kill_test);
RUN_TEST(thread_kill_overlap_test);
RUN_TEST(close_pager_test);
RUN_TEST(close_port_test);
END_TEST_CASE(lifecycle_tests)

} // namespace pager_tests

//TODO: Test cases which violate various syscall invalid args

#ifndef BUILD_COMBINED_TESTS
int main(int argc, char** argv) {
    if (!unittest_run_all_tests(argc, argv)) {
        return -1;
    }
    return 0;
}
#endif

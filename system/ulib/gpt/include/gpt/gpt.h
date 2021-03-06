// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/unique_ptr.h>
#include <gpt/c/gpt.h>

namespace gpt {

constexpr uint32_t kPartitionCount = 128;
constexpr uint64_t kGuidStrLength = 37;

#define GPT_DIFF_TYPE    (0x01u)
#define GPT_DIFF_GUID    (0x02u)
#define GPT_DIFF_FIRST   (0x04u)
#define GPT_DIFF_LAST    (0x08u)
#define GPT_DIFF_FLAGS   (0x10u)
#define GPT_DIFF_NAME    (0x20u)

class GptDevice {
public:
    static zx_status_t Create(int fd, uint32_t blocksize, uint64_t blocks,
                              fbl::unique_ptr<GptDevice>* out_dev);

    // returns true if the partition table on the device is valid
    bool Valid() const { return valid_; }

    // Returns the range of usable blocks within the GPT, from [block_start, block_end] (inclusive)
    int Range(uint64_t* block_start, uint64_t* block_end) const;

    // Writes changes to partition table to the device. If the device does not contain valid
    // GPT, a gpt header gets created. Sync doesn't nudge block device driver to rescan the
    // partitions. So it is the caller's responsibility to rescan partitions for the changes
    // if needed.
    int Sync();

    // perform all checks and computations on the in-memory representation, but DOES
    // NOT write it out to disk. To perform checks AND write to disk, use Sync
    int Finalize();

    // Adds a partition to in-memory instance of GptDevice. The changes stay visible
    // only to this instace. Needs a Sync to write the changes to the device.
    int AddPartition(const char* name, const uint8_t* type,
                     const uint8_t* guid, uint64_t offset, uint64_t blocks,
                     uint64_t flags);

    // Writes zeroed blocks at an arbitrary offset (in blocks) within the device.
    //
    // Can be used alongside gpt_partition_add to ensure a newly created partition
    // will not read stale superblock data.
    int ClearPartition(uint64_t offset, uint64_t blocks);

    // Removes a partition from in-memory instance of GptDevice. The changes stay visible
    // only to this instace. Needs a Sync to write the changes to the device.
    int RemovePartition(const uint8_t* guid);

    // Removes all partitions from in-memory instance of GptDevice. The changes stay visible
    // only to this instace. Needs a Sync to write the changes to the device.
    int RemoveAllPartitions();

    // given a gpt device, get the GUID for the disk
    void GetHeaderGuid(uint8_t (*disk_guid_out)[GPT_GUID_LEN]) const;

    // return true if partition# idx has been locally modified
    int GetDiffs(uint32_t idx, uint32_t* diffs) const;

    // returns a pointer to partition at index pindex
    gpt_partition_t* GetPartition(uint32_t pindex) const {
        ZX_ASSERT(pindex < kPartitionCount);
        return partitions_[pindex];
    }

    // print out the GPT
    void PrintTable() const;

private:
    GptDevice() { valid_ = false; };

    int FinalizeAndSync(bool persist);

    // read the partition table from the device.
    zx_status_t Init(int fd, uint32_t blocksize, uint64_t blocks);

    // true if the partition table on the device is valid
    bool valid_;

    // pointer to a list of partitions
    gpt_partition_t* partitions_[kPartitionCount] = {};

    // device to use
    int fd_ = -1;

    // block size in bytes
    uint64_t blocksize_ = 0;

    // number of blocks
    uint64_t blocks_ = 0;

    // true if valid mbr exists on disk
    bool mbr_ = false;

    // header buffer, should be primary copy
    gpt_header_t header_ = {};

    // partition table buffer
    gpt_partition_t ptable_[kPartitionCount] = {};

    // copy of buffer from when last init'd or sync'd.
    gpt_partition_t ptable_backup_[kPartitionCount] = {};
};

} // namespace gpt

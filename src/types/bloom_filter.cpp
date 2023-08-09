// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <cstdint>
#include <cstring>
#include <memory>

#include "bloom_filter.h"

#include "xxh3.h"

constexpr uint32_t BlockSplitBloomFilter::SALT[kBitsSetPerBlock];

BlockSplitBloomFilter::BlockSplitBloomFilter() {}

bool BlockSplitBloomFilter::Init(uint32_t num_bytes) {
  if (num_bytes < kMinimumBloomFilterBytes) {
    num_bytes = kMinimumBloomFilterBytes;
  }

  // Get next power of 2 if it is not power of 2.
  if ((num_bytes & (num_bytes - 1)) != 0) {
    num_bytes = static_cast<uint32_t>(NextPower2(num_bytes));
  }

  if (num_bytes > kMaximumBloomFilterBytes) {
    num_bytes = kMaximumBloomFilterBytes;
  }

  num_bytes_ = num_bytes;
  data_.resize(num_bytes_, 0);
  return true;
}

bool BlockSplitBloomFilter::Init(const uint8_t* bitset, uint32_t num_bytes) {
  if (num_bytes < kMinimumBloomFilterBytes || num_bytes > kMaximumBloomFilterBytes ||
      (num_bytes & (num_bytes - 1)) != 0) {
    return false;
  }

  num_bytes_ = num_bytes;
  data_ = {reinterpret_cast<const char*>(bitset), num_bytes};
}

bool BlockSplitBloomFilter::Init(std::string bitset) {
  if (bitset.size() < kMinimumBloomFilterBytes || bitset.size() > kMaximumBloomFilterBytes ||
      (bitset.size() & (bitset.size() - 1)) != 0) {
    return false;
  }

  num_bytes_ = bitset.size();
  data_ = std::move(bitset);
}


static constexpr uint32_t kBloomFilterHeaderSizeGuess = 256;

bool BlockSplitBloomFilter::FindHash(uint64_t hash) const {
  const uint32_t bucket_index =
      static_cast<uint32_t>(((hash >> 32) * (num_bytes_ / kBytesPerFilterBlock)) >> 32);
  const uint32_t key = static_cast<uint32_t>(hash);
  const uint32_t* bitset32 = reinterpret_cast<const uint32_t*>(data_->data());

  for (int i = 0; i < kBitsSetPerBlock; ++i) {
    // Calculate mask for key in the given bitset.
    const uint32_t mask = UINT32_C(0x1) << ((key * SALT[i]) >> 27);
    if ((0 == (bitset32[kBitsSetPerBlock * bucket_index + i] & mask))) {
      return false;
    }
  }
  return true;
}

void BlockSplitBloomFilter::InsertHash(uint64_t hash) {
  const uint32_t bucket_index =
      static_cast<uint32_t>(((hash >> 32) * (num_bytes_ / kBytesPerFilterBlock)) >> 32);
  const uint32_t key = static_cast<uint32_t>(hash);
  uint32_t* bitset32 = reinterpret_cast<uint32_t*>(data_.data());

  for (int i = 0; i < kBitsSetPerBlock; i++) {
    // Calculate mask for key in the given bitset.
    const uint32_t mask = UINT32_C(0x1) << ((key * SALT[i]) >> 27);
    bitset32[bucket_index * kBitsSetPerBlock + i] |= mask;
  }
}

uint64_t BlockSplitBloomFilter::Hash(const char* data, size_t length) const {
  return XXH64(data, length, /*seed=*/0);
}

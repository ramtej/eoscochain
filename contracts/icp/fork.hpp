#pragma once

#include <eosiolib/singleton.hpp>

#include "icp.hpp"

namespace eosio {

using eosio::multi_index;
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::singleton;

key256 to_key256(const checksum256& c) {
    std::array<uint8_t, 32> a;
    std::copy(std::begin(c.hash), std::end(c.hash), a.begin());
    return key256(a);
}

using stored_block_header_ptr = std::shared_ptr<struct stored_block_header>;
using stored_block_header_state_ptr = std::shared_ptr<struct stored_block_header_state>;

/* Irreversible block header */
struct stored_block_header {
    uint64_t pk;

    block_id_type id;
    uint32_t block_num;

    block_id_type previous;

    checksum256 action_mroot = checksum256{};

    bool has_action_mroot() {
        uint8_t zero[32] = {};
        return !std::equal(std::cbegin(action_mroot.hash), std::cend(action_mroot.hash), std::cbegin(zero.hash), std::cend(zero.hash));
    }

    auto primary_key() const { return pk; }
    key256 by_blockid() const { return to_key256(id); }
    key256 by_prev() const { return to_key256(previous); }
    uint32_t by_blocknum() const { return block_num; }
};

typedef multi_index<N(block), stored_block_header,
        indexed_by<N(blockid), const_mem_fun<stored_block_header, key256, &stored_block_header::by_blockid>>,
        indexed_by<N(prev), const_mem_fun<stored_block_header, key256, &stored_block_header::by_prev>>,
        indexed_by<N(blocknum), const_mem_fun<stored_block_header, uint32_t, &stored_block_header::by_blocknum>>
> stored_block_header_table;

/* Block header state */
struct stored_block_header_state {
    uint64_t pk;

    block_id_type id;
    uint32_t block_num;

    block_id_type previous;

    uint32_t dpos_irreversible_blocknum;
    uint32_t bft_irreversible_blocknum;

    incremental_merkle blockroot_merkle; // merkle root of block ids

    uint32_t last_irreversible_blocknum() {
       return std::max(dpos_irreversible_blocknum, bft_irreversible_blocknum);
    }

    auto primary_key() const { return pk; }
    key256 by_blockid() const { return to_key256(id); }
    key256 by_prev() const { return to_key256(previous); }
    uint32_t by_blocknum() const { return block_num; }
    uint128_t by_lib_block_num() const {
       return std::numeric_limits<uint128_t>::max() - ((uint128_t(dpos_irreversible_blocknum) << 64) + (uint128_t(bft_irreversible_blocknum) << 32) + block_num);
    }
};

typedef multi_index<N(blockstate), stored_block_header_state,
        indexed_by<N(blockid), const_mem_fun<stored_block_header_state, key256, &stored_block_header_state::by_blockid>>,
        indexed_by<N(prev), const_mem_fun<stored_block_header_state, key256, &stored_block_header_state::by_prev>>,
        indexed_by<N(blocknum), const_mem_fun<stored_block_header_state, uint32_t, &stored_block_header_state::by_blocknum>>,
        indexed_by<N(libblocknum), const_mem_fun<stored_block_header_state, uint128_t, &stored_block_header_state::by_lib_block_num>>
> stored_block_header_state_table;

typedef singleton<N(activesched), producer_schedule> producer_schedule_singleton;

struct pending_schedule {
   uint32_t pending_schedule_lib_num;
   digest_type pending_schedule_hash;
   producer_schedule pending_schedule;
};
typedef singleton<N(pendingsched), pending_schedule> pending_schedule_singleton;

struct store_parameters {
   uint32_t max_blocks;
};
typedef singleton<N(storeparas), store_parameters> store_parameters_singleton;

using fork_store_ptr = std::shared_ptr<class fork_store>;

class fork_store {
public:
    fork_store(account_name code);

    void setmaxblocks(uint32_t max);
    void add_block_header_with_merkle_path(const block_header_state& h, const vector<block_id_type>& merkle_path);
    void add_block_header(const block_header& h);
    bool is_producer(account_name name, const ::public_key& key);
    producer_schedule get_producer_schedule();
    void update_producer_schedule(const producer_schedule& schedule);
    incremental_merkle get_block_mroot(const block_id_type& block_id);
    checksum256_ptr get_action_mroot(const block_id_type& block_id);
    void cutdown(uint32_t num);

private:
    void add_block_state(const block_header_state& block_state);
    void add_block_id(const block_id_type& block_id);
    void set_pending_schedule(uint32_t lib_num, const digest_type& hash, const producer_schedule& schedule);
    void prune(const stored_block_header_state& block_state);
    void remove(const block_id_type& id);

    account_name _code;
    stored_block_header_state _head;
    producer_schedule_ptr _producers;
    stored_block_header_state_table _block_states;
    stored_block_header_table _blocks;
    producer_schedule_singleton _producer_schedule;
    pending_schedule_singleton _pending_schedule;
    store_parameters_singleton _store_parameters;
};

}

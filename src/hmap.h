/*
    This file is part of ehmap.

    ehmap is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ehmap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ehmap.  If not, see <https://www.gnu.org/licenses/>.
 * */

#ifndef _HMAP_
#define _HMAP_

#include <atomic>
#include <vector>
#include <cstdint>

namespace ema {

	typedef uint32_t	hash_type;

	template<typename Key, typename Value, typename Hasher,
		size_t Nbuckets = 1024, size_t Nelems = (Nbuckets*8)>
	class hmap {
		struct kv {
			Key	k;
			Value	v;
		};

		struct kv_chunk {
			std::atomic<uint32_t>	cur_pair;
			kv			pairs[Nelems];
			std::atomic<uint32_t>	unused_pairs;

			kv_chunk() noexcept : cur_pair(0), unused_pairs(0) {
			}

			bool insert_kv(const Key& k, const Value& v, uint32_t& idx) {
				uint32_t	cv = cur_pair.load();
				if(cv >= Nelems)
					return false;
				if(!cur_pair.compare_exchange_strong(cv, cv+1))
					return insert_kv(k, v, idx);
				pairs[cv].k = k;
				pairs[cv].v = v;
				idx = cv;
				return true;
			}
		};

		struct hash_index {
			uint32_t	hash,
					index;

			hash_index() noexcept : hash(0), index(0) {
			}

			hash_index(const uint32_t h_, const uint32_t i_) noexcept : hash(h_), index(i_) {
			}
		};

		const static int		MAX_HASH_ENTRIES = 7;

		struct hash_entry {
			std::atomic<hash_index>		entries[MAX_HASH_ENTRIES];
			std::atomic<hash_entry*>	next;

			hash_entry() : next(0) {
			}

			int size(void) const {
				for(int i = 0; i < MAX_HASH_ENTRIES; ++i) {
					const auto	e = entries[i].load();
					if(!e.hash)
						return i;
				}
				return MAX_HASH_ENTRIES-1;
			}

			const std::atomic<hash_index>* find(const hash_type h, const Key& k, const kv_chunk* data) const {
				for(int i = 0; i < MAX_HASH_ENTRIES; ++i) {
					const auto	e = entries[i].load();
					// if we don't have valid
					// value, that's it, we don't
					// have any more entries
					if(!e.hash)
						return 0;
					// otherwise compare if it's
					// the same
					if(e.hash == h) {
						// then compare the key
						// value itself
						if(data->pairs[e.index].k == k)
							return &entries[i];
					}
				}
				if(next.load(std::memory_order_relaxed)) return next.load(std::memory_order_relaxed)->find(h, k, data);
				else return 0;
			}

			template<typename FnAddEntry>
			const std::atomic<hash_index>* insert_once(const hash_type h, const Key& k, const Value& v, kv_chunk* data, FnAddEntry&& fn_add_entry, uint32_t idx = (uint32_t)-1) {
				for(int i = 0; i < MAX_HASH_ENTRIES; ++i) {
					auto	e = entries[i].load();
					// if we don't have valid
					// value, that's it, try to
					// write it
					if(!e.hash) {
						// if we don't have an assigned index
						// then assign one
						if(idx == (uint32_t)-1) {
							if(!data->insert_kv(k, v, idx))
								return 0;
						}
						hash_index	hi(h, idx);
						if(entries[i].compare_exchange_strong(e, hi))
							return &entries[i];
						// if we can't do it, call again this function...
						// need to keep stack at a minimum...
						// The compiler should know that...
						return this->insert_once(h, k, v, data, fn_add_entry, idx);
					}
					// otherwise compare if it's
					// the same
					if(e.hash == h) {
						// then compare the key
						// value itself
						if(data->pairs[e.index].k == k) {
							// if it's the same, just return 0
							// if we have a non (-1) index then
							// count for unused_pairs and reset
							// them with default initialization
							if(idx != (uint32_t)-1) {
								data->pairs[idx].k = Key();
								data->pairs[idx].v = Value();
								++data->unused_pairs;
							}
							return 0;
						}
					}
				}
				// manage allocation of
				// new bucket list...
				if(next.load(std::memory_order_relaxed))
					return next.load(std::memory_order_relaxed)->insert_once(h, k, v, data, fn_add_entry, idx);
				// otherwise, swap it - we may have a
				// 'soft' memory (entry) leak
				hash_entry	*cur = 0,
						*cur_next = fn_add_entry();
				if(next.compare_exchange_strong(cur, cur_next)) {
					return cur_next->insert_once(h, k, v, data, fn_add_entry, idx);
				} else {
					// just use the other value
					return cur->insert_once(h, k, v, data, fn_add_entry, idx);
				}
			}
		};

		static_assert(sizeof(hash_index) == 8, "hash_index structure has to be 8 bytes");
		static_assert(sizeof(hash_entry) == 64, "hash_entry structure has to be 64 bytes (cacheline)");

		// idea of this structure is to hold entries
		// when we exceed the max elements per entry
		// Ideally this should never be used, means
		// or we have not many buckets
		// or we have a bad hash function 
		struct add_entries {
			const static int		N_ENTRIES = 8*1024*1024/sizeof(hash_entry);

			std::atomic<uint32_t>		cur_entry;
			hash_entry			entries[N_ENTRIES];
			std::atomic<add_entries*>	next;

			add_entries() : cur_entry(0), next(0) {
			}

			hash_entry* get_entry(void) {
				// if we have already a next
				// naviagte through. Again not
				// optimal on purpose
				if(next.load(std::memory_order_relaxed))
					return next.load(std::memory_order_relaxed)->get_entry();
				// check if entries are already maxed out
				// in case, initialize a new 'add_entries'
				uint32_t	tmp_cur_entry = cur_entry.load(std::memory_order_relaxed);
				if(tmp_cur_entry >= N_ENTRIES) {
					// allocate a new structure
					add_entries	*next_entries = new add_entries(),
							*cur_next = 0;
					// replace it atomically - if we fail
					// it means someone else did it before
					if(next.compare_exchange_strong(cur_next, next_entries)) {
						return next_entries->get_entry();
					} else {
						delete next_entries;
						return get_entry();
					}
				}
				// try to get ahold of cur_entry
				// and reserve one
				// If we can't reserve, re-execute this
				// method
				if(!cur_entry.compare_exchange_strong(tmp_cur_entry, tmp_cur_entry+1)) {
					// the compiler should know to emit code
					// not to use the stack...
					return get_entry();
				}
				return &entries[tmp_cur_entry];
			}
		};

		hash_entry			*entries_;
		kv_chunk			*data_;
		std::atomic<add_entries*>	add_entries_;

		const static uint32_t	HASH_FLAG = 0x80000000;

		hash_entry* get_additional_entry(void) {
			add_entries	*cur = add_entries_.load(std::memory_order_relaxed);
			if(cur)
				return cur->get_entry();
			add_entries	*tmp_entry = new  add_entries();
			// then as usual, swap it (note cur == 0)
			// If we can swap, use it, otherwise
			// it means somene else must have added
			// a valid one, then use that one
			if(add_entries_.compare_exchange_strong(cur, tmp_entry)) {
				return tmp_entry->get_entry();
			} else {
				delete tmp_entry;
				return cur->get_entry();
			}
		}
	public:
		struct stats {
			size_t		els_per_bucket[8];
			uint32_t	unused_pairs,
					all_pairs;
		};

		hmap() : entries_(0), data_(0), add_entries_(0) {
			entries_ = new (std::nothrow) hash_entry[Nbuckets];
			if(!entries_)
				throw std::bad_alloc();
			data_ = new (std::nothrow) kv_chunk;
			if(!data_) {
				delete [] entries_;
				throw std::bad_alloc();
			}
		}

		Value* find(const Key& k) const {
			Hasher		hasher;
			const hash_type	cur_hash = hasher(k) | HASH_FLAG;
			const auto&	cur_bucket = entries_[cur_hash%Nbuckets];
			const auto*	cur_el = cur_bucket.find(cur_hash, k, data_);
			if(cur_el) return &data_->pairs[cur_el->load().index].v;
			else return 0;
		}

		bool insert(const Key& k, const Value& v) {
			Hasher		hasher;
			const hash_type	cur_hash = hasher(k) | HASH_FLAG;
			auto&		cur_bucket = entries_[cur_hash%Nbuckets];
			const auto*	cur_el = cur_bucket.insert_once(cur_hash, k, v, data_, [this](){ return this->get_additional_entry(); });
			return cur_el != 0;
		}

		size_t mem_size(void) const {
			return sizeof(*this) + sizeof(hash_entry)*Nbuckets + sizeof(kv_chunk);
		}

		void get_stats(stats& s) const {
			for(int i = 0; i < 8; ++i)
				s.els_per_bucket[i] = 0;
			for(size_t i = 0; i < Nbuckets; ++i) {
				++s.els_per_bucket[entries_[i].size()];
			}
			s.unused_pairs = data_->unused_pairs;
			s.all_pairs = data_->cur_pair;
		}

		~hmap() {
			delete [] entries_;
			delete data_;
			// recursively delete the additional entries
			auto* p_add_entries = add_entries_.load();
			while(p_add_entries) {
				auto*	p_next = p_add_entries->next.load();
				delete p_add_entries;
				p_add_entries = p_next;
			}
		}
	};
}

#endif // _HMAP_


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
			std::atomic<kv*>	cur_pair;
			kv			pairs[Nelems];
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
			std::atomic<hash_index>	entries[MAX_HASH_ENTRIES];
			hash_entry		*next;

			hash_entry() : next(0) {
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
				if(next) return next->find(h, k, data);
				else return 0;
			}

			const std::atomic<hash_index>* insert(const hash_type h, const Key& k, kv_chunk* data, const uint32_t idx) {
				for(int i = 0; i < MAX_HASH_ENTRIES; ++i) {
					auto	e = entries[i].load();
					// if we don't have valid
					// value, that's it, try to
					// write it
					if(!e.hash) {
						hash_index	hi(h, idx);
						if(entries[i].compare_exchange_strong(e, hi))
							return &entries[i];
						// if we can't do it, call again this function...
						// need to keep stack at a minimum...
						return this->insert(h, k, data, idx);
					}
					// otherwise compare if it's
					// the same
					if(e.hash == h) {
						// then compare the key
						// value itself
						if(data->pairs[e.index].k == k) {
							hash_index	hi(h, idx);
							if(entries[i].compare_exchange_strong(e, hi)) {
								// if we have replaced, reset the original
								// key and value...
								data->pairs[e.index].k = Key();
								data->pairs[e.index].v = Value();
								return &entries[i];
							}
							// if we can't do it, call again this function...
							// need to keep stack at a minimum...
							return this->insert(h, k, data, idx);	
						}
					}
				}
				// todo... manage allocation of
				// new bucket list...
				return 0;
			}
		};

		static_assert(sizeof(hash_index) == 8, "hash_index structure has to be 8 bytes");
		static_assert(sizeof(hash_entry) == 64, "hash_entry structre has to be 64 bytes (cacheline)");

		hash_entry	*entries_;
		kv_chunk	*data_;
	public:
		hmap() : entries_(new hash_entry[Nbuckets]), data_(new kv_chunk) {
		}

		Value* find(const Key& k) const {
			Hasher		hasher;
			const hash_type	cur_hash = hasher(k);
			const auto&	cur_bucket = entries_[cur_hash%Nbuckets];
			const auto*	cur_el = cur_bucket.find(cur_hash, k, data_);
			if(cur_el) return &data_->pairs[cur_el->load().index].v;
			else return 0;
		}

		Value* insert(const Key& k, const Value& v) {
			Hasher		hasher;
			const hash_type	cur_hash = hasher(k);
			auto&		cur_bucket = entries_[cur_hash%Nbuckets];
			const auto*	cur_el = cur_bucket.insert(cur_hash, k, data_, 0);
			if(cur_el) return &data_->pairs[cur_el->load().index].v;
			else return 0;
		}

		~hmap() {
			delete [] entries_;
			delete data_;
		}
	};
}

#endif // _HMAP_


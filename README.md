# ehmap
Simple C++11 `lock-free` hash map

Please note this is a template library (released under GPLv3) hence the main itself is simply a test to show the performance/test functionality; the current version compiles on Linux, still it should be easy to adapt to Windows or other architectures.

## Purpose

This is _prototype_ code to create a simple `lock-free` hash-map; the usage of this hash map has to be temporay cache-like, where you usualy write elements only once, you want to be fast, have virtually no memory fragmentation and know already approximately how many `k/v` pairs you will insert (or at least can tune through time).
See [Pros/Cons](#pros-cons) section below.

## Building
Simply invoke `make` (`make release` for optimized build) and the main should compile.

## Running
The main is a simple test for performance/some functionality.

## API

Brief overview how to use this class.

### Instantiation

This class is a template:
``` c++
	template<typename Key, typename Value, typename Hasher,
		size_t Nbuckets = 1024, size_t Nelems = (Nbuckets*8)>
	class hmap
```
|Parameter|Description|
|---------|-----------|
|_Key_      |Key type, has to be copiable and `default` constructable|
|_Value_    |Value type, has to be copiable and `default` constructable|
|_Hasher_   |Hasher type, has to expose ```operator(const Key&)``` returning a ```uint32_t``` hash|
|_Nbuckets_ |Fixed number of buckets - ideally should be expected number of elements plus some contingency|
|_Nelems_   |Internal buffer to store copy of elements - ideally should be expected number of elements plus some contingency if we have clashes when inserting|

### Usage

Please note all these methods are `thread-safe`, in fact are all `lock-free` and to not require holding any `spinlock` nor `accessors`.

|Method|Description|
|------|-----------|
|_Value* **find**(const Key& k) const_|Returns a pointer to the given value or `null` if not found|
|_bool **insert**(const Key& k, const Value& v)_|Inserts a `k` with `v` and returns `true` on success or `false` if the element is already present|
|_size_t **mem_size**(void) const_|Returns the total size the map is using in memory (`this` variable included)|
|_void **get_stats**(stats& s) const_|Returns ```stats``` object with some stats w.r.t. map usage - useful to understand trends and optimize|

## Pros Cons

T.b.d.

## F.A.Q.

T.b.d.


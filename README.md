# ehmap
Simple C++11 `lock-free` hash map

Please note this is a template library (released under GPLv3) hence the main itself is simply a test to show the performance/test functionality; the current version compiles on Linux, still it should be easy to adapt to Windows or other architectures.

## Purpose

This is _prototype_ code to create a simple `lock-free` hash-map; the usage of this hash map has to be temporay cache-like, where you usualy write elements only once, you want to be fast, have virtually no memory fragmentation and know already approximately how many `k/v` pairs you will insert (or at least can tune through time).
The map and its elements will be destroyed/deallocated upon destruction of the map itself.
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

Please note all these methods are `thread-safe`, in fact are all `lock-free` and do not require holding any `spinlock` nor `accessors`.

|Method|Description|
|------|-----------|
|_Value* **find**(const Key& k) const_|Returns a pointer to the given value or `null` if not found|
|_bool **insert**(const Key& k, const Value& v)_|Inserts a `k` with `v` and returns `true` on success or `false` if the element is already present|
|_size_t **mem_size**(void) const_|Returns the total size the map is using in memory (`this` variable included)|
|_void **get_stats**(stats& s) const_|Returns ```stats``` object with some stats w.r.t. map usage - useful to understand trends and optimize|

### Example

Follows simple example how to use it.

``` c++
// simple hasher compatible with requirements
struct my_hasher {
	uint32_t operator()(const int& i) const {
		return static_cast<uint32_t>(i);
	}
};
// useful typedef
typedef ema::hmap<int, double, my_hasher, 32*1024>	MY_MAP;
// create an object
MY_MAP	m;
// manage error if we can't insert (element already present)
if(!m.insert(1, 1.234)) ... 
auto*	v_ptr = m.find(1);
// manage errors if we couldn't find the element
if(!v_ptr) ...
// print out the value
std::cout << "value of 1: " << *v_ptr << std::endl;
// you can modify the values once written, but you are responsible 
// to synchronize access to those
*v_ptr = 2.345;
```

## Pros Cons

* Pros
  1. *Fast* - Adding and looking up should be quite fast, _O(1)_. This requires the _hash_ function to be good (as any other hash map/container).
  2. *Lock-free* - No locks/spinlock/... are present, as long as we don't have too many elements in same bucket (_<=7_) there should be _very little_ contention when inserting elements.
  3. *No memory fragmentation* - This hash map internally allocates _chuncks_ of memory, it all gets _free'd_ once the main object is destroyed
  4. *Buckets fit in 64 bytes* - This ensures each single bucket can fit a single _cacheline_ size on modern CPUs, which means that when writing multiple buckets in parallel there should be no contention.
* Cons
  1. *Insert once only* - You can only insert once given a key. Not good if you want to perform updates of values (you may use a _shared_ptr_ as _Value_ to remedy this).
  2. *Can't iterate through* - Feature not available yet.
  3. *May be memory-heavy when inserting the same `Key` in parallel* - If you have multiple threads trying to insert the same _Key_ in parallel, this may lead to increased memory usage of internal buffers (that's why by default I have set an internal _k/v_ buffer 8 times the number of buckets).
  4. *Number of buckets is fixed* - If you have too many elements (usually _>7_ per bucket), this map may slow down.
  5. *`Key` and `Value` require to be copiable and `default` constructable* - As per description

## F.A.Q.

T.b.d.


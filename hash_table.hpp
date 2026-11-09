#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "associative_array.hpp"

/**
 * An associative array backed by a hash table that uses
 *  - separate chaining (each bucket is a singly linked list), and
 *  - the division method for hashing: index = hash(key) mod m,
 *    where m (the bucket count) is always a prime.
 *
 * The bucket count follows the "good hash table primes" list
 * (https://planetmath.org/goodhashtableprimes), where each prime is roughly
 * double the previous one and far from a power of two. When the load factor
 * (size / buckets) would exceed 1.0, the table grows to the next prime and all
 * nodes are relinked (rehashed) into the new bucket array.
 *
 * Expected running time of set/get/contains/remove is O(1); keyValuePairs is
 * O(n + m).
 *
 * @tparam K        key type
 * @tparam V        value type
 * @tparam Hash     hash functor (defaults to std::hash<K>)
 * @tparam KeyEqual key equality functor (defaults to std::equal_to<K>)
 */
template <typename K, typename V,
          typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>>
class HashTable : public AssociativeArray<K, V> {
public:
    /** Creates an empty table with the smallest prime bucket count. */
    HashTable() : buckets_(kPrimes[0], nullptr) {}

    /** Copy constructor: deep copies every node. */
    HashTable(const HashTable& other)
        : buckets_(other.buckets_.size(), nullptr),
          size_(other.size_),
          primeIndex_(other.primeIndex_),
          hash_(other.hash_),
          eq_(other.eq_) {
        for (std::size_t i = 0; i < other.buckets_.size(); ++i) {
            Node* tail = nullptr;
            for (Node* n = other.buckets_[i]; n != nullptr; n = n->next) {
                Node* copy = new Node{n->key, n->value, nullptr};
                if (tail == nullptr) buckets_[i] = copy;
                else tail->next = copy;
                tail = copy;
            }
        }
    }

    /** Move constructor: steals the other table's storage. */
    HashTable(HashTable&& other) noexcept : HashTable() { swap(other); }

    /** Copy/move assignment via copy-and-swap (by-value parameter). */
    HashTable& operator=(HashTable other) noexcept {
        swap(other);
        return *this;
    }

    ~HashTable() override { clear(); }

    /** Swaps the contents of two tables. */
    void swap(HashTable& other) noexcept {
        std::swap(buckets_, other.buckets_);
        std::swap(size_, other.size_);
        std::swap(primeIndex_, other.primeIndex_);
        std::swap(hash_, other.hash_);
        std::swap(eq_, other.eq_);
    }

    void set(const K& k, const V& v) override {
        Node* existing = find(k);
        if (existing != nullptr) {
            existing->value = v;  // replace the existing mapping
            return;
        }
        // Grow first if adding this element would push load factor above 1.
        if (size_ + 1 > buckets_.size()) rehash();
        std::size_t idx = indexFor(k);
        buckets_[idx] = new Node{k, v, buckets_[idx]};  // insert at chain head
        ++size_;
    }

    bool contains(const K& k) const override { return find(k) != nullptr; }

    std::optional<V> get(const K& k) const override {
        const Node* n = find(k);
        if (n == nullptr) return std::nullopt;
        return n->value;
    }

    bool remove(const K& k) override {
        std::size_t idx = indexFor(k);
        Node* prev = nullptr;
        for (Node* cur = buckets_[idx]; cur != nullptr; prev = cur, cur = cur->next) {
            if (eq_(cur->key, k)) {
                if (prev == nullptr) buckets_[idx] = cur->next;
                else prev->next = cur->next;
                delete cur;
                --size_;
                return true;
            }
        }
        return false;
    }

    std::size_t size() const override { return size_; }

    std::vector<std::pair<K, V>> keyValuePairs() const override {
        std::vector<std::pair<K, V>> result;
        result.reserve(size_);
        for (Node* head : buckets_) {
            for (Node* n = head; n != nullptr; n = n->next) {
                result.emplace_back(n->key, n->value);
            }
        }
        return result;
    }

    /** @return the current number of buckets (always one of the table primes). */
    std::size_t bucketCount() const { return buckets_.size(); }

    /** @return size() / bucketCount(). */
    double loadFactor() const {
        return static_cast<double>(size_) / static_cast<double>(buckets_.size());
    }

private:
    /** A single link in a bucket's chain. */
    struct Node {
        K key;
        V value;
        Node* next;
    };

    /** Good hash table primes (https://planetmath.org/goodhashtableprimes). */
    static constexpr std::array<std::size_t, 26> kPrimes = {
        53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
        196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
        50331653, 100663319, 201326611, 402653189, 805306457, 1610612741};

    std::vector<Node*> buckets_;
    std::size_t size_ = 0;
    std::size_t primeIndex_ = 0;
    Hash hash_;
    KeyEqual eq_;

    /** Division method: hash(k) mod (number of buckets). */
    std::size_t indexFor(const K& k) const { return hash_(k) % buckets_.size(); }

    /** @return the node holding @p k, or nullptr. */
    Node* find(const K& k) const {
        for (Node* n = buckets_[indexFor(k)]; n != nullptr; n = n->next) {
            if (eq_(n->key, k)) return n;
        }
        return nullptr;
    }

    /**
     * Grows to the next prime in the table and relinks every existing node
     * into the new bucket array (no nodes are allocated or copied).
     * If already at the largest prime, the table simply keeps chaining.
     */
    void rehash() {
        if (primeIndex_ + 1 >= kPrimes.size()) return;
        ++primeIndex_;
        std::vector<Node*> newBuckets(kPrimes[primeIndex_], nullptr);
        for (Node* head : buckets_) {
            Node* n = head;
            while (n != nullptr) {
                Node* next = n->next;
                std::size_t idx = hash_(n->key) % newBuckets.size();
                n->next = newBuckets[idx];
                newBuckets[idx] = n;
                n = next;
            }
        }
        buckets_.swap(newBuckets);
    }

    /** Deletes every node (iteratively, so long chains can't overflow the stack). */
    void clear() {
        for (Node*& head : buckets_) {
            while (head != nullptr) {
                Node* next = head->next;
                delete head;
                head = next;
            }
        }
        size_ = 0;
    }
};

#endif  // HASH_TABLE_HPP

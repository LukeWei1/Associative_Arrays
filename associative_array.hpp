#ifndef ASSOCIATIVE_ARRAY_HPP
#define ASSOCIATIVE_ARRAY_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/**
 * Represents a mapping of keys to values.
 * @tparam K the type of the keys
 * @tparam V the type of the values
 */
template <typename K, typename V>
class AssociativeArray {
public:
    virtual ~AssociativeArray() = default;

    /**
     * Insert the mapping from the key, @p k, to the value, @p v.
     * If the key already maps to a value, replace the mapping.
     */
    virtual void set(const K& k, const V& v) = 0;

    /** @return true if @p k is a key in the associative array. */
    virtual bool contains(const K& k) const = 0;

    /**
     * @return the value associated with the key @p k, or std::nullopt
     *         if the key doesn't exist.
     */
    virtual std::optional<V> get(const K& k) const = 0;

    /**
     * Remove the key, @p k, from the associative array.
     * @return true if the item was removed, false if the key was not found.
     */
    virtual bool remove(const K& k) = 0;

    /** @return the number of key/value pairs stored. */
    virtual std::size_t size() const = 0;

    /** @return the full list of key/value pairs (in no particular order). */
    virtual std::vector<std::pair<K, V>> keyValuePairs() const = 0;
};

#endif  // ASSOCIATIVE_ARRAY_HPP

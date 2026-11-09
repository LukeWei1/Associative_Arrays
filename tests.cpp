// Unit tests for HashTable. Build & run:
//   g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined tests.cpp -o tests && ./tests
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

#include "hash_table.hpp"

static int g_checks = 0, g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failures;                                                    \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  "      \
                      << #cond << "\n";                                      \
        }                                                                    \
    } while (0)

#define RUN(test)                                                            \
    do {                                                                     \
        std::cout << "running " #test "\n";                                  \
        test();                                                              \
    } while (0)

/** A deliberately terrible hash: every key collides into one bucket. */
struct ConstantHash {
    std::size_t operator()(int) const { return 42; }
};

void testEmptyTable() {
    HashTable<int, std::string> t;
    CHECK(t.size() == 0);
    CHECK(!t.contains(1));
    CHECK(!t.get(1).has_value());
    CHECK(!t.remove(1));
    CHECK(t.keyValuePairs().empty());
}

void testSetGetContains() {
    HashTable<std::string, int> t;
    t.set("one", 1);
    t.set("two", 2);
    CHECK(t.size() == 2);
    CHECK(t.contains("one"));
    CHECK(t.contains("two"));
    CHECK(!t.contains("three"));
    CHECK(t.get("one").value() == 1);
    CHECK(t.get("two").value() == 2);
    CHECK(!t.get("three").has_value());
}

void testSetReplacesExisting() {
    HashTable<std::string, int> t;
    t.set("a", 1);
    t.set("a", 99);
    CHECK(t.size() == 1);
    CHECK(t.get("a").value() == 99);
    CHECK(t.keyValuePairs().size() == 1);
}

void testRemove() {
    HashTable<int, int> t;
    for (int i = 0; i < 10; ++i) t.set(i, i * i);
    CHECK(t.remove(3));
    CHECK(!t.contains(3));
    CHECK(t.size() == 9);
    CHECK(!t.remove(3));       // already gone
    CHECK(!t.remove(1000));    // never existed
    CHECK(t.get(4).value() == 16);  // others untouched
    t.set(3, 7);               // can re-insert
    CHECK(t.get(3).value() == 7);
    CHECK(t.size() == 10);
}

void testCollisionsWithSeparateChaining() {
    HashTable<int, int, ConstantHash> t;  // everything lands in one chain
    for (int i = 0; i < 100; ++i) t.set(i, i + 1000);
    CHECK(t.size() == 100);
    for (int i = 0; i < 100; ++i) CHECK(t.get(i).value() == i + 1000);

    // Remove head, middle, and tail positions of the chain.
    CHECK(t.remove(99));  // most recently inserted = chain head
    CHECK(t.remove(50));  // middle
    CHECK(t.remove(0));   // oldest = chain tail
    CHECK(t.size() == 97);
    CHECK(!t.contains(99) && !t.contains(50) && !t.contains(0));
    CHECK(t.contains(1) && t.contains(98));

    t.set(42, -1);  // replace within a long chain
    CHECK(t.get(42).value() == -1);
    CHECK(t.size() == 97);
}

void testRehashGrowsAndPreservesData() {
    HashTable<int, int> t;
    std::size_t initialBuckets = t.bucketCount();
    CHECK(initialBuckets == 53);

    const int N = 5000;
    for (int i = 0; i < N; ++i) {
        t.set(i, i * 2);
        CHECK(t.loadFactor() <= 1.0);
    }
    CHECK(t.bucketCount() > initialBuckets);
    CHECK(t.size() == static_cast<std::size_t>(N));
    for (int i = 0; i < N; ++i) {
        CHECK(t.contains(i));
        CHECK(t.get(i).value() == i * 2);
    }
}

void testBucketCountsArePrimesFromList() {
    HashTable<int, int> t;
    const std::size_t expected[] = {53, 97, 193, 389, 769, 1543};
    std::size_t seen = 0;
    std::size_t last = 0;
    for (int i = 0; i < 1500; ++i) {
        t.set(i, i);
        if (t.bucketCount() != last) {
            last = t.bucketCount();
            CHECK(seen < 6 && last == expected[seen]);
            ++seen;
        }
    }
    CHECK(seen == 6);
}

void testKeyValuePairs() {
    HashTable<int, std::string> t;
    for (int i = 0; i < 200; ++i) t.set(i, "v" + std::to_string(i));
    t.remove(10);
    auto pairs = t.keyValuePairs();
    CHECK(pairs.size() == 199);
    std::sort(pairs.begin(), pairs.end());
    for (std::size_t i = 0; i + 1 < pairs.size(); ++i)
        CHECK(pairs[i].first < pairs[i + 1].first);  // no duplicates
    for (const auto& [k, v] : pairs) CHECK(v == "v" + std::to_string(k));
}

void testCopySemantics() {
    HashTable<int, int> a;
    for (int i = 0; i < 300; ++i) a.set(i, i);
    HashTable<int, int> b(a);  // copy construct
    a.set(0, -5);
    a.remove(1);
    CHECK(b.size() == 300);
    CHECK(b.get(0).value() == 0);  // independent of a
    CHECK(b.contains(1));

    HashTable<int, int> c;
    c.set(-1, -1);
    c = b;  // copy assign
    b.set(2, 500);
    CHECK(c.size() == 300);
    CHECK(!c.contains(-1));
    CHECK(c.get(2).value() == 2);

    c = c;  // self assignment is safe
    CHECK(c.size() == 300);
}

void testMoveSemantics() {
    HashTable<int, int> a;
    for (int i = 0; i < 100; ++i) a.set(i, i);
    HashTable<int, int> b(std::move(a));
    CHECK(b.size() == 100);
    CHECK(a.size() == 0);   // moved-from table is valid and empty
    a.set(1, 1);            // ...and still usable
    CHECK(a.get(1).value() == 1);
}

void testPolymorphicUse() {
    AssociativeArray<std::string, int>* arr = new HashTable<std::string, int>();
    arr->set("x", 10);
    CHECK(arr->contains("x"));
    CHECK(arr->get("x").value() == 10);
    delete arr;  // virtual destructor
}

void testRandomizedAgainstStdMap() {
    std::srand(12345);
    HashTable<int, int> t;
    std::map<int, int> ref;
    for (int step = 0; step < 20000; ++step) {
        int key = std::rand() % 1000;
        int op = std::rand() % 3;
        if (op == 0) {
            int val = std::rand();
            t.set(key, val);
            ref[key] = val;
        } else if (op == 1) {
            CHECK(t.remove(key) == (ref.erase(key) == 1));
        } else {
            auto it = ref.find(key);
            auto got = t.get(key);
            CHECK(got.has_value() == (it != ref.end()));
            if (got) CHECK(*got == it->second);
        }
        CHECK(t.size() == ref.size());
    }
}

int main() {
    RUN(testEmptyTable);
    RUN(testSetGetContains);
    RUN(testSetReplacesExisting);
    RUN(testRemove);
    RUN(testCollisionsWithSeparateChaining);
    RUN(testRehashGrowsAndPreservesData);
    RUN(testBucketCountsArePrimesFromList);
    RUN(testKeyValuePairs);
    RUN(testCopySemantics);
    RUN(testMoveSemantics);
    RUN(testPolymorphicUse);
    RUN(testRandomizedAgainstStdMap);

    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}

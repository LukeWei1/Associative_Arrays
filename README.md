**Limpel Ziv Compression**

**Results**
| Input | Original (bytes) | Phrases | Compressed (bytes) | Ratio |
|---|---|---|---|---|
| Small sample text (`sample.txt`) | 90 | 54 | 91 | 1.01 |
| Source code (`code.txt`) | 21,462 | 5,015 | 12,145 | 0.57 |
| English text (`prose.txt`) | 46,507 | 10,138 | 25,836 | 0.56 |

The source code and English text both compressed to about 56-57% of their original size. 
They contain repeated words and patterns, so the dictionary builds up phrases that cover several bytes each. 
The small sample did not compress (ratio 1.01). With only 90 bytes there are few repeated patterns, so it still needs 
to use many unique phrases, and each phrase costs a pointer plus 8 bits. 
This shows that the Limpel Ziv algorithm improves as the input grows, because the dictionary has more phrases to reuse. 
The hash table keeps each dictionary lookup at expected O(1), so encoding time grows roughly linearly with input size.

**How to Run**

**Compile and run tests**
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined tests.cpp -o tests && ./tests
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined lz_tests.cpp -o lz_tests && ./lz_tests

**Compile and run encoder**
g++ -std=c++17 -O2 lz_main.cpp -o lz
./lz <input-file> [packed-output-file] [bit-string-output-file]

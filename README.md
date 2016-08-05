autocomplete_libface
====================

- Description
  - autocomplete c++ library from [cpp-libface](https://github.com/duckduckgo/cpp-libface)
  - modification
    - `remove all dependencies(http-parser,libuv)` 
    - `leave library only`
    - `adjust source tree` 
    - `swtich default algorithm to 'BenderRMQ'`
      - BenderRMQ
        - space cost O(n) (= 8n hidden cost), build cost O(n), query cost O(1) (= 10 hidden cost)
      - SparseTable
        - space cost O(nlogn) (=2nlogn hidden cost), build cost O(nlogn), query cost O(1) (=2 hidden cost)
    - `refactoring for easy use : create~, import~, suggest~`
  - theoretical background
    - [segment tree, rmq and autocomplete](https://github.com/dsindex/blog/wiki/%5Balgorithm%5D-segment-tree,-rmq-and-autocomplete)

- Compilation
  ```bash
  cd src
  make
  make test
  make perf
  ```

- Input format
  ```
  FREQUENCY_COUNT \t PHRASE \t SNIPPET
  10        hello world        
  15        How I met your mother
  21        How do you do?            Not an answer
  9         Great Days ahead          Show in blue background
  ```

- Test
  ```bash
  cd src
  ./test_libface -f data/ac_file.txt
  Successfully added 4/4records from '../data/ac_file.txt' in 0second(s)

  카카오
  [ { "phrase": "카카오", "score": 100, "snippet": "카카오" },
    { "phrase": "카카오톡", "score": 98, "snippet": "카카오톡" },
    { "phrase": "카카오페이지", "score": 90, "snippet": "카카오페이지" },
    { "phrase": "카카오그룹", "score": 80, "snippet": "카카오그룹" }
  ]
  ```

- Dev notes
  - BenderRMQ vs SparseTable
    - input file : 213m, 5464518 record
    - BenderRMQ
      - building time : 17s
      - memory : 659m ( x3 )
      - search time : 1m10s=70s -> 78064 qps
    - SparseTable
      - building time : 18s
      - memory : 938m ( x4.5 )
      - search time : 1m7s=67s -> 81559 qps

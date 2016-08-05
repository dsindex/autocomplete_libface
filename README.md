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
  Successfully added 100000/100000records from '../data/ac_file.txt' in 0second(s)

  [ 
  { "phrase": "다음", "score": 76374, "snippet": "다음" },
  { "phrase": "다음앱", "score": 46771, "snippet": "다음앱" },
  { "phrase": "다음지도", "score": 27387, "snippet": "다음지도" },
  { "phrase": "다음증권", "score": 13817, "snippet": "다음증권" },
  { "phrase": "다음카페", "score": 10480, "snippet": "다음카페" },
  { "phrase": "다음카카오", "score": 9956, "snippet": "다음카카오" },
  { "phrase": "다음웹툰", "score": 4630, "snippet": "다음웹툰" },
  { "phrase": "다음운세", "score": 3566, "snippet": "다음운세" },
  { "phrase": "다음팟", "score": 2529, "snippet": "다음팟" },
  { "phrase": "다음팟플레이어", "score": 2132, "snippet": "다음팟플레이어" },
  { "phrase": "다음증권시세", "score": 1971, "snippet": "다음증권시세" },
  { "phrase": "다음클라우드", "score": 1876, "snippet": "다음클라우드" },
  { "phrase": "다음메일", "score": 1737, "snippet": "다음메일" },
  { "phrase": "다음팟인코더", "score": 1487, "snippet": "다음팟인코더" },
  { "phrase": "다음까페", "score": 1397, "snippet": "다음까페" }
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

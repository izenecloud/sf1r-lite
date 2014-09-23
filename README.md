SF1R-Lite(Search Formula-1 Lite Engine)
=======================================
*A distributed massive data engine for vertical search in C++.*

### Features
* _Flexible configuration_. SF1R could be highly configurable to support either distributed or non-distributed search engine. For Asia languages, different kinds of morphlogical analyzer or dedicated tokenizer could be 
applied as well to be adapted to different situations. Each SF1R instance could be configured to support multiple collections, while the concept of collection could be compared with "Table" in `RDBMS`. Collections could managed totally dynamically without stopping the server instance.
* _Commercially proved_ . SF1R has been fully proved under commercial environments with both complicated situations  and ultra high concurrency. In order to satisfy different kinds of requirements, three kinds of indices are supported within SF1R, including [Lucene](lucene.apache.org) like file based inverted index, pure memory based inverted index with ultra high decompression performance, and succinct self index. This is a practical deployment for a search cloud with both distributed and non-distributed verticals, all of them are behind a single nginx based http reverse proxy to provide unified entry.
![Topology](https://raw.githubusercontent.com/izenecloud/sf1r/master/docs/source/images/sf1r.png)

* _Mining components extendable_. In the early stage of SF1R, there are tens of mining components attached, such as `duplicate detection`,`taxonomy generation`, `query recommendation`, `collaborative filtering`,...,etc. To keep the repository as lite as possible, we made some refinements to remove most mining components. However, the architecture of SF1R has guaranteed the flexibility to introduce any of them, actually, one of index---succinct self index, it was encapsulated using mining component for conveniences. 


### Documents
The Chinese documents could be accessed [here](http://izenecloud.github.io/sf1r/), while we also prepared the English [technical report](https://github.com/izenecloud/sf1r-lite/raw/master/docs/pdf/sf1r-tr.pdf).

### Dependencies
We've just switched to `C++ 11` for SF1R recently, and `GCC 4.8` is required to build SF1R correspondingly. You also need `CMake` and `Boost 1.56` to build the repository .Here are the dependent repositories list:

* __[cmake](https://github.com/izenecloud/cmake)__: The cmake modules required to build all iZENECloud C++ projects.


* __[izenelib](https://github.com/izenecloud/izenelib)__: The general purpose C++ libraries.


*  __[icma](https://github.com/izenecloud/icma)__: The Chinese morphological analyzer library.
  

*  __[ijma](https://github.com/izenecloud/ijma)__: The Japanese morphological analyzer library.


*  __[ilplib](https://github.com/izenecloud/ilplib)__: The language processing libraries.
  

*  __[idmlib](https://github.com/izenecloud/idmlib)__: The data mining libraries.



### Usage
To use SF1R, you should have configuration files located in the `config` directory. After that:
```bash
$ cd bin
$ CobraProcess -F config
```
Please see the [documents](http://izenecloud.github.io/sf1r/) for further usage.

### License
The SF1R project is published under the Apache License, Version 2.0:
http://www.apache.org/licenses/LICENSE-2.0

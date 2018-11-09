# Algolia Applicants Technical Assignment

## Build and launch the solution

```
mkdir build; cd build; cmake ..; make
./hnStat <distinct|top [nb_top_queries]> [--from T1] [--to T2] <input_file>
```

## Theorical questions

### About the *distinct* command:

#### Time complexity

Linear of the number of queries in the input file

#### Memory complexity

Linear of the number of distinct queries

### About the *top* command:

#### Time complexity

Let n denote the number of queries in the input file 

Let k denote the number of top queries desired

Let m denote the number of distinct queries

First we insert each request in an unordered_map: O(n)

For each request in the unoredered_map (O(m)), we insert it in a set if it has 
been encountered at least one more time than any value of this set. The insertion is
O(log(k)). We remove the worse value of the set if the set size is larger than k (O(log(k))).
If the insertion is not always performed, the removal is almost always performed. Checking if the 
value should be inserted or removed is O(1). So we can roughly say that the insertion + removal is
O(log(k)).

In summary:
O(n + m * log(k))

#### Memory complexity

O(m + k)

## Personal notes

The comments in the code are here to help the comprehension, I did not try to
build a sort of documentation (doxygen etc.)

I tried to mix a maximum of my knowledge of c++ for this assignment, but I could have done more :)
I especially tried to provide a convenient transform/filter input iterator interface
for streaming the requests, that you can easily stack together to form a preprocessing pipeline. This kind of iterator is nicely implemented in boost, but I
did not try to look at their code.

- Inputs are traversed one time to perform the from/to checks.
- Functional programming is used when it is relevant or efficient.
- Template programming is almost readable.
- I tried to avoid copying the requests text, using move semantics when possible.
- The implementation is not multi-threaded, but the processing pipeline could be adapted easily.

## Issues

- Using the optimisation flags -Ox of g++ or clang++ compilers breaks the solution : 
the first or last request of the file is messed-up with this flag. This bug is really strange, and I did not 
have the time to investigate a bug that can come from the compiler.
- Separating the code into different files had a great cost in terms of performance. 
I guess it may be due to the functions marked inline that can't be inlined by the
compiler when they are located in different compilation objects. I'm not sure though. However, this file is only 350 lines of
code so it is still ok in terms of coding style.
- I do not like to parse arguments myself, I usually prefer using boost.
This part of the code is ugly, and I'm sorry :/

#Thank you for your time!
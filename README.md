stable_vector\<T\>
==================
A STL-compliant vector container, offering reference stability. 

It might be seen identical to *boost::stable_vector*, although both are very different. What can we say about *stable_vector*?

  * is it using contiguous storage, like *std::vector* &mdash; and this makes it cache friendly
  * elements are being constructed in this storage itself, without extra memory allocations &mdash; this is a big difference with *boost::stable_vector* which is implemented as a node container
  * it does not support deletions &mdash; there is no magic ; you cannot live with infinite continuous storage and no reference invalidations if elements can be removed
  * last, but not least: when growing, the growing cost is predictible and constant, which is not the case for *std::vector*


Example
=======
Basic usage:
```c++
    stable_vector<A> va;
    va.emplace_back();
    
    A& a = va.back();
    
    // client code can safely store the reference
    Notify(a);
```


Here, we are using the second template parameter (that defaults to 4096) to change the size of the chunk used as underlying storage:
```c++
    stable_vector<A, 1024> va;
    std::cout << va.capacity(); // prints 1024
``` 


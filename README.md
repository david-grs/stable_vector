stable_vector\<T\>
==================
A STL-compliant vector container, offering reference stability. 

It might be seen identical to *boost::stable_vector\<T\>*, although both are very different:

  * *stable_vector\<T\>* is not implemented as a node container: insertions don't do any extra memory allocation
  * *stable_vector\<T\>* it using contiguous storage, which makes it nice for cache locality &mdash; the underlying contiguous storage's size can be defined through template parameter
  * *stable_vector\<T\>* does not support deletions



Example
=======
```c++
    stable_vector<A> va;
    va.emplace_back();
    
    A& a = va.back();
    
    // client code can safely store the reference
    Notify(a);
```


```c++
    stable_vector<A, 1024> va;
    std::cout << va.capacity(); // prints 1024
``` 


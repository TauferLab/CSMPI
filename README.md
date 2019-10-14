# A PMPI module for tracing call-stacks

## Summary:
CSMPI is a PMPI module for tracing call-stacks of MPI functions. For instance, 
you may want to know which chains of function calls in your application end in
calls to certain MPI functions; CSMPI can help you find out!

## Dependencies:
* JSON for Modern C++ (https://github.com/nlohmann/json)
* Boost (Serialization, MPI)
* pyelftools (https://github.com/eliben/pyelftools) (Optional, used for analysis scripts not CSMPI itself)

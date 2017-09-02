# Multithreading

The thread should allow multiple read locks but only serve single write lock, on its own. If there are read locks and write lock has been requested, it will be put on hold. After serving all the read locks, write lock will be served. A blocked writer should block later readers

```
make
lab4 [testcase numbner]
or 
lab4 1
lab4 10
```
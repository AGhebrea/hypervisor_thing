# Goal:
Something like explained in [this lwn article](https://lwn.net/Articles/194543/)  
The goal is to write a HOST program, and a GUEST program.  
  
- They will both share an interface, "struct paravirt_ops",
- HOST is the entry point.
- HOST loads GUEST into memory.
- HOST transfers control to GUEST
- GUEST fills "struct paravirt_ops" with relevant functions.
- GUEST does it's thing.
  
WHEN GUEST does a "syscall" the HOST runs the GUEST defined function if that is the case.  
The idea is to simulate that interaction between a hypervisor and an OS as described in the article.  

# Arbitrary restrictions:
- No execve or variants.
- No ptrace.
- We must kind of do the bare minimum that a "loader" does.

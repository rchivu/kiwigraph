~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~ Low Level Concepts ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is a document that outlines the low level concepts used in the kwgraph 
library comments. 

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Branch: For the scope of this document, a branch is the code path taken when 
reaching an if statement.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Branch prediction: As explained in the processor pipeline section, CPUs
usually request the contents of variables from memory ahead of time. However
when you hit a branch and don't know exactly which code path it will become.
Instead of waiting for the condition variable to become available, processors
try to guess what the branch will be. Depending on the processor this can be
either a sort of an educated guess based on the history of the branch or a blind
guess that always takes either the always true or the always false path

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Pointer aliasing: Term that describes two pointers from the same scope that
reference the same data

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Processor pipeline: To improve program efficiency, processors generally call 
variable contents into memory before they are actually needed. The size of the
pipeline tells you how many items it can request from memory ahead of time.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

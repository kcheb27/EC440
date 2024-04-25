Hello this is the Read me for My user-level thread scheduler.
the following code utilized the depreciated Ularam for its alarm function. Sytax for it was quite simple and its signal handeler's syntax was found on https://pubs.opengroup.org/onlinepubs/009695399/basedefs/signal.h.html.

The Thread level scheduler manages to read in start routines with different arguments and is able to context switch with an indefinite amount of them using pthread create to initialize the threads and pthread join to wait for them. using pthread exit you can also set an exit value that will be returned to you in pthread join. additionally pthread.id will give you the thread id chronilogically based on which thread it was in its creation timeline. 

Basic Makefile was taken from https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/



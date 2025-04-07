# HTTPProxyClientSide
Implemented a basic HTTP Proxy as a socket programming project, processing URL requests and local  file saving.

Remarks:
1.  After compiling, the input for the program should follow this format: "./cproxy <URL> <-S>".
    When <URL> represents the URL ("http://www.example.com/" for example) and <-S> represents the
    absence/existence of the '-s' flag (type '-s' to use the flag or omit it to not use it).
2.  '-s' flag will cause the program to open the URL in the default web browser, after saving it to local files/printing
    its content.
3.  Running the program for URL 'x' for the first time, will save 'x' data in local files, print its content and length.
    Running the program for URL 'x' for the second time, will print 'x' data from saved local files.
4.  Giving the program wrong input will cause the program to exit, printing the function/reason for exiting.

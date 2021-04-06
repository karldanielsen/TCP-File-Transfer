OVERVIEW:

On a high level, my server and client set up sockets, connect those sockets,
and then alternatingly call recv() and send(), so that each message sent down
the pipe must be acknowledged before another is sent. This allows large files
to transfer in sections safely.

PROBLEMS ENCOUNTERED:

There seemed to be a lack of common knowledge about a lot of the TCP socket
libraries online, and most my of time was spent reading man pages rather than
stack overflow links (which are what helps with most projects). I encountered
two major problems while implementing my server:

1. c++'s read() function just returns raw data. There is no '\0' or anything
else to guide other parts of the program and help them understand what to do
with this data. As a result I made heavy use of c++'s stringstream libraries.
Sadly, trying to turn a binary file into an array of characters causes a
number of problems, and my program finally worked when I stopped trying to
micromanage and just allowed the raw buffer to be passed and parsed out
server-side.

2. connect() has no native timeout functionality. Eventually I solved this
by making the socket non-blocking and polling for a write pipe after connect
fails and begins to run asynchronously.


LIBRARIES USED:

poll
fstream
csignal
netdb
fcntl

ONLINE RESOURCES:

Other than some stray one-or-two-line stack overflow links I found with Google, the
only resource I used intimately was an online CRC64 library, which provided
clearer code for how to implement CRC64 than other pseudocode online. Hopefully it
works, it is viewable here:

https://github.com/f4exb/sdrdaemon/blob/master/sdmnbase/CRC64.cpp

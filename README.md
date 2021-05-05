Gabriel Corella
Computer Networks: Programming Assignment 2
 
Goal: In Project 1, you wrote a client and server to transfer a file over TCP. Since TCP provides reliable data transfer,
that was a relatively simple task. Now, in Project 2, you will transfer a file over UDP. Since UDP is unreliable and
can lose packets, you must implement reliable data transfer functionality into your client and server. The protocol
that you will implement to provide this functionality is the Stop-and-Wait Protocol.

At specific places in both your client and server programs, you must print out specific messages. The symbol ”n”
below refers to the sequence number of the transmitted or received packet (note that the sequence number must
always be either 0 or 1), and the symbol ”c” below refers to the count (number of data bytes) in the transmitted or
received packet.

–  List of relevant files:
        server.c
        client.c
        loremipsum.txt
        recv.txt
        Makefile
        README 

–  Compilation instructions:
        Compile using the make file, use make all to build both client and server, or 'make client' / 'make server'.
-  Configuration file(s): 

–  Running instructions: 
        Use the makefile to build the project. 'Make all' creates both client and server files, all you need now is to 
        split terminals and run the server first, followed by the client with the necessarie command line arguments. 

        Server:
            ./server n m 
            Where n is the timeout value of 1-10 and stored as 10^n
            Where m is the packet loss ratio between 0 and 1 (ex. .4)

        Client: 
            ./client s x
            Where s is the String file name within the directory 
            Where x is the ACK loss Ratio


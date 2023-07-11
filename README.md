# Multithreaded-HTTP-client

Implement a simple HTTP client. It accepts one command line parameter - URL. The client makes a request at the specified URL and outputs the response body to the terminal as text (i.e. if the response is HTML, it prints the original text without formatting). Output is produced as data is received from the HTTP connection. When more than a screen (more than 25 lines) of data has been output, the client should continue receiving data, but should stop output and issue a Press space to scroll down prompt.

When pressed by the user, the client should output the next screen of data. To read data from the terminal and the network connection at the same time, use the select system call.

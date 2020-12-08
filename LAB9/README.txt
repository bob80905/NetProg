Test 2 Google Test - Review Your Output     ./a.out 108.177.112.106
Defaults: SO_RCVBUF = 87380, MSS = 536
After connect: SO_RCVBUF = 374400, MSS = 1380


Test 3 RPI Test - Review Your Output     ./a.out 128.113.0.2
Defaults: SO_RCVBUF = 87380, MSS = 536
After connect: SO_RCVBUF = 374400, MSS = 1460

Test 4 localhost Test - Review Your Output     ./a.out 127.0.0.1
Defaults: SO_RCVBUF = 87380, MSS = 536
After connect: SO_RCVBUF = 1062000, MSS = 21845

The MSS value changed after connecting to the different addresses because 
it is a receiver specified value. SO_RCVBUF was the same for both remote tests,
since it is the buffer size the kernal allows for the socket. 

We suspect that both the MSS and SO_RCVBUF sizes were allowed to be much 
greater for the localhost test because we can easily send much more locally 
than remotely, especially when we don't need to worry so much about a lot arriving
unexpectedly at once and overflowing the buffer.
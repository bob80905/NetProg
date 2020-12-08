Submit a single README.txt file that states the MSS and receive window values before and after connecting for each case, and explain why these values did or didn’t change, and why they might be different for the different IP addresses that were tested. If you get different results on your local machine, feel free to include them - particularly depending on your operating system you may see different results.

Test 2 Google Test - Review Your Output     ./a.out 108.177.112.106
Defaults: SO_RCVBUF = 87380, MSS = 536
After connect: SO_RCVBUF = 374400, MSS = 1380


Test 3 RPI Test - Review Your Output     ./a.out 128.113.0.2
Defaults: SO_RCVBUF = 87380, MSS = 536
After connect: SO_RCVBUF = 374400, MSS = 1460

Test 4 localhost Test - Review Your Output     ./a.out 127.0.0.1
1Defaults: SO_RCVBUF = 87380, MSS = 536
2After connect: SO_RCVBUF = 1062000, MSS = 21845
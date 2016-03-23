1. Execution
	Type $make to generate 4 executable files named bidding_system, host, player and player_bonus. Then you can follow the spec to run the program. That is, $./bidding_system [host_num] [player_num].

2. Description
	In bidding_system.c, fork [host_num] children to execute host. Before fork(), it is necessary to create pipes so that parent and child can communicate. After forking enough children, using select() to deal with I/O between parent and children. In each writing, parent writes one unused combination in C(player_num, 4) into pipe which is ready to write. In each reading, parent reads the data from pipe and processes it. After C(player_num, 4) combinations have already used, parent sends end signal and close the pipe. Finally, bidding_system calculates the ranks and print them on stdout. Before bidding_system ends, wait [host_num] chilren so that zombie process won't be generated.

	I think the toughest part in host.c is to deal with reading data from host[host_id].FIFO. I first try to read it after four players write their annoucement into host[host_id].FIFO, but this method failed(I think it is my problem). Hence, I change the method in which host read data from host[host_id].FIFO as long as one of the players annouces his price. It succeeds, but there are still some problems. I print the data which is read from FIFO on stderr, and there is gibberish in it. I think there may be some problems when the player writes data into FIFO(I use sprintf() and write()). After ten rounds, host calculate the ranks and send them to bidding_system

	In player.c, it's the easiest part of this homework since the money each player annouces is regular.
	In player_bonus.c, the algorithm I use is that if the player can certainly win this round, he announces the least money to win it. Otherwise, annouce all his money.

3. Self-Examination
	1, 2, 3, 4, 5, 6 and 7(and maybe bonus). I implement bidding_system.c, host.c, player.c(snd player_bonus.c). Once I finish one of them, I execute it with others TA's program and check whether the output is correct. After I finish all of them, I also execute all my program and compare my output and TA's output. The brief desciption of my program has been mentioned above.
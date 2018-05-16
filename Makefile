compiler	=gcc
compflags	=-Wall -pthread
path		=bin

default : checkdir $(path)/server  $(path)/secondClient  $(path)/firstClient
checkdir :
	@if [ ! -d $(path) ]; then mkdir $(path); fi


$(path)/firstClient: 
	$(compiler) $(compflags) firstClient.c -o $(path)/firstClient 

$(path)/server : main.o
	$(compiler) $(compflags) main.c -o $(path)/server 

$(path)/secondClient: 
	$(compiler) $(compflags) secondClient.c -o $(path)/secondClient 

clean :
	rm -f $(path)/*
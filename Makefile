# Compilation flags
CFLAGS = -Wall -g

# Source files
MANAGER_SRCS = nfs_manager.c log_funcs.c commands.c  
MANAGER_OBJS = $(MANAGER_SRCS:.c=.o)

CONSOLE_SRCS = nfs_console.c log_funcs.c   
CONSOLE_OBJS = $(CONSOLE_SRCS:.c=.o)

# Targets
all: nfs_console nfs_manager nfs_client

nfs_console: $(CONSOLE_SRCS)
	gcc $(CFLAGS) -o nfs_console $(CONSOLE_SRCS)

nfs_manager: $(MANAGER_SRCS)
	gcc $(CFLAGS) -o nfs_manager $(MANAGER_SRCS)

nfs_client: nfs_client.c
	gcc $(CFLAGS) -o nfs_client nfs_client.c

# Run targets
run_man:
	./nfs_manager -l manager_logfile -c config_file.txt -n 2 -p 4321 -b 1024

run_cons:
	./nfs_console -l console_logfile -h 127.0.0.1 -p 4321

run_client:
	./nfs_client -p 1234

# Clean targets
clean:
	rm -f *.o nfs_manager nfs_console nfs_client
	rm -f nfs_manager nfs_console nfs_client console_logfile manager_logfile
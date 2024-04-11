CC = gcc
CFLAGS = -Wall -Wextra -g

SRCDIR = src
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BINDIR)/%.o, $(SOURCES))
EXECUTABLE = sim

LOGFILE = "switch.log"

all: $(EXECUTABLE)
	./$(BINDIR)/$(EXECUTABLE) $(LOGFILE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(BINDIR)/$@

$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BINDIR)/*.o $(BINDIR)/$(EXECUTABLE)
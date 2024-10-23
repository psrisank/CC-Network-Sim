CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g

SRCDIR = src
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BINDIR)/%.o, $(SOURCES))
EXECUTABLE = sim

ARTIFACTDIR = artifacts
INPUTFILE = output_trace.csv
MEMFILE = meminit.csv
LOGFILE = switchlog.csv

all: clean $(EXECUTABLE)
	@./$(BINDIR)/$(EXECUTABLE) $(ARTIFACTDIR)/$(INPUTFILE) $(ARTIFACTDIR)/$(MEMFILE) $(ARTIFACTDIR)/$(LOGFILE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(BINDIR)/$@

$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BINDIR)/*.o $(BINDIR)/$(EXECUTABLE)
	rm -f *.csv
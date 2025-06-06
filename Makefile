TARGET1	= cl
TARGET2	= sv
CFLAGS = -Wall -g

all: $(TARGET1) $(TARGET2)

$(TARGET1): cl.c
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET2): sv.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET1) $(TARGET2) *~ pipe *.log

test: $(TARGET)
	./test.sh
	



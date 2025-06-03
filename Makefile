TARGET1	= rlog
TARGET2	= sv
CFLAGS = -Wall -g

all: $(TARGET1) $(TARGET2)

$(TARGET1): rlog.c
	$(CC) -o $@ $<

$(TARGET2): sv.c
	$(CC) -o $@ $<

clean:
	rm -f $(TARGET1) $(TARGET2) *~ pipe

test: $(TARGET)
	./test_rlog.sh
	



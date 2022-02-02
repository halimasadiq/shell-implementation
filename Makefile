OUTPUT = shell
CFLAGS = -g -Wall -Wvla -fsanitize=address

%: %.c
	gcc $(CFLAGS) -o $@ $^

all: $(OUTPUT)

clean:
	rm -f $(OUTPUT) *.tmp

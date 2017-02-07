# Define values required for running make
C-SRCS := $(shell find $(CURDIR) -type f \( -name "*.c" \))
C-OBJS := $(addsuffix .o, $(basename $(C-SRCS)))

# Phonies
.PHONY: all clean install
all: imgtool

clean:
	@rm -vf src/*.o
	@rm -vf imgtool

install: imgtool
	mv $< /usr/local/bin/$<


# Build Rules
imgtool: $(C-OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -o $@ -c $<

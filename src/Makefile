CODEFILES=irdc4cube.c parseconf.c scan.c
HEADERFILES=parseconf.h irdctype.h scan.h

all: irdc4cube

irdc4cube: $(CODEFILES) $(HEADERFILES)
	gcc -o irdc4cube $(CODEFILES) $(HEADERFILES)

clean:
	rm irdc4cube

.PHONY: clean

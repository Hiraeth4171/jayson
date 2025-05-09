objects = obj/json.o obj/util.o obj/logging.o

#erm what the flip
setup:
	mkdir -p obj bin bin/shared
	@echo " - you can edit the flags in the makefile to remove the debug build stuff"

build: FLAGS = -ggdb
build: build-lib build-test

build-objects:
	gcc -c $(FLAGS) src/json.c -o obj/json.o
	gcc -c $(FLAGS) src/util.c -o obj/util.o
	gcc -c $(FLAGS) src/logging.c -o obj/logging.o

build-lib: build-objects
	ar rcs bin/libjayson.a $(objects)

# example static library build
build-test:
	gcc $(FLAGS) -o test test.c -L./bin -ljayson -I./src/include/

shared: FLAGS = -fPIC -ggdb
shared: build-objects shared-lib install shared-test

shared-lib:
		gcc -shared $(objects) -o bin/shared/libjayson.so

# example shared library build
shared-test:
	gcc shared-test.c -ljayson -o shared-test -I./src/include/

# install shared library build | you might wanna change these to be less system-wide
install: 
	sudo cp bin/shared/libjayson.so /usr/lib
	sudo cp -r src/include/jayson/ /usr/lib/include/jayson/
	sudo chmod 755 /usr/lib/include/jayson/
	sudo chmod 755 /usr/lib/libjayson.so

valgrind-clean:
	rm -f vgcore.*

clean:
	rm -rf obj bin

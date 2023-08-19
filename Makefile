# define CPPFLAGS=-I... for other (system) includes
# define LDFLAGS=-L... for other (system) libs to link

CC = g++ -g -Wno-float-conversion -Wno-narrowing -Wreturn-type -Wunused-function -Wreorder -Wunused-variable

CC_DEBUG = @$(CC) -std=c++11
CC_RELEASE = @$(CC) -std=c++11 -O3 -DNDEBUG

G_DEPS = $(wildcard *.cpp *.h apps/* src/* include/*)

G_SRC = $(wildcard src/*.cpp *.cpp)

G_INC = $(CPPFLAGS)

G_LINK = $(LDFLAGS)

all: image

image : $(G_DEPS)
	$(CC_DEBUG) $(G_INC) $(G_SRC) apps/main_image.cpp apps/image.cpp apps/image_recs.cpp -o image

clean:
	@rm -rf image tests bench dbench draw pa?_*.png final_*.png something.png *.dSYM


CPPFLAGS = -Wall -O3 -std=c++14 -lm -w
PROGRAMS = main 

all: $(PROGRAMS)

main:main.cpp \
	BOBHASH32.h BOBHASH64.h CuckooCounter.h heavykeeper.h params.h spacesaving.h ssummary.h BubbleSketch.h LossyStrategy.h Uss.h
	g++ -o BubbleSketch main.cpp $(CPPFLAGS)

clean:
	rm -f *.o $(PROGRAMS)

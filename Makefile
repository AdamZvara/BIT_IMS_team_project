###############################
# IMS project
# Autori: Tomas Matus, Adam Zvara
# Login: xmatus37, xzvara01
###############################

CXX=g++
CXXFLAGS=--std=c++11 -g -Wall -Wextra
LDLIBS=-lsimlib
exec=panama
login=xmatus37
NAME=manual
SHELL=/usr/bin/env bash
FILES=*.cpp *.hpp Makefile manual.pdf README.md

all: $(exec)

run: $(exec)
	./panama -v
	cat ./simulation.out

1: $(exec)
	./panama -e 1
	cat ./simulation.out

2: $(exec)
	./panama -e 2
	cat ./simulation.out

pdf:
	pdflatex $(NAME)
	pdflatex $(NAME)

#### main
$(exec): ims_project.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

#### Object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^

#### MISC
clean:
	rm -f *.o $(exec)
	rm -f $(NAME).{aux,out,dvi,ps,log,te~,bcf,xml}
	rm -f simulation.out

tar:
	tar -cf $(login).tar $(FILES)

zip:
	zip $(login).zip $(FILES)

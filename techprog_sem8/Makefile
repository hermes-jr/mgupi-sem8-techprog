all:: clean main

main: main.cpp
	g++ /System/Library/Frameworks/GLUT.framework/GLUT /System/Library/Frameworks/OpenGL.framework/OpenGL -o $@ $?

clean::
	rm -f main

.PHONY: clean

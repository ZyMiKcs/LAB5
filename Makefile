all :
	gcc `pkg-config --cflags gtk+-3.0` -o build/release main.c `pkg-config --libs gtk+-3.0` -lm
clean : 
	rm build/release
rebuild : clean all
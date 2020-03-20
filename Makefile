build:
	gcc -Wall -lX11 -ldl -m32 main.c -fPIC -shared -o libFakeX11.so

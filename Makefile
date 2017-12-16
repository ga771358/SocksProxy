all exec: proxy cgi
proxy: proxy.cpp config.h
	g++ proxy.cpp -o proxy
cgi:

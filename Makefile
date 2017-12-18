all exec: proxy cgi
proxy: proxy.cpp config.h
	g++ proxy.cpp -o proxy
cgi: cgi.cpp config.h
	g++ cgi.cpp -o hw4.cgi

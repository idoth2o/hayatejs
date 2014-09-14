# hayate.js  
Next generation web Infrastructure.

## Prerequisites

- [gcc4.7 or later][libv8][libevent2]

## Feature
* High performance
	- Support multithread enviroment.

* simplicity
 	- Internal Web server
  	- Support JavaScript

## Support
	OSX 10.9
	Linux(Not Testing)
	Windows(T.B.D)

## Usage
	Run app:
 	
	$ ./hayate 8080 4 web.js

	Connect web app:

	$ curl http://127.0.0.1:8080/hello
	hello World
	
## Status
	Experimental stage.
	only support GET Method.
	
## License
	GPL v2 or Proprietary(T.B.D)
	
## Reference
reference :	https://developers.google.com/v8/get_started

goog code :	http://kukuruku.co/hub/cpp/lightweight-http-server-in-less-than-40-lines-on-libevent-and-c-11

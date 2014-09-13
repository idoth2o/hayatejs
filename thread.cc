#include <iostream>
#include <thread>
#include "v8.h"
#include <evhttp.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

using namespace v8;

int httpserver_bindsocket(int port, int backlog) {
  int r;
  int nfd;
  nfd = socket(AF_INET, SOCK_STREAM, 0);
  if (nfd < 0) return -1;
 
  int one = 1;
  r = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
 
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
 
  r = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
  if (r < 0) return -1;
  r = listen(nfd, backlog);
  if (r < 0) return -1;
 
  int flags;
  if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
      || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
    return -1;
 
  return nfd;
}

static void run(struct evhttp_request *req,void *obj){
         struct evbuffer *OutBuf = evhttp_request_get_output_buffer(req);
         evbuffer_add_printf(OutBuf, "<html><body><center><h1>Hello Wotld!</h1></center></body></html>");
         //evbuffer_add_printf(OutBuf,content);
         evhttp_send_reply(req, HTTP_OK, "", OutBuf);
}

class Worker{

	struct event_base *base;
    struct evhttp *httpd;
        
    Isolate* isolate;
    Local<Context>  context;
    Local<String> source;
    Local<Script> script;

#if 0
static void inv(struct evhttp_request *req, void *obj){
	work *myClass = reinterpret_cast<work *>(obj);
    myClass->run(req);
    return;
}

#endif
public:
void init(int nfd) {
#if 0
    struct event_base *base;
	struct evhttp *httpd;
#endif
	//++num;
	base = event_init();
	httpd = evhttp_new(base);
	evhttp_accept_socket(httpd, nfd);
	
#if 0	
	Isolate* isolate;
	Local<Context>  context;
	Local<String> source;
	Local<Script> script;
#endif
	    isolate = Isolate::New();
    	//Locker lock(isolate);
    	Isolate::Scope isolate_scope(isolate);

    	// Create a stack-allocated handle scope.
    	HandleScope handle_scope(isolate);

    	// Create a new context.
    	context = Context::New(isolate);

    	// Enter the context for compiling and running the hello world script.
    	Context::Scope context_scope(context);

    	//source = String::NewFromUtf8(isolate,"'Hello' + ', World:' + getUrl();");
    	source = String::NewFromUtf8(isolate,"'Hello' + ', World:'");

    	// Compile the source code.

    	script = Script::Compile(source);
    	Local<Value> result = script->Run();
		String::Utf8Value utf8(result);
		std::cout << *utf8 << std::endl;

		evhttp_set_gencb(httpd,inv,this);
		event_base_dispatch(base);
	}
	static void inv(struct evhttp_request *req, void *obj){
		((Worker*)obj)->run(req);
	}
	void run(struct evhttp_request *req){
		Local<Value> result = script->Run();
		String::Utf8Value utf8(result);
		std::cout << *utf8 << std::endl;
		
		struct evbuffer *OutBuf = evhttp_request_get_output_buffer(req);
        evbuffer_add_printf(OutBuf, "<html><body><center><h1>Hello Wotld!</h1></center></body></html>");
    	//evbuffer_add_printf(OutBuf,content);
    	evhttp_send_reply(req, HTTP_OK, "", OutBuf);
    	
	}	
};

void lunch(int nfd){
	Worker *worker = new Worker();
	worker->init(nfd);
};

int main() {
	//work worker;
	int nfd = httpserver_bindsocket(8080,1024);
    int num = 100;
    std::thread th1(lunch,nfd);
    num++;
    //std::thread th2(worker,nfd);
	th1.join();
	//th2.join();

    std::cout << num << std::endl;

    return 0;
}

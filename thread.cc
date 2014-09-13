#include <iostream>
#include <fstream>
#include <thread>
#include "v8.h"
#include <stdio.h>
#include <string.h>
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

__thread char *req_global;

void getUrl(const v8::FunctionCallbackInfo<v8::Value>& info) {
	//info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),"/start"));
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),req_global));
	//return v8::Undefined();
}

class Worker{

	struct event_base *base;
    struct evhttp *httpd;
    const char *req_path;
    char *jsScript;
        
    Isolate* isolate;
    Local<Context>  context;
    Local<String> source;
    Local<Script> script;
	Local<ObjectTemplate> getUrlObj ;
	Local<Context> getUrlCtx;

public:
	int loadScript(){
	    std::ifstream ifs("test.js");
    	if (ifs.fail())
    	{
        	std::cerr << "faild" << std::endl;
        	return EXIT_FAILURE;
    	}
    	ifs.seekg(0, ifs.end);
    	int size = static_cast<int>(ifs.tellg());
    	ifs.seekg(0, ifs.beg);
    	jsScript = new char[size];
    	jsScript[size - 1] = '\0';
    	ifs.read(jsScript, size);
    	std::cout << "[" << jsScript << "]" << std::endl;
    	ifs.close();
    	return EXIT_SUCCESS;
	}
void init(int nfd) {
	char *str = "/start";
	req_global = new char[255];
	strcpy(req_global, str); 
	
	//++num;
	base = event_init();
	httpd = evhttp_new(base);
	evhttp_accept_socket(httpd, nfd);
	
	loadScript();
	
	    isolate = Isolate::New();
    	//Locker lock(isolate);
    	Isolate::Scope isolate_scope(isolate);

    	// Create a stack-allocated handle scope.
    	HandleScope handle_scope(isolate);

    	// Create a new context.
    	context = Context::New(isolate);

    	// Enter the context for compiling and running the hello world script.
    	Context::Scope context_scope(context);

	    getUrlObj= ObjectTemplate::New(isolate);
    	getUrlObj->Set(String::NewFromUtf8(isolate,("getUrl")), FunctionTemplate::New(isolate, getUrl));
    	getUrlCtx = Context::New(isolate, nullptr, getUrlObj);
    	Context::Scope context_scope2(getUrlCtx);

    	//source = String::NewFromUtf8(isolate,"'Hello' + ', World:' + getUrl();");
    	//source = String::NewFromUtf8(isolate,"'Hello' + ', World:'+ getUrl();");
		if(jsScript != NULL)
			source = String::NewFromUtf8(isolate,jsScript);
		else
			source = String::NewFromUtf8(isolate,"'Welcom' + ', World:'+ getUrl();");
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
		
		
		//std::cout << *utf8 <<   __PRETTY_FUNCTION__ << pthread_self() << std::endl;
		
		req_path = evhttp_request_uri(req);
    	strcpy(req_global, req_path); 
    	printf("ACCESS:%s\n",req_global);
		
		Local<Value> result = script->Run();
		String::Utf8Value utf8(result);
		char *content = new char[255];
    	sprintf(content,"%s\n", *utf8);
		
		struct evbuffer *OutBuf = evhttp_request_get_output_buffer(req);
        //evbuffer_add_printf(OutBuf, "<html><body><center><h1>Hello Wotld!</h1></center></body></html>");
    	evbuffer_add_printf(OutBuf,content);
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
    int num = 4;
    
    std::thread Threads[4];
    
    for(int i=0;i<num;i++){
   	 	Threads[i] = std::thread(lunch,nfd);
    }
    for(int i=0;i<num;i++){
    	Threads[i].join();
    }
    //std::thread th2(worker,nfd);
	//th1.join();
	//th2.join();

    //std::cout << num << std::endl;

    return 0;
}

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <string>
#include <typeinfo>

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

__thread char *req_global;
__thread int res_code;

/*
 V8 Interface Function
*/
void setResponse(const v8::FunctionCallbackInfo<v8::Value>& info) {
	//info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),"/start"));
	//info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),req_global));
	//return v8::Undefined();
}
void getUrl(const v8::FunctionCallbackInfo<v8::Value>& info) {
	//info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),"/start"));
	info.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),req_global));
	//return v8::Undefined();
}

class Worker{
	//std::mutex mtx;
	
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
	int loadScript(std::string& name){
	    std::ifstream ifs(name);
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
#ifdef DEBUG
    	std::cout << "[" << jsScript << "]" << std::endl;
#endif
    	ifs.close();
    	return EXIT_SUCCESS;
	}
void init(int nfd,std::string& name) {
    //std::lock_guard<std::mutex> lock(mtx);
	std::string str = "/start";
	req_global = new char[255];
	strcpy(req_global, str.c_str()); 
	
	//++num;
	base = event_init();
	httpd = evhttp_new(base);
	evhttp_accept_socket(httpd, nfd);
	
	if(loadScript(name) == EXIT_FAILURE)
		return;
	
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
			source = String::NewFromUtf8(isolate,"'Welcome:' + getUrl();");
    	// Compile the source code.
    	script = Script::Compile(source);
    	if (script.IsEmpty()) {
    		std::cout <<"Fail Compile:" << name << std::endl;
    		return;
    	}
    	Local<Value> result = script->Run();
		String::Utf8Value utf8(result);
#ifdef DEBUG
		std::cout <<"Init v8 jit result"<< *utf8 << std::endl;
#endif

		evhttp_set_gencb(httpd,inv,this);
		event_base_dispatch(base);
	}
	static void inv(struct evhttp_request *req, void *obj){
		((Worker*)obj)->run(req);
	}
	/*
	* Main Block
	*/
	void run(struct evhttp_request *req){
		res_code = 200;
	
		if (req->type != EVHTTP_REQ_GET) {
    	    evhttp_send_error(req, HTTP_BADREQUEST, "Available GET only");
    	    return;
    	}
		req_path = evhttp_request_uri(req);
    	strcpy(req_global, req_path); 
    	printf("Access Url:%s\n",req_global);
		
		Local<Value> result = script->Run();
		String::Utf8Value utf8(result);

#ifdef DEBUG		
		std::cout <<"Result Length:" << utf8.length() << std::endl;
#endif

		char *content = new char[utf8.length()+1];
    	sprintf(content,"%s\n", *utf8);
    	
#ifdef DEBUG
		std::cout <<"Result:" << *utf8 << pthread_self() << std::endl;
#endif
		
		struct evbuffer *OutBuf = evhttp_request_get_output_buffer(req);
        //evbuffer_add_printf(OutBuf, "<html><body><center><h1>Hello Wotld!</h1></center></body></html>");
    	evbuffer_add(OutBuf,content,strlen(content));
    	evhttp_send_reply(req, HTTP_OK, "", OutBuf);
    	
	}	
};

void lunch(int nfd,std::string name){
	Worker *worker = new Worker();
	worker->init(nfd,name);
};
#define PORT_DEFAULT		8080
#define THREAD_DEFAULT	4
int main(int argc, char* argv[]) {
	std::string scriptName="web.js";
	int port = PORT_DEFAULT;
	int thread_n = THREAD_DEFAULT;

	for (int i=0; i<=argc; i++){
		if (argv[i] != 0) {
			if(i ==1){
				port = std::atoi(argv[i]);
				if(!(1< port && port <65535))
					port = PORT_DEFAULT;
			}else if(i ==2){
				thread_n = std::atoi(argv[i]);
				if(!(1< thread_n && thread_n <256))
					thread_n = THREAD_DEFAULT;
			}else if(i ==3){
				scriptName = std::string(argv[i]);
			}		
		}
	}
	
	int nfd = httpserver_bindsocket(8080,1024);
    std::cout <<"Script:" << scriptName<<" Thread:" << thread_n << " Port:" << port << std::endl;
    
    std::vector<std::thread>threadList;
   
    for(int i=0;i<thread_n;i++){
    	threadList.push_back(std::thread(lunch,nfd,scriptName));
    }
    for(auto iter = threadList.begin(); iter != threadList.end(); ++iter){
    	iter->join();
  	}
    return 0;
}

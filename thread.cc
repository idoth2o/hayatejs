//============================================================================
// Name        : hayate.js
// Author      : .h2o
// Copyright   : .h2o
// Description : Next generation web Infrastructure
// License     : GPL v2 or Proprietary(T.B.D)
//============================================================================
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <string>
#include <typeinfo>
#include <map>

#include "v8.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <evhttp.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define VERSION "0.0.2"
#define PORT_DEFAULT	8080
#define THREAD_DEFAULT	4

using namespace v8;

/*
 Extracts a C string from a V8 Utf8Value.
 */
const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

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

std::mutex cmtx;

void log(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::String::Utf8Value str(args[0]);
    const char* cstr = ToCString(str);
    {
        std::lock_guard<std::mutex> lock(cmtx);
        std::cout << "[Log]" << cstr << std::endl;
    }
}

class Worker{
    //Interface v8 - naitave
    char *req_global;
    int res_code;
    int req_parm;
    std::map<std::string, std::string> req_parms;
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
    /*
     V8 Interface Function
     */
    static void getParms(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate = Isolate::GetCurrent();
        Worker *p = (Worker *)isolate->GetData(0);
        v8::Local<v8::Object> obj = v8::Object::New(isolate);
        
        //for (std::map<std::string, std::string>::iterator it = p->req_parms.begin();
        for (decltype(req_parms)::iterator it = p->req_parms.begin();
            it != p->req_parms.end(); it++) {
            // イテレータは pair<const string, int> 型なので、
            std::string strKey = it->first ;     // イテレータからキーが得られる。
            std::string strVal = it->second;     // イテレータから値が得られる。
            obj->Set(
                     String::NewFromUtf8(isolate, strKey.c_str(),String::kNormalString,strlen(strKey.c_str())),
                     String::NewFromUtf8(isolate, strVal.c_str(),String::kNormalString,strlen(strVal.c_str()))
            );
        }
        args.GetReturnValue().Set(obj);
    }
    static void getParm(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate = Isolate::GetCurrent();
        Worker *p = (Worker *)isolate->GetData(0);
        char buff[10];
        sprintf(buff,"%d",p->req_parm);
        args.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),buff));
    }
    static void getUrl(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate = Isolate::GetCurrent();
        Worker *p = (Worker *)isolate->GetData(0);
#ifdef DEBUG
        std::cout << "[Path]" << p->req_path << std::endl;
#endif
        args.GetReturnValue().Set(String::NewFromUtf8(Isolate::GetCurrent(),p->req_path));
    }
    static void setResponse(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate = Isolate::GetCurrent();
        Worker *p = (Worker *)isolate->GetData(0);
        v8::HandleScope handle_scope(isolate);
        v8::String::Utf8Value utf8(args[0]);
        const char* cstr = ToCString(utf8);
        p->res_code = std::atoi(cstr);
#ifdef DEBUG
        std::cout << "Response:" << p->res_code << " Len[" << args.Length() << "]" << std::endl;
#endif
    }
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
        {
            std::lock_guard<std::mutex> lock(cmtx);
            std::cout << "[" << jsScript << "]" << std::endl; 
        }
#endif
    	ifs.close();
    	return EXIT_SUCCESS;
	}
void init(int nfd,std::string& name) {
	std::string str = "/start";
	req_global = new char[1024];
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
        isolate->SetData(0,this);

    	// Create a stack-allocated handle scope.
    	HandleScope handle_scope(isolate);

    	// Create a new context.
    	context = Context::New(isolate);

    	// Enter the context for compiling and running the hello world script.
    	Context::Scope context_scope(context);
    
        v8::Handle<v8::String> scname = String::NewFromUtf8(isolate,name.c_str());
        ScriptOrigin origin(scname);

	    getUrlObj= ObjectTemplate::New(isolate);
        getUrlObj->Set(String::NewFromUtf8(isolate,("log")), FunctionTemplate::New(isolate,log));
        getUrlObj->Set(String::NewFromUtf8(isolate,("getUrl")), FunctionTemplate::New(isolate, Worker::getUrl));
        getUrlObj->Set(String::NewFromUtf8(isolate,("getParm")), FunctionTemplate::New(isolate, Worker::getParm));
        getUrlObj->Set(String::NewFromUtf8(isolate,("getParms")), FunctionTemplate::New(isolate, Worker::getParms));
        getUrlObj->Set(String::NewFromUtf8(isolate,("setResponse")), FunctionTemplate::New(isolate, Worker::setResponse));
    	getUrlCtx = Context::New(isolate, nullptr, getUrlObj);
    	Context::Scope context_scope2(getUrlCtx);

		if(jsScript != nullptr)
			source = String::NewFromUtf8(isolate,jsScript);
		else
			source = String::NewFromUtf8(isolate,"'Welcome:' + getUrl();");
    
    	// Compile the source code.
    	script = Script::Compile(source,&origin);
    	if (script.IsEmpty()) {
            v8::TryCatch try_catch;
            v8::String::Utf8Value exception(try_catch.Exception());
            
    		std::cout <<"Fail Compile:" << *exception << std::endl;
    		return;
    	}
    
#ifdef RAW_DEBUG
    	Local<Value> result = script->Run();
		String::Utf8Value utf8(result);
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
        struct evkeyvalq params;
        struct evkeyval *param;
        
        req_parm = 0;
		res_code = 200;
	
		if (req->type != EVHTTP_REQ_GET && req->type != EVHTTP_REQ_POST) {
    	    evhttp_send_error(req, HTTP_BADREQUEST, "Available GET only");
    	    return;
    	}
		req_path = evhttp_request_uri(req);
    	strncpy(req_global, req_path ,1023);
    	
        if (req->type == EVHTTP_REQ_GET) {
            char* ret=0;
            int pos=0;
            if ((ret = strchr(req_path,'?')) != NULL ) {
                pos = ret - req_path;
                char *uri = new char[pos+1];
                strncpy(uri, req_path, pos);
                uri[pos+1] = '\0';
#ifdef DEBUG
                std::cout <<"HTTP URI:"<< uri <<std::endl;
#endif
                strncpy(req_global, uri ,1023);
                delete uri;
 
                evhttp_parse_query(req_path,&params);
                for (param = params.tqh_first; param; param = param->next.tqe_next) {
                    req_parm++;
                    std::string key = std::string(param->key);
                    std::string value = std::string(param->value);
                    req_parms[key] = value;
#ifdef DEBUG
                    std::cout <<"HTTP PARM:"<< key << ":"<< value <<std::endl;
#endif
                }
//                std::cout << "[Pasre Size]" << req_parms.size() << std::endl;
            }
        }
        if (req->type ==  EVHTTP_REQ_POST) {
            std::string contenttype;
            std::string headName;
            std::string headValue;
            headName = "Content-Type";
            
            char * post_data = (char *) EVBUFFER_DATA(req->input_buffer);
            int bufsize = EVBUFFER_LENGTH(req->input_buffer);
            
            char * tmp = (char *)malloc(bufsize + 5);
            tmp[0] = '/';
            tmp[1] = '?';
            strncpy((char *) &tmp[2], post_data, bufsize);
            tmp[bufsize + 2] = 0;
            tmp[bufsize + 3] = 0;
            
            struct evkeyvalq *params;
            struct evkeyval *param;
            params = evhttp_request_get_input_headers(req);
            for (param = params->tqh_first; param; param = param->next.tqe_next) {
                std::string key = std::string(param->key);
                std::string value = std::string(param->value);
                if (key.compare("Content-Type") == 0) {
                    if (value.compare("application/x-www-form-urlencoded") == 0) {
#ifdef DEBUG
                        std::cout <<"HTTP POST PARM:"<< tmp <<std::endl;
#endif
                    }
                }
#ifdef DEBUG
                std::cout <<"HTTP PARM:"<< key << ":"<< value <<std::endl;
#endif
            }
            
#ifdef DEBUG		
                std::cout <<"HTTP POST PARM:" << tmp << std::endl;
#endif
        }
        printf("Access Url:%s\n",req_global);
        
		Local<Value> result = script->Run();
        if (result.IsEmpty()){
            v8::TryCatch try_catch;
            v8::String::Utf8Value exception(try_catch.Exception());
            v8::Handle<v8::Message> message = try_catch.Message();
            if (message.IsEmpty()) {
                std::cout <<"Fail Execute:" << *exception << std::endl;
            }else{
                //v8::String::Utf8Value filename(message.GetScriptOrigin().ResourceName());
                int linenum = message->GetLineNumber();
                std::cout <<"Fail Execute: Line:" << linenum << std::endl;
            }
            evhttp_send_error(req, HTTP_INTERNAL , "Internal error");
            return;
            
        }
		String::Utf8Value utf8(result);

#ifdef DEBUG		
		std::cout <<"V8 Result Length:" << utf8.length() << std::endl;
#endif

		char *content = new char[utf8.length()+1];
    	sprintf(content,"%s\n", *utf8);
    	
#ifdef DEBUG
		std::cout <<"V8 Result:" << *utf8 <<" TID:" <<pthread_self() << std::endl;
#endif
        switch (res_code) {
            case 200:
                break;
            case 404:
                evhttp_send_error(req, HTTP_NOTFOUND , "File Not Found");
                return;
            default:
                evhttp_send_error(req, HTTP_INTERNAL , "Internal error");
                return;
        }
        char content_length[8];
        int message_length = strlen(content);
		snprintf(content_length, 7, "%d", message_length);
		struct evbuffer *OutBuf = evhttp_request_get_output_buffer(req);
        evhttp_add_header(req->output_headers, "Content-Type", "text/html");
        evhttp_add_header(req->output_headers, "Content-Length", content_length);
    	evbuffer_add(OutBuf,content,strlen(content));
    	evhttp_send_reply(req, HTTP_OK, "", OutBuf);
        delete content;
	}	
};

void lunch(int nfd,std::string name){
	Worker *worker = new Worker();
	worker->init(nfd,name);
};

int main(int argc, char* argv[]) {
	std::string scriptName="web.js";
    std::string ver = VERSION;
	int port = PORT_DEFAULT;
	int thread_n = THREAD_DEFAULT;

    if (argc < 2) {
        std::cout << "Version:" << ver << std::endl;
        std::cout << "Usage: hayate PortNo threadNo ScriptName" << std::endl;
        return -1;
    }
    
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

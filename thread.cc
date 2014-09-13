#include <iostream>
#include <thread>
#include "v8.h"
using namespace v8;

void worker(int& num) {
    ++num;
		Isolate* isolate;
		Local<Context>  context;
		Local<String> source;
		Local<Script> script;

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
}

int main() {
    int num = 100;
    std::thread th1(worker, std::ref(num));
    num++;
    std::thread th2(worker, std::ref(num));
	th1.join();
	th2.join();

    std::cout << num << std::endl;

    return 0;
}

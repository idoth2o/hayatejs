//require "test.js";
var WebApp = (function(){
    // constructor
    function WebApp(name){
        this._name = name;
		this._route = {};
    }

    // public method
    function get(path,body){
		this._route[path] = body;
    }
    function listen(){
    	var url = getUrl();
    	var cur = this._route[url];
		if( cur == undefined){
        	setResponse(404);
			return "";
		}
		setResponse(200);
		return cur;
    }   	
    WebApp.prototype = {
        constructor: WebApp,
        get: get,
		listen: listen
    };
    return WebApp;
}());

var app = new WebApp("Mini");
app.get("/","Welcome Index");
app.get("/hello","hello World");

app.listen();


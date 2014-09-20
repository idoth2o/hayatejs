//require "test.js";
var WebApp = (function(){
    // constructor
    function WebApp(name){
        this._name = name;
		this._flag = false;
		this._body = "";
    }

    // public method
    function get(path,body){
        var url = getUrl();
		if(url == path){
			this._flag =true;
			this._body =body;
			setResponse(200);
		}
    }
    function listen(){
		if(this._flag == false){
        	setResponse(404);
			return "";
		}
		return this._body;
    }   	
    WebApp.prototype = {
        constructor: WebApp,
        get: get,
		listen: listen
    };
    return WebApp;
}());

log("start");
log("PARM NUM:" + getParm());
var app = new WebApp("Mini");
app.get("/","Welcome Index");
app.get("/hello","hello World");
app.listen();

//require "test.js";

function start() {

  switch (getUrl()){
	case "/":
		return "index";
	case "/next":
		return "next page";
	default:
		return "default";
  }
  //return "welocome";
  //return a + b + getUrl();
　//return a + b;
}
start();

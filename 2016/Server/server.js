var express = require('express');
var fs = require('fs');
var https = require('http'); //require('http');
//TODO: https & socket.io

var app = express();

app.get('/', function(req, res)
{
   var page_html = fs.readFileSync("./pages/home_page.html").toString();
   var date = new Date().toString();
   
   page_html = page_html.replace("[DATE]", date);
   res.send(page_html);
});

app.get('/scout/', function(req, res)
{
   var page_html = fs.readFileSync("./pages/scouting.html").toString();
   res.send(page_html);
});

/*
var HttpsOptions = 
{
  cert: fs.readFileSync('./openssl/key.pem'),  
  key: fs.readFileSync('./openssl/cert.pem')
};
*/

//var server = https.createServer(HttpsOptions, app);
var server = https.createServer(app);

server.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
var express = require('express');
var fs = require('fs');

var app = express();

app.get('/', function(req, res)
{
   var page_html = fs.readFile("./home_page.html");
   page_html.replace("[DATE]", "I have no idea");
   res.send(page_html);
});

var server = app.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
var express = require('express');
var fs = require('fs');
var http = require('http');
var socketio = require('socket.io');

var app = express();
app.use('/assets', express.static("./assets"));
app.set('views', './pages');
app.set('view engine', 'jade');

/**
TODO:
   -encrypt login creds
   -https
   -babel & uglifyjs?
   -logins
   -scouting data submit
   -general posts
   -remove fs.existsSync
   -caching?
   -
ISSUES:
   -you need to refresh to see bluealliance data
   -
*/

var logged_in = [];

var httpget_cache = [];
function HTTPGet(host, path, callback)
{
   var index = host + path;
   
   //TODO: reload this every once in a while
   if(httpget_cache[index] == null)
   {   
      var options = {
         host: host,
         port: 80,
         path: path
      }

      var req = http.request(options, function(res)
      {
         var result = "";
         res.setEncoding('utf8');
         
         res.on("data", function(data)
         {
            result += data;
         });      
         
         res.on("end", function()
         {
            httpget_cache[index] = {value: result, timestamp: (new Date().getTime())};
            callback();
         });
      }).end();
   }
   
   if(httpget_cache[index] != null)
   {
      return httpget_cache[index].value;
   }
   
   return null;
}

app.get('/', function(req, res)
{
   if(logged_in[req.connection.remoteAddress])
   {
      res.render('internal');
   }
   else
   {
      res.render('home');
   }
});

app.get('/scout', function(req, res)
{
   res.redirect('/scout 4618');
});

app.get('/scout:team', function(req, res)
{
   var team = parseInt(req.params.team).toString();
   var bluealliance = HTTPGet("www.thebluealliance.com", "/api/v2/team/frc" + team + "?X-TBA-App-Id=frc4618:CNFRCWebserver:0", function() { console.log(""); });
   
   var bluealliancedata = [];
   
   if(bluealliance != null)
   {
      var bluealliance_json = JSON.parse(bluealliance);
      
      for(var key in bluealliance_json)
      {
         bluealliancedata.push(key + ": " + bluealliance_json[key]);
      }
   }
   else
   {
      bluealliancedata.push("No data found for Team " + team);
   }
   
   res.render('scouting', {"bluealliancedata": bluealliancedata, "teamid": team});
});

function IsValidLogin(username, password)
{
   if(fs.existsSync('./logins.json'))
   {
      var login_json = JSON.parse(fs.readFileSync("./logins.json"));
      
      for(var i = 0; i < login_json.length; ++i)
      {
         var tag = login_json[i];
         if((tag.username === username) &&
            (tag.password === password))
         {
            return true;
         }
      }
   }
   
   return false;
}

var server = http.createServer(app);
var io = socketio(server);

io.on('connection', function(socket)
{
   socket.on('internal_login_creds', function(msg)
   {
      var username = msg.split("____")[0];
      var password = msg.split("____")[1];
      
      if(IsValidLogin(username, password))
      {
         logged_in[socket.handshake.address] = true;
         console.log("Valid");
      }
   });
   
   socket.on('internal_logout', function(msg)
   {
      logged_in[socket.handshake.address] = false;
   });
});

server.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
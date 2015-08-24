var express = require('express');
var fs = require('fs');
var http = require('http');
var socketio = require('socket.io');

var app = express();
app.use('/assets', express.static("./assets"));

/**
TODO:
   -encrypt login creds
   -https
   -
*/

var logged_in = [];

/*
function LoadTeamInfo(team)
{
   var file_path = "./teams/t" + team + ".json";
   var is_valid = fs.existsSync(file_path);
   var team_json = "NOJSON";
   
   if(is_valid)
   {
      team_json = fs.readFileSync(file_path).toString();
   }
   
   var result = 
   {
      valid: is_valid,
      json: is_valid ? JSON.parse(team_json) : {}
   };
   
   return result;
}

function SetTeamInfo(team, info)
{
   var file_path = "./teams/t" + team + ".json";
   fs.writeFileSync(file_path, JSON.stringify(info));
}

function ProcessCommand(command)
{
   try
   {
      var json = JSON.parse(command);
      SetTeamInfo(json.team, json);
   }
   catch(e) {}
}
*/

function HTTPGet(url)
{
   var options = {
      host: url + "?X-TBA-App-Id=frc4618:CNFRCWebserver:0",
      port: 80,
      path: '/'
   }
   
   var result = null;
   
   //TODO: fix this
   /*
   var req = http.request(options, function(res)
   {
      res.setEncoding('utf8');
      
      res.on("data", function(data)
      {
         console.log(data);
         result = data;
      });      
   });
   
   req.end();
   */
   
   return result;
}

app.get('/', function(req, res)
{
   if(logged_in[req.connection.remoteAddress])
   {
      var page_html = fs.readFileSync("./pages/internal.html").toString();
      res.send(page_html);
   }
   else
   {
      var page_html = fs.readFileSync("./pages/home_page.html").toString();
      var date = new Date().toString();
   
      page_html = page_html.replace("[DATE]", date);
      res.send(page_html);
   }
});

app.get('/scout/', function(req, res)
{
   var page_html = fs.readFileSync("./pages/scouting.html").toString();
   res.send(page_html);
});

app.get('/scout/display:team', function(req, res)
{
   var page_html = fs.readFileSync("./pages/scouting_display.html").toString();
   var team = parseInt(req.params.team).toString();
   
   /*
   var team_info = LoadTeamInfo(team);
   
   if(team_info.valid)
   {
      var data = "";
      
      for(var key in team_info.json)
      {
         data = data + "<p>" + key + ": " + team_info.json[key] + "</p>";
      }
      
      page_html = page_html.replace("[DATA_INSERTION_POINT]", data);
   }
   else
   {
      page_html = page_html + "<p>No data found for Team " + team + "</p>";
   }
   */
   
   var bluealliance = HTTPGet("www.thebluealliance.com/api/v2/team/frc" + team);
   console.log(bluealliance);
   
   if(bluealliance != null)
   {
      var data = "";
      var bluealliance_json = JSON.parse(bluealliance);
      
      for(var key in bluealliance_json)
      {
         data = data + "<p>" + key + ": " + bluealliance_json[key] + "</p>";
      }
      
      page_html = page_html.replace("[DATA_INSERTION_POINT]", data);
   }
   else
   {
      page_html = page_html.replace("[DATA_INSERTION_POINT]", "<p>No data found for Team " + team + "</p>");
   }
   
   page_html = page_html.replace("[TEAM]", team);
   
   res.send(page_html);
});

var server = http.createServer(app);
var io = socketio(server);

io.on('connection', function(socket)
{
   io.emit('recive_value', "--connected");
   
   socket.on('submit_value', function(msg)
   {
      //ProcessCommand(msg);
      io.emit('recive_value', msg);
   });
   
   socket.on('internal_login_creds', function(msg)
   {
      var username = msg.split("____")[0];
      var password = msg.split("____")[1];
      
      //TODO: check against actual creds
      logged_in[socket.handshake.address] = true;
   });
   
   socket.on('internal_logout', function(msg)
   {
      logged_in[socket.handshake.address] = false;
   });
   
   socket.on('disconnect', function()
   {
      io.emit('recive_value', "--disconnected");
   });
});

server.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
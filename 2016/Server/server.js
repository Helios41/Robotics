var express = require('express');
var fs = require('fs');
var https = require('http'); //require('http');
var socketio = require('socket.io');

var app = express();

function LoadTeamInfo(team)
{
   var file_path = "./teams/t" + team + ".json";
   var is_valid = fs.existsSync(file_path);
   var team_json = "NOJSON";
   
   if(is_valid)
   {
      team_json = fs.readFileSync(file_path).toString();
      console.log(team_json);
   }
   else
   {
      console.log("no data for team " + team);
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

app.get('/scout/display:team', function(req, res)
{
   var page_html = fs.readFileSync("./pages/scouting_display.html").toString();
   var team = parseInt(req.params.team).toString();
   
   var team_info = LoadTeamInfo(team);
   
   if(team_info.valid)
   {
      for(var key in team_info.json)
      {
         page_html = page_html + "<p>" + key + ": " + team_info.json[key] + "</p>";
      }
   }
   else
   {
      page_html = page_html + "<p>Searching for Team " + team + "</p><p>No data found!</p>";
   }
   
   page_html = page_html.replace("[TEAM]", team);
   res.send(page_html);
});

/*
var HttpsOptions = 
{
  cert: fs.readFileSync('./openssl/key.pem'),  
  key: fs.readFileSync('./openssl/cert.pem')
};
*/

var server = https.createServer(/*HttpsOptions,*/ app);
var io = socketio(server);

io.on('connection', function(socket)
{
   io.emit('recive_value', "--connected");
   
   socket.on('submit_value', function(msg)
   {
      ProcessCommand(msg);
      io.emit('recive_value', msg);
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
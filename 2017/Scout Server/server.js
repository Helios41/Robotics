var express = require('express');
var fs = require('fs');
var http = require('http');
var jade = require('jade');
var cookieParser = require('cookie-parser');
var csvParse = require('csv-parse/lib/sync');

var app = express();
app.set('views', './pages');
app.set('view engine', 'jade');
app.use(cookieParser());

function jadeCompile(viewname, locals)
{
   var render_data = {};
   render_data["view_name"] = viewname;
   
   for(var attrib in locals)
   {
      render_data[attrib] = locals[attrib];
   }
   
   return jade.compileFile(("./pages/" + viewname + ".jade"), {})(render_data);
}

function MatchData(match_number, assigned_team)
{
   var result = {};
   result["number"] = match_number;
   result["assigned_team"] = assigned_team;
   result["submitted"] = false;
   return result;
}

function EntryElement(name, type)
{
   var result = {};
   result["name"] = name;
   result["type"] = type;
   return result;
}

function IngestFormCSV()
{
   var csvSource = fs.readFileSync("./form.csv");
   var parsedCSV = csvParse(csvSource);
   
   var result = [];
   
   for(var i = 0;
       i < parsedCSV.length;
       i++)
   {
      result.push(EntryElement(parsedCSV[i][0], parsedCSV[i][1]));
   }
   
   return result;
}

function IngestScheduleCSV()
{
   var csvSource = fs.readFileSync("./schedule.csv");
   var parsedCSV = csvParse(csvSource);
   
   var result = {};
   
   for(var i = 0;
       i < parsedCSV.length;
       i++)
   {
      var curr_row = parsedCSV[i];
      var name = curr_row[2];
      
      if(result[name] == undefined)
      {
         result[name] = {matches: []};
      }
      
      result[name].matches.push(MatchData(curr_row[0], curr_row[1]));
   }
   
   return result;
}

function DumpDatabase()
{
   var team_list = Object.keys(scouting_data);
   var result = "";

   for(var team_list_index = 0;
       team_list_index < team_list.length;
       team_list_index++)
   {
      result = result + team_list[team_list_index] + "\n";
      var team_data = scouting_data[team_list[team_list_index]];

      for(var entry_index = 0;
          entry_index < team_data.length;
          entry_index++)
      {
         result = result + "   Match " + team_data[entry_index].match_number + "\n";
         result = result + "   By " + team_data[entry_index].name + "\n";
         
         for(var form_element_index = 0;
             form_element_index < form_definition.length;
             form_element_index++)
         {
            result = result + "      " + form_definition[form_element_index].name + ": " + team_data[entry_index][form_definition[form_element_index].name] + "\n";
         }
      }
   }

   return result;
}

function addPortal(render_data, portal_name, portal_link)
{
   if(render_data["navPortals"] === undefined)
   {
      render_data["navPortals"] = [];
      render_data["nav_portal_events"] = "";
   }
   
   render_data["navPortals"].push({"name": portal_name, "link": portal_link});
   render_data["nav_portal_events"] += ("$('#portal_" + portal_link + "').bind('click', function() {window.location.href = '/" + portal_link + "';});");
}

function addPortals(render_data)
{
   addPortal(render_data, "Home", "home");
   addPortal(render_data, "Team Entry", "team_entry");
}

function HasCreds(cookies)
{
   if((cookies == undefined) ||
      (cookies.name == undefined))
   {
      return false;
   }
   
   return true;
}

function IsNumber(in_value)
{
   var value = parseInt(in_value);
            
   if(isNaN(value))
      return false;
   
   return true;
}

function GetMatch(user_name, match_number)
{
   var user_matches = users[user_name].matches;
   
   for(var i = 0;
       i < user_matches.length;
       i++)
   {
      var curr_match = user_matches[i];
      
      if(curr_match.number == match_number)
      {
         return curr_match;
      }
   }
}

app.get('/', function(req, res)
{
   if(HasCreds(req.cookies))
   {
      res.redirect("/home");
   }
   else
   {
      res.redirect("/login");
   }
});

app.get('/login', function(req, res)
{
   if(!HasCreds(req.cookies) || (users[req.cookies.name] == undefined))
   {
      res.send(renderLogin());
   }
   else
   {
      res.redirect("/home");
   }
});

app.get('/home', function(req, res)
{
   if(HasCreds(req.cookies) && (users[req.cookies.name] != undefined))
   {
      res.send(renderHome(req.cookies));
   }
   else
   {
      res.redirect("/login");
   }
});

app.get('/team_entry_form:match_number', function(req, res)
{
   if(IsNumber(req.params.match_number) && (req.params.match_number > 0) &&
      (users[req.cookies.name] != undefined) && 
      (GetMatch(req.cookies.name, parseInt(req.params.match_number).toString()) != undefined) &&
      (!GetMatch(req.cookies.name, parseInt(req.params.match_number).toString()).submitted))
   {
      res.send(renderTeamEntryForm(req.cookies, req.params.match_number));
   }
   else
   {
      res.redirect('/home');
   }
});

app.get('/team_entry', function(req, res)
{
   res.send(renderTeamEntry(req.cookies, null));
});

app.get('/team_entry:team', function(req, res)
{
   if(IsNumber(req.params.team) && (req.params.team > 0))
   {
      res.send(renderTeamEntry(req.cookies, req.params.team));
   }
   else
   {
      res.redirect('/team_entry');
   }
});

function renderLogin()
{
   var render_data = {};
   render_data["users"] = Object.keys(users);
   
   return jadeCompile('login', render_data);
}

function renderHome(cookies)
{
   var render_data = {};
   
   render_data["loggedIn"] = true;
   render_data["name"] = cookies.name;
   render_data["user_data"] = users[cookies.name];
   
   addPortals(render_data);
   return jadeCompile('home', render_data);
}

function renderTeamEntry(cookies, team_in)
{
   var team = parseInt(team_in).toString();
   var render_data = {};
   
   if(team && team_in)
   {
      if(users[cookies.name] != undefined)
      {
         render_data["loggedIn"] = true;
         render_data["name"] = cookies.name;
      }
      
      render_data["teamid"] = team;
      render_data["team_data"] = scouting_data[team];
      render_data["entry_template"] = form_definition;
   }
   else
   {
      render_data["team_list"] = Object.keys(scouting_data);
   }
   
   addPortals(render_data);
   return jadeCompile('team_entry', render_data);
}

function renderTeamEntryForm(cookies, match_number_in)
{
   var match_number = parseInt(match_number_in).toString();
   var render_data = {};
   
   render_data["loggedIn"] = true;
   render_data["name"] = cookies.name;
   render_data["match_number"] = match_number;
   render_data["assigned_team"] = GetMatch(cookies.name, match_number).assigned_team;
   render_data["entry_template"] = form_definition;
   
   addPortals(render_data);
   return jadeCompile('team_entry_form', render_data);
}

app.use('/assets', express.static("./assets"));
app.use('/database_dump', express.static("./database_dump.txt"));

var server = http.createServer(app);
server.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
var socket_io = require('socket.io')(server);

socket_io.on('connection', function(socket)
{
   socket.on('submit_form', function(msg)
   {
      GetMatch(msg.name, msg.match_number).submitted = true;
      
      var assigned_team = GetMatch(msg.name, msg.match_number).assigned_team;
      
      if(scouting_data[assigned_team] == undefined)
      {
         scouting_data[assigned_team] = [];
      }
      
      scouting_data[assigned_team].push(msg);
      fs.writeFileSync("./database_dump.txt", DumpDatabase());
   });
});

var form_definition = IngestFormCSV();
var users = IngestScheduleCSV();
var scouting_data = {};
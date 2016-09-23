var express = require('express');
var fs = require('fs');
var http = require('http');
var jade = require('jade');
var cookieParser = require('cookie-parser');

var RenderFunctions = 
{
   login: renderLogin,
   home: renderHome
};
Object.freeze(RenderFunctions);

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

function MatchData(match_number, red_alliance, blue_alliance, assigned_team)
{
   var result = {};
   result["number"] = match_number;
   result["red_alliance"] = red_alliance;
   result["blue_alliance"] = blue_alliance;
   result["assigned_team"] = assigned_team;
   return result;
}

function UserData(matches_array)
{
   var result = {};
   result["matches"] = matches_array;
   return result;
}

function EntryElement(name, type)
{
   var result = {};
   result["name"] = name;
   result["type"] = type;
   return result;
}

var scouting_data = {};
var users = 
{
   "test1": UserData([MatchData(10, [1, 2, 3], [4, 5, 6], 2), MatchData(12, [1, 2, 7], [4, 5, 6], 4)]),
   "test2": UserData([])
};

function HTTPGet(host, path, callback)
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
         callback(result);
      });
   }).end();  
}

function capitalizeFirstLetter(string)
{
   return string.charAt(0).toUpperCase() + string.slice(1);
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
   if(!HasCreds(req.cookies))
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
   if(HasCreds(req.cookies))
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
   if(IsNumber(req.params.match_number) && (req.params.match_number > 0))
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
   
   if(users[cookies.name] != undefined)
   {
      render_data["loggedIn"] = true;
      render_data["name"] = cookies.name;
      render_data["user_data"] = users[cookies.name];
      
      addPortals(render_data);
      
      return jadeCompile('home', render_data);
   }
   
   return jadeCompile('invalidpath');
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
      addPortals(render_data);
   }
   
   return jadeCompile('team_entry', render_data);
}

function renderTeamEntryForm(cookies, match_number_in)
{
   var match_number = parseInt(match_number_in).toString();
   var render_data = {};
   
   if(match_number && match_number_in && (users[cookies.name] != undefined))
   {
      render_data["loggedIn"] = true;
      render_data["name"] = cookies.name;
      render_data["match_number"] = match_number;
      
      var user_matches = users[cookies.name].matches;
      
      for(var i = 0;
          i < user_matches.length;
          i++)
      {
         var curr_match = user_matches[i];
         
         if(curr_match.number == match_number)
         {
            render_data["assigned_team"] = curr_match.assigned_team;
         }
      }
      
      render_data["entry_template"] = [EntryElement("Score", "text_box"), EntryElement("Is Good", "check_box")];
      
      addPortals(render_data);
   }
   
   return jadeCompile('team_entry_form', render_data);
}

app.use('/assets', express.static("./assets"));

var server = http.createServer(app);
server.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
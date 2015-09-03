var express = require('express');
var fs = require('fs');
var http = require('http');
var socketio = require('socket.io');
var cookieParser = require('cookie-parser');
var socketio_cookieParser = require('socket.io-cookie');
var jade = require('jade');

var crypto = require('crypto');
var encryption = 'aes-256-ctr';
var encryption_key = fs.readFileSync("./key.dat");

const logins_file = "./logins.json";

var RenderFunctions = 
{
   login: renderLogin,
   home: renderHome,
   posts: renderPosts,
   profile: renderProfile,
   internal: renderInternal,
   scouting: renderScout,
   messages: renderMessages
};
Object.freeze(RenderFunctions);

function encryptString(string, key)
{
   var cipher = crypto.createCipher(encryption, key);
   var crypted = cipher.update(string, 'utf8', 'hex');
   crypted += cipher.final('hex');
   return crypted;
}

function decryptString(string, key)
{
   var decipher = crypto.createDecipher(encryption, key);
   var dec = decipher.update(string, 'hex', 'utf8');
   dec += decipher.final('utf8');
   return dec;
}

//console.log(encryptString("test", encryption_key));

var app = express();
app.set('views', './pages');
app.set('view engine', 'jade');

const AccessLevel_User = "User";
const AccessLevel_SU = "SU";
var AccessLevels = [AccessLevel_User, AccessLevel_SU];
Object.freeze(AccessLevels);

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

app.use(cookieParser());

/**
NOTE:
   this server is fairly insecure (eg. unencrypted data packets)
         --it is not intended to be commercially used--
*/

/**
TODO:
   -finish encryption
   -https
   
   -message search not working
   
   -save posts & messages to a file
   -delete specific messages
   -
ISSUES:
   -
*/
var online = [];
var posts = [];
var messages = {};
var scouting_data = {};

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

function addPortal(render_data, portal_name)
{
   if(render_data["navPortals"] === undefined)
   {
      render_data["navPortals"] = [];
      render_data["nav_portal_events"] = "";
   }
   
   render_data["navPortals"].push(portal_name);
   render_data["nav_portal_events"] += ("$('#portal_" + portal_name + "').bind('click', function() {window.location.href = '" + portal_name.toLowerCase() + "';});");
}

function addPortals(render_data, username, password)
{
   var login_data = GetLoginData(username, password);
   
   if(login_data.valid)
   {
      addPortal(render_data, "Home");
      addPortal(render_data, "Scout");
      addPortal(render_data, "Posts");
      addPortal(render_data, "Profile");
      addPortal(render_data, "Messages");
      
      if(login_data.access === AccessLevel_SU)
      { 
         addPortal(render_data, "Internal");
      }
   }
}

function HasCreds(cookies)
{
   if(cookies == undefined)
      return false;
   
   if((cookies.username == undefined) ||
      (cookies.password == undefined))
   {
      return false;
   }
   
   return true;
}

function RequireLogin(cookies, res)
{
   if(!HasCreds(cookies))
   {
      res.redirect("/login");
      return false;
   }
   
   return true;
}

function GetPosts(count)
{
   var rposts = posts.slice().reverse();
   var result = [];
   
   for(var i = 0; (i < count) && (i < rposts.length); ++i)
   {
      result[i] = rposts[i];
   }
   
   return result;
}

function UploadPost(value, username, password)
{
   var login_data = GetLoginData(username, password);
   
   if(login_data.valid)
   {
      var post_data = 
      {
         value: value,
         user: username
      };
      
      posts.push(post_data);
   }
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
   if(RequireLogin(req.cookies, res))
   {
      res.send(renderHome(req.cookies));
   }
});

app.get('/posts', function(req, res)
{
   if(RequireLogin(req.cookies, res))
   {
      res.send(renderPosts(req.cookies));
   }
});

app.get('/messages', function(req, res)
{
   if(RequireLogin(req.cookies, res))
   {
      res.send(renderMessages(req.cookies));
   }
});

app.get('/profile', function(req, res)
{
   if(RequireLogin(req.cookies, res))
   {
      res.send(renderProfile(req.cookies));
   }
});

app.get('/internal', function(req, res)
{
   if(RequireLogin(req.cookies, res))
   {
      res.send(renderInternal(req.cookies));
   }
});

app.get('/scout', function(req, res)
{
   res.send(renderScout(req.cookies, null));
});

app.get('/scout:team', function(req, res)
{
   if(IsNumber(req.params.team) && (req.params.team > 0))
   {
      res.send(renderScout(req.cookies, req.params.team));
   }
   else
   {
      res.redirect('/scout');
   }
});

function renderLogin()
{
   return jadeCompile('login');
}

function renderHome(cookies)
{
   var login_data = GetLoginData(cookies.username, cookies.password);
   var render_data = {};
   
   if(login_data.valid)
   {
      if(online.indexOf(login_data.username) < 0)
         online.push(login_data.username); 
      
      render_data["loggedIn"] = true;
      render_data["username"] = login_data.username;
      
      if(Object.keys(posts).length)
         render_data["posts"] = GetPosts(4);
      
      if(messages[login_data.username] != undefined)
         render_data["messages"] = messages[login_data.username];
      
      if(online.length)
         render_data["online"] = online;
      
      addPortals(render_data, login_data.username, login_data.password);
      
      return jadeCompile('home', render_data);
   }
   
   return jadeCompile('invalidpath');
}

function renderPosts(cookies)
{
   var login_data = GetLoginData(cookies.username, cookies.password);
   var render_data = {};
   
   if(login_data.valid)
   {
      render_data["loggedIn"] = true;
      render_data["username"] = login_data.username;
      
      if(Object.keys(posts).length)
         render_data["posts"] = posts.slice().reverse();
      
      addPortals(render_data, login_data.username, login_data.password);
      
      return jadeCompile('posts', render_data);
   }
   
   return jadeCompile('invalidpath');
}

function renderMessages(cookies, sender)
{
   var login_data = GetLoginData(cookies.username, cookies.password);
   var render_data = {};
   
   if(login_data.valid)
   {
      render_data["loggedIn"] = true;
      render_data["username"] = login_data.username;
      
      if(online.length)
         render_data["online"] = online;
      
      if(sender == undefined)
      {
         if(messages[login_data.username] != undefined)
            render_data["messages"] = messages[login_data.username];
      }
      else
      {
         if(messages[login_data.username] != undefined)
         {
            render_data["messages"] = [];
            
            for(var i = 0; i < messages[login_data.username].length; ++i)
            {
               var message = messages[login_data.username][i];
               if(message.sender == sender)
                  render_data["messages"].push(message);
            }
         }
      }
      
      addPortals(render_data, login_data.username, login_data.password);
      
      return jadeCompile('messages', render_data);
   }
   
   return jadeCompile('invalidpath');
}

function renderProfile(cookies)
{
   var login_data = GetLoginData(cookies.username, cookies.password);
   var render_data = {};
   
   if(login_data.valid)
   {
      render_data["loggedIn"] = true;
      render_data["username"] = login_data.username;
      
      render_data["access_level"] = login_data.access;
      
      addPortals(render_data, login_data.username, login_data.password);
      
      return jadeCompile('profile', render_data);
   }
   
   return jadeCompile('invalidpath');
}

function renderInternal(cookies)
{
   var login_data = GetLoginData(cookies.username, cookies.password);
   var render_data = {};
   
   if(login_data.valid && (login_data.access === AccessLevel_SU))
   {
      render_data["loggedIn"] = true;
      render_data["username"] = login_data.username;
      
      render_data["access_levels"] = AccessLevels;
      render_data["invalid_access_levels"] = CheckAccessLevels();
      render_data["accounts"] = GetUsernames();
      
      addPortals(render_data, login_data.username, login_data.password);
      
      return jadeCompile('internal', render_data);
   }
   
   return jadeCompile('invalidpath');
}

function renderScout(cookies, team_in)
{
   var team = parseInt(team_in).toString();
   var render_data = {};
   
   if(HasCreds(cookies))
   {
      var login_data = GetLoginData(cookies.username, cookies.password);
            
      if(login_data.valid)
      {
         render_data["loggedIn"] = true;
         render_data["username"] = login_data.username;
         addPortals(render_data, login_data.username, login_data.password);
      }
   }
   
   if(team && team_in)
   {
      render_data["teamid"] = team;
      
      if(scouting_data[team] != undefined)      
         render_data["submitted"] = scouting_data[team];
   }
   
   return jadeCompile('scouting', render_data);
}

app.use('/assets', express.static("./assets"));

function CreateAccount(username, password, access)
{
   if(fs.existsSync(logins_file))
   {
      var login_json = JSON.parse(fs.readFileSync(logins_file));
      
      for(var i = 0; i < login_json.length; ++i)
      {
         var tag = login_json[i];
         
         if(tag.username == username)
         {
            console.log("[ERROR] tryed to create duplicate account: " + username);
            return;
         }
      }
      
      var login_info = 
      {
         username: username,
         password: encryptString(password, encryption_key),
         access: access
      };
      
      login_json.push(login_info);
      
      var login_json_str = JSON.stringify(login_json);
      fs.writeFileSync(logins_file, login_json_str);
   }
}

function DeleteAccount(username)
{
   if(fs.existsSync(logins_file))
   {
      var login_json = JSON.parse(fs.readFileSync(logins_file));
      var index = 0;
      
      for(; index < login_json.length; ++index)
      {
         var tag = login_json[index];
         
         if(tag.username == username)
            break;
      }
      
      login_json = login_json.splice(index - 1, 1);
      
      var login_json_str = JSON.stringify(login_json);
      fs.writeFileSync(logins_file, login_json_str);
   }
}

function GetLoginData(username, password)
{
   if(fs.existsSync(logins_file))
   {
      var login_json = JSON.parse(fs.readFileSync(logins_file));
      
      for(var i = 0; i < login_json.length; ++i)
      {
         var tag = login_json[i];
         
         if((tag.username == username) &&
            (tag.password == encryptString(password, encryption_key)))
         {
            if(AccessLevels.indexOf(tag.access) < 0)
            {
               console.log("[ERROR] Invalid access level for " + tag.username + ": " + tag.access);
            }
            
            var login_data = 
            {
               valid: true,
               username: tag.username,
               password: decryptString(tag.password, encryption_key),
               access: tag.access
            };
            
            return login_data;
         }
      }
   }
   
   return {valid: false};
}

function CheckAccessLevels()
{
   if(fs.existsSync(logins_file))
   {
      var invalid_users = [];
      var login_json = JSON.parse(fs.readFileSync(logins_file));
      
      for(var i = 0; i < login_json.length; ++i)
      {
         var tag = login_json[i];
         
         if(AccessLevels.indexOf(tag.access) < 0)
         {
            invalid_users.push(tag.username + ": " + tag.access);
         }
      }
      
      if(invalid_users.length > 0)
         return invalid_users;
   }
   
   return ["No Invalid Access Levels"];
}

function DoesUsernameExist(username)
{
   if(fs.existsSync(logins_file))
   {
      var login_json = JSON.parse(fs.readFileSync(logins_file));
      
      for(var i = 0; i < login_json.length; ++i)
      {
         var tag = login_json[i];
         
         if(tag.username == username)
            return true;
      }
   }
   
   return false;
}

function GetUsernames()
{
   var usernames = [];
   
   if(fs.existsSync(logins_file))
   {
      var login_json = JSON.parse(fs.readFileSync(logins_file));
      
      for(var i = 0; i < login_json.length; ++i)
      {
         var tag = login_json[i];
         usernames.push(tag.username);
      }
   }
   
   return usernames;
}

function UpdateDiv(socket, viewname, divname, in_params)
{
   var params = [];
   
   if(in_params != undefined)
      params = in_params;
   
   var update_data = 
   {
      params: params,
      id: divname,
      view: viewname
   };
   socket.emit('update_div_callback', update_data);
}

var server = http.createServer(app);
var io = socketio(server);

io.use(socketio_cookieParser);

io.on('connection', function(socket)
{  
   socket.on('login', function(msg)
   {
      if((msg.username !== undefined) &&
         (msg.password !== undefined))
      {
         var login_data = GetLoginData(msg.username, msg.password);
         
         if(login_data.valid)
         {
            socket.emit('set_cookie', {name: "username", value: msg.username});
            socket.emit('set_cookie', {name: "password", value: msg.password});
            
            socket.emit('goto_page', '/home');
         }
      }
   });

   socket.on('update_div_callback', function(msg)
   {
      if(msg == undefined)
         return;
      
      if((msg.params != undefined) &&
         (msg.id != undefined) &&
         (msg.view != undefined))
      {
         if(RenderFunctions.hasOwnProperty(msg.view) > -1)
         {
            var param_array = [socket.request.headers.cookie];
            
            for(var i = 0; i < msg.params.length; ++i)
               param_array.push(msg.params[i]);
            
            var update_data = 
            {
               html: RenderFunctions[msg.view].apply(null, param_array),
               id: msg.id,
               view: msg.view
            };
         
            socket.emit('update_div', update_data);
         }
      }
   });
   
   socket.on('logout', function(msg)
   {
      socket.emit('clear_cookie', "username");
      socket.emit('clear_cookie', "password");
      
      socket.emit('goto_page', '/login');
   });
   
   socket.on('create_login', function(msg)
   {
      var username = socket.request.headers.cookie.username;
      var password = socket.request.headers.cookie.password;
      
      if((username != undefined) &&
         (password != undefined))
      {
         var login_data = GetLoginData(username, password);
         
         if(login_data.valid && (login_data.access === AccessLevel_SU))
         {
            CreateAccount(msg.username, msg.password, msg.access);
         }
      }
   });
   
   socket.on('delete_user', function(msg)
   {
      var username = socket.request.headers.cookie.username;
      var password = socket.request.headers.cookie.password;
      
      if((username != undefined) &&
         (password != undefined))
      {
         var login_data = GetLoginData(username, password);
         
         if(login_data.valid && (login_data.access === AccessLevel_SU))
         {
            DeleteAccount(msg);
         }
      }
   });
   
   socket.on('upload_post', function(msg)
   {
      if(HasCreds(socket.request.headers.cookie))
      {
         var login_data = GetLoginData(socket.request.headers.cookie.username,
                                       socket.request.headers.cookie.password);
         
         if(login_data.valid)
         {
            UploadPost(msg,
                       socket.request.headers.cookie.username,
                       socket.request.headers.cookie.password);
                    
            UpdateDiv(io, 'home', "postdata_block");
            UpdateDiv(io, 'posts', "postdata_block");
         }
      }
   });
   
   socket.on('clear_posts', function(msg)
   {
      if(HasCreds(socket.request.headers.cookie))
      {
         var login_data = GetLoginData(socket.request.headers.cookie.username,
                                       socket.request.headers.cookie.password);
                                       
         if(login_data.valid && (login_data.access === AccessLevel_SU))
         {
            posts = [];
            
            UpdateDiv(io, 'home', "postdata_block");
            UpdateDiv(io, 'posts', "postdata_block");
         }
      }
   });
   
   socket.on('check_access_levels', function(msg)
   {
      if(HasCreds(socket.request.headers.cookie))
      {
         var login_data = GetLoginData(socket.request.headers.cookie.username,
                                       socket.request.headers.cookie.password);
         
         if(login_data.valid && (login_data.access === AccessLevel_SU))
            UpdateDiv(socket, 'internal', "invalid_accessdata_block");
      }      
   });
   
   socket.on('request_specific_messages', function(msg)
   {
      UpdateDiv(socket, 'messages', "messagesdata_block", [msg]);
   });
   
   socket.on('send_message', function(msg)
   {
      if(HasCreds(socket.request.headers.cookie) &&
         (msg.value != undefined) &&
         (msg.target != undefined))
      {
         var login_data = GetLoginData(socket.request.headers.cookie.username,
                                       socket.request.headers.cookie.password);
         
         if(login_data.valid && DoesUsernameExist(msg.target))
         {
            if(messages[msg.target] == undefined) 
               messages[msg.target] = [];
            
            var message_data = 
            {
               value: msg.value,
               sender: login_data.username
            };

            messages[msg.target].push(message_data);
            UpdateDiv(io, 'home', "messagedata_block");
            UpdateDiv(io, 'messages', "messagesdata_block");
         }
      }
   });
   
   socket.on('submit_scout_data', function(msg)
   {
      if(HasCreds(socket.request.headers.cookie) &&
         (msg.value != undefined) &&
         (msg.team != undefined))
      {
         var login_data = GetLoginData(socket.request.headers.cookie.username,
                                       socket.request.headers.cookie.password);
         
         if(login_data.valid)
         {
            if(scouting_data[msg.team] == undefined)
               scouting_data[msg.team] = [];
            
            var scout_data = 
            {
               user: login_data.username,
               value: msg.value
            };
            
            scouting_data[msg.team].push(scout_data);
            UpdateDiv(io, 'scouting', "submitteddata_block");
         }
      }
   });
   
   socket.on('request_scout_data', function(team)
   {
      HTTPGet("www.thebluealliance.com", "/api/v2/team/frc" + team + "?X-TBA-App-Id=frc4618:CNFRCWebserver:0", function(data)
      {
         var render_data = {};
         
         var username = socket.request.headers.cookie.username;
         var password = socket.request.headers.cookie.password;
         
         if((username != undefined) &&
            (password != undefined))
         {
            var login_data = GetLoginData(username, password);
            
            if(login_data.valid)
            {
               render_data["loggedIn"] = true;
               render_data["username"] = login_data.username;
            }
         }
         
         var bluealliancedata = [];
         var bluealliance_json = JSON.parse(data);
         
         for(var key in bluealliance_json)
         {
            bluealliancedata.push(key + ": " + bluealliance_json[key]);
         }
         
         render_data["teamid"] = team;
         render_data["bluealliancedata"] = bluealliancedata;
         
         var scout_data = 
         {
            teamid: team,
            value: jadeCompile("scouting", render_data)
         };
         
         socket.emit('recive_scout_data', scout_data);
      });
   });
   
   socket.on('disconnect', function()
   {
      //TODO: fix this, it doesnt work
      return;
      if(HasCreds(socket.request.headers.cookie))
      {
         var login_data = GetLoginData(socket.request.headers.cookie.username,
                                       socket.request.headers.cookie.password);
                                       
         if(login_data.valid)
         {
            //TODO: what if youre logged on in 2 computers?
            if(online.indexOf(login_data.username) > -1)
               online.splice(online.indexOf(login_data.username), 1); 
         }
      }
   });
});

server.listen(3000, function()
{
   var host = server.address().address;
   var port = server.address().port;
   
   console.log("Server listening at %s : %s", host, port);
});
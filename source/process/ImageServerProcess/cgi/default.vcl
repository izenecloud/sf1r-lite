# This is a basic VCL configuration file for varnish.  See the vcl(7)
# man page for details on VCL syntax and semantics.
# 
# Default backend definition.  Set this to point to your content
# server.
# 
backend default {
    .host = "127.0.0.1";
    .port = "8888";
}

acl purge {
     "localhost";
     "172.16.0.0"/24;
}

# 
# Below is a commented-out copy of the default VCL logic.  If you
# redefine any of these subroutines, the built-in logic will be
# appended to your code.
# 
 sub vcl_recv {
     if (req.http.x-forwarded-for) {
         set req.http.X-Forwarded-For =
             req.http.X-Forwarded-For ", " client.ip;
     } else {
         set req.http.X-Forwarded-For = client.ip;
     }
     if (req.request == "PURGE") {
         if (!client.ip ~ purge) {
             error 405 "This IP is not allowed to send PURGE requests.";
         }

         return (lookup);
     }
     if (req.request != "GET" &&
       req.request != "HEAD" &&
       req.request != "PUT" &&
       req.request != "POST" &&
       req.request != "TRACE" &&
       req.request != "OPTIONS" &&
       req.request != "DELETE") {
         /* Non-RFC2616 or CONNECT which is weird. */
         return (pipe);
     }
     if (req.request != "GET" && req.request != "HEAD") {
         /* We only deal with GET and HEAD by default */
         return (pass);
     }
     if (req.url ~ "^/image.cgi") {
         unset req.http.cookie;
     }
     if (req.url ~ "^/image/") {
         unset req.http.cookie;
     }
     if (req.http.Authorization || req.http.Cookie) {
         /* Not cacheable by default */
         return (pass);
     }
     return (lookup);
 }
 
 sub vcl_pipe {
     # Note that only the first request to the backend will have
     # X-Forwarded-For set.  If you use X-Forwarded-For and want to
     # have it set for all requests, make sure to have:
     # set req.http.connection = "close";
     # here.  It is not set by default as it might break some broken web
     # applications, like IIS with NTLM authentication.
     set req.http.connection = "close";
     return (pipe);
 }
 
sub vcl_pass {
    return (pass);
}
# 
# sub vcl_hash {
#     set req.hash += req.url;
#     if (req.http.host) {
#         set req.hash += req.http.host;
#     } else {
#         set req.hash += server.ip;
#     }
#     return (hash);
# }
 
 sub vcl_hit {
     if (req.request == "PURGE") {
         set obj.ttl = 0s;
         error 200 "Purged";
     }
     return (deliver);
 }
 
 sub vcl_miss {
     if (req.request == "PURGE") {
         error 404 "Not in cache";
     }
     return (fetch);
 }
 
 sub vcl_fetch {
#     if (!beresp.cacheable) {
#         return (pass);
#     }
     if( (req.request == "GET" || req.request == "HEAD") && (req.url ~ "^/image.cgi" || req.url ~ "^/image/") ) {
         if(beresp.status == 200 || beresp.status == 302) {
             set beresp.ttl = 1h;
         }
         if(beresp.status == 404) {
             set beresp.ttl = 0s;
         }
         if(beresp.status >= 500) {
             set beresp.ttl = 0s;
         }
         return (deliver);
     }
     if (beresp.http.Set-Cookie) {
         return (pass);
     }
 }
# 
# sub vcl_deliver {
#     return (deliver);
# }
# 
# sub vcl_error {
#     set obj.http.Content-Type = "text/html; charset=utf-8";
#     synthetic {"
# <?xml version="1.0" encoding="utf-8"?>
# <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
#  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
# <html>
#   <head>
#     <title>"} obj.status " " obj.response {"</title>
#   </head>
#   <body>
#     <h1>Error "} obj.status " " obj.response {"</h1>
#     <p>"} obj.response {"</p>
#     <h3>Guru Meditation:</h3>
#     <p>XID: "} req.xid {"</p>
#     <hr>
#     <p>Varnish cache server</p>
#   </body>
# </html>
# "};
#     return (deliver);
# }

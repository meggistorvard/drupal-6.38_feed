
================================================================================
Drupal Feeds Test Site
DEMO http://classified.synergize.co/
================================================================================

A Drupal installation profile for testing Feeds module.

Installation
--------------------------------------------------------------------------------

Download and install following Drupal installation instructions http://drupal.org/getting-started/install When prompted for which profile to install, choose "Feeds Test Site".

Once installed, manually install Simpletest following its setup instructions in INSTALL.txt. Simpletest is available in profiles/feeds_test/modules/simpletest but it is not automatically enabled as it will require you to patch Drupal core before installation. Patch simpletest with #587304-3 http://drupal.org/node/587304#comment-2083362

Then, download SimplePie and place simplepie.inc in feeds/libraries.

Go to admin/build/testing, select "Feeds" and run tests.

Developers
--------------------------------------------------------------------------------

If you develop for Feeds, run rebuild-dev.sh in profiles/feeds_test which will rebuild the modules directory, check out the latest Feeds version from CVS and patch Simpletest module (rebuild-dev.sh won't patch Drupal core though, so you still have to take care of that yourself).

For running rebuild-dev.sh, you will need drush >= 3.1 and drush_make >= 2.0-beta7.
http://drupal.org/project/drush
http://drupal.org/project/drush_make

Modules in this project
--------------------------------------------------------------------------------

Feeds
********************************************************************************

Import or aggregate data as nodes, users, taxonomy terms or simple database records.

One-off imports and periodic aggregation of content
Import or aggregate RSS/Atom feeds
Import or aggregate CSV files
Import or aggregate OPML files
PubSubHubbub support
Create nodes, users, taxonomy terms or simple database records from import
Extensible to import any other kind of content
Granular mapping of input elements to Drupal content elements
Exportable configurations
Batched import for large files
Installation
For requirements and installation instructions, refer to the included README.txt file.

Documentation
For a guide to using Feeds in site builds or how to develop for Feeds, see the Feeds documentation. On developmentseed.org, there is a December 09 post showing how to import and aggregate with Feeds in three brief screencasts.
To see what Feeds can do, you can enable one of the included feature modules: Feeds Import or Feeds News (requires the Features module).

Modules that extend Feeds
Feeds Tamper is a useful module that provides several ways to transform your data before it gets saved.
Feeds extensible parsers allows you to import XML or JSON files (among others) using XPath/JSONPath queries.
Feeds Import Preview provides a way of previewing the source content before importing. This can help you with debugging your imports.
Commerce Feeds allows you to import products into your Drupal Commerce installation.
Feeds Entity Processor allows you to import content for almost any entity type. Be aware though that the module is still a bit experimental, so for some entity types you may run into errors. If so, don't hesitate to open an issue!
For a better reference of contributed modules for Feeds, read the corresponding page of the documentation or use drupal.org's search.

6.x
The D6 version of Feeds is no longer officially supported. All 6.x issues have been closed. However, if you still like to see something fixed in this version you can still open an issue for it or reopen and old one. Just be prepared to get it RTBC by yourself, as these issues will generally be ignored by the maintainers.

Feeds powers the news tracker Managing News https://www.drupal.org/project/managingnews


Services
http://drupal.org/project/services
********************************************************************************

A standardized solution for building API's so that external clients can communicate with Drupal. Out of the box it aims to support anything Drupal Core supports and provides a code level API for other modules to expose their features and functionality via HTTP. It provide Drupal plugins that allow others to create their own authentication mechanisms, request formats, and response formats.

Services 3.x(Drupal 7):
Service API allow modules to create other services, including pluggable access control
Server API allow modules to create other servers, such as SOAP
Aliasing methods
Integration with core Drupal functionality like files, nodes, taxonomy, users, files and more.
Response format API allows you to define response Formats for CONTENT-TYPE ie. application/json or application/xml. (also calls such as ENDPOINT/node/1.json work)
Visit the Services Handbook for help and information. Subscribe to the Services Group for news, updates and discussions.

For modules with services support goto https://drupal.org/node/750036. Note: anyone can add to this list.

REST Server
https://www.drupal.org/project/rest_server
********************************************************************************

The REST Server provides an interface for the Services module to communicate with REST clients.

REST Server 2.x
This is a brief introduction to how the rest server works. See the [services_oop][services_oop] module to find out more about how you easily can expose functionality in a resource-oriented way.

Controllers
-------------------

Tabulation of the controller mapping for the REST server. Requests gets mapped to different controllers based on the HTTP method used and the number of parts in the path.

Count refers to the number of path parts that comes after the path that identifies the resource type. The request for `/services/rest/node/123` would have the count 1, as `/services/rest/node` identifies the resource.

    X = CRUD
    A = Action
    T = Targeted action
    R = Relationship request

    COUNT |0|1|2|3|4|N|
    -------------------
    GET   |X|X|R|R|R|R|
    -------------------
    POST  |X|A|T|T|T|T|
    -------------------
    PUT   | |X| | | | |
    -------------------
    DELETE| |X| | | | |
    -------------------

CRUD
-------------------

The basis of the REST server.

    Create:   POST /services/rest/node + body data
    Retrieve: GET /services/rest/node/123
    Update:   PUT /services/rest/node/123 + body data
    Delete:   DELETE /services/rest/node/123

And last but least, the little bastard sibling to Retrieve that didn't get it's place in the acronym: 

    Index:    GET /services/rest/node

In the REST server the index often doubles as a search function. The comment resource allows queries like the following for checking for new comments on a node (where 123456 is the timestamp for the last check and 123600 is now):

    New comments: GET /services/comment?nid=123&timestamp=123456:
    Comments in the last hour: GET /services/comment?timestamp=120000:123600

Actions
-------------------

Actions are performed directly on the resource type, not a individual resource. The following example is hypothetical (but plausible). Say that you want to expose a API for the [apachesolr][apachesolr] module. One of the things that could be exposed is the functionality to reindex the whole site.

    Publish:  POST /services/rest/apachesolr/reindex

Targeted actions
-------------------

Targeted actions acts on a individual resource. A good, but again - hypothetical, example would be the publishing and unpublishing of nodes. 

    Publish:  POST /services/rest/node/123/publish

Relationships
-------------------

Relationship requests are convenience methods (sugar) to get something thats related to a individual resource. A real example would be the relationship that the [comment_resource][comment_resource] module adds to the node resource:

    Get comments: GET /services/rest/node/123/comments

This more or less duplicates the functionality of the comment index:

    Get comments: GET /services/rest/comments?nid=123

[apachesolr]: http://drupal.org/project/apachesolr "Apache Solr Search Integration"
[comment_resource]: http://github.com/hugowetterberg/comment_resource "Comment resource"
[services_oop]: http://github.com/hugowetterberg/services_oop "Services OOP"
REST Server 1.x
REST can use either GET or POST values, so if you request:

http://www.example.com/services/rest?method=node.load&nid=1&fields=title,body
... You will be returned the title and body of node 1 in a format that REST clients could understand. The same goes for parameters that are passed through by POST. The key here is that you request method as the service to call, and pass all the parameters as their own names.


REST Client
https://www.drupal.org/project/rest_client
********************************************************************************

This module is undergoing large changes, production use is not recommended.

INTRODUCTION
REST Client is a robust HTTP request module to consume REST style services.

Why use REST Client instead of drupal_http_request():

Streams large files
Uses Expect 100 header before sending data in case of redirection
Fully customizable HTTP request
REST utility functions: HMAC, binary SHA1, binary MD5
TARGET AUDIENCE
End Users:
This is for module developers only. Do NOT install unless required by another module.

Module Developers:
Please use drupal_http_request() included in Drupal core. If drupal_http_request()
does not fit your needs, this is a good replacement module. See included README.txt
file for instructions on how to use this module.

Please post an issue for any REST service this does not work for along with a
link to the service's REST API in this projects issue queue.

I would like to hear not only what services this module does NOT work with but also the services it DOES work with.

REST Client is a robust HTTP request module to consume REST style services.

Why use REST Client instead of drupal_http_request():
1. Streams large files
2. Uses Expect 100 header before sending data in case of redirection
3. Fully customizable HTTP request
4. REST utility functions: HMAC, binary SHA1, binary MD5


TARGET
======
End Users:
This is for module developers only. Do NOT install unless required by another 
module.

Module Developers:
Please use drupal_http_request() included in Drupal core. If drupal_http_request() 
does not fit your needs this is your module. 

Please post an issue for any REST service this does not work for along with a 
link to the service's REST API in this projects issue queue.


REQUEST API
===========
rest_client_request
This is the only function you call to send a HTTP request. All other functions 
except utility functions are private funcitons used by rest_client_request

PARAMS
$request = array()
The request parameter is an associative array holding parts of the HTTP request 
that make up the first line of the request.
Ex. $request = array( // defaults
    'method'      => 'GET',  // ['PUT', 'GET', 'DELETE', 'HEAD'] or any other method
    'resource'    => '/',    // the path of the request not including the host
    'port'        => '80',   // port of the request
    'httpversion' => '1.1',  // HTTP version '1.0' or '1.1'
    'scheme'      => 'http'  // scheme of request ['http', 'https'] or other scheme
  );

$headers = array()
This is an associative array of request headers. The key is the header name and 
the values is the header value. Any headers can be used here. Date, Content-Length, 
and Content-Md5 are determined for you at the time of the request.
Ex. $headers = array(
    'host' => 'example.com',
    'content-type' => 'text/plain'
  );

$data = null
This holds the request body information. If there is no body to be sent then set 
$data to null. If a body is to be sent then set $data to an assosative array.
Ex. $data = array(
    'type'  => 'file',             // ['file', 'string']
    'value' => '/path/to/file.mpg' // if type = file then value is the path to the file
  );                                 // if type = string then value contains the string

$retry = 3
This is the number of times to retry the request if redirected.

RETURN
rest_client_request returns a std class object holding the parsed response headers 
and the raw response body.
Ex. $response->headers   // an associative array containing all the response headers
  $response->code      // contains the HTTP response code
  $response->text      // contains the HTTP response text following the code
  $response->codeText  // contains the standard HTTP response text that the code matches
  $response->data      // contains the raw response body  
  $response->errorCode // contains the error code if the socket could not be opened
  $response->errorText // contains the error text if the socket could not be opened
  
You should always check if $resopnse->errorCode has been set before attempting 
to process the response.


UTILITY FUNCTIONS API
=====================
rest_client_hmac
This function takes a string and encryptes it with a HMAC SHA1 encryption. A key 
must be given encrypt with. This is usually the secret key in many REST services.

rest_client_binarySha1
This function takes a string and encrypts it with sha1 returns the result in 
binary form. This function is PHP4 & 5 safe. Many times you will need to wrap 
this with base64_encode().

rest_client_binaryMd5
This function takes a data array as sent to the request function and returns the 
$data->value in binary form. This function is PHP4 & 5 safe. Many times you will 
need to wrap this with base64_encode().
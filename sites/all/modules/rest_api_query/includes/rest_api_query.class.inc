<?php
/**
 * @file
 * Defines the rest_api_query class; a base for other modules to extend.
 */

/**
 * Base class for building and executing RESTful API queries.
 */
abstract class rest_api_query {
  // Prevents execution if initialization failed.
  protected $initialized = FALSE;

  // The REST API Schema.  Not to be confused with the HTTP scheme.
  protected $rest_api_schema = '';

  /**
   * Components to build the HTTP request.
   */

  // Access point.
  protected $scheme = 'http';
  protected $host;
  protected $port;
  protected $path = '/';

  // @todo Postponed: Support other formats.  XML is mildly popular in REST APIs.
  protected $format = 'json';

  // Authentication credentials.
  protected $uses_api_key = TRUE;
  protected $api_key;
  protected $username;
  protected $password;

  // The object type, object ID and METHOD for the query.
  public $method = 'GET';
  public $resource;
  public $identifier;

  // Other public parameters.
  public $parameters = array();
  // Many REST APIs need the correct content-type header.
  public $headers = array(
    'Content-type' => 'application/json',
  );

  // The body of the message to send.  It will be encoded with drupal_to_js().
  public $data;


  /**
   * Properties for configuration, internal use and read-only response data.
   *
   * All protected and private properties are made readable via __get().
   */

  // The fully assembled URI for the access point.  URI parts may be set through
  // this property.  It has a magic getter and setter so can be accessed
  // externally, despite it's "private" visibility.
  private $access_point;

  // The fully assembled URL, including the resource and identifier.
  // Internal use only, though has a magic getter for external read access.
  private $url;

  private $cache_id;

  // The response object from drupal_http_request().
  protected $response;

  // Error code and error message, if any.
  protected $code;
  protected $error = FALSE;
  protected $success = FALSE;

  // The decoded response.
  protected $result;

  // When do caches expire?
  // Defaults to one hour; 60 * 60.
  public $cache_validity = 3600;
  public $cache_table = 'cache_rest_api_query';

  /**
   * Set various properties on instantiation.
   *
   * If the value is NULL or empty, the constructor will attempt to get the value
   * with variable_get().  It will look for variable names of the format
   * "SCHEMA_PROP", where "SCHEMA" is the REST API Schema's name, and "PROP" is
   * the name of the property.  E.g. "redmine_access_point" or "toggl_api_key".
   * If the value is still not found, the constructor will set an error message
   * to $this->error, drupal_set_message() and watchdog().
   *
   * @param $properties Array
   *    The key is the name of the rest_api_query property to be set.  The value
   *    is the value to set it to.
   */
  function __construct($properties) {
    // Allow set_active_rest_api_key() to override the api_key.
    if ($this->uses_api_key) {
      if ($key = set_active_rest_api_key($this->rest_api_schema)) {
        $this->api_key = $key;
      }
    }

    foreach ($properties as $property => $value) {
      // Was the value passed in?
      if (empty($value)) {
        // No;  Is it set in Drupal's configuration?
        $variable_name = $this->rest_api_schema . '_' . $property;
        $value = variable_get($variable_name, FALSE);
        // Do not set error if the API key is set with set_active_rest_api_key().
        if (empty($value) && empty($this->{$property})) {
          // No;  Instantiation failed.  Set error messages.
          $message = "!schema API needs the <code>!variable_name</code> to access the REST API.  Set <code>\$conf['!variable_name']</code> in <code>!conf_path/settings.php</code>.";
          $args = array(
            '!schema' => get_rest_api_schema($this->rest_api_schema, 'name'),
            '!variable_name' => $variable_name,
            '!conf_path' => conf_path(),
          );

          // Set the error message on this instance, as a drupal message, and
          // with watchdog.
          $this->error = t($message, $args);
          drupal_set_message($this->error, 'error');
          watchdog('redmine', $message, $args, WATCHDOG_ERROR);

          // Initialization failed.  Stop trying.
          return;
        }
      }

      // Set the property.  Try to use the setter function first.
      if (!$this->__set($property, $value)) {
        // Set the property directly only if the setter function failed.
        $this->$property = $value;
      }
    }

    // Set the api_key yet again if it has been overridden with
    // set_active_rest_api_key().  The previous code will have overridden it.
    if ($key) {
      $this->api_key = $key;
    }

    // Initialization succeeded.  Permit this object to be executed.
    $this->initialized = TRUE;
  }

  /**
   * Magic method to allow all private and protected properties to be read.
   */
  function __get($name) {
    // Use the getter function if there is one.
    $method = "get_$name";
    if (!isset($this->{$name}) && method_exists($this, $method)) {
      $this->{$name} = $this->$method();
    }

    // Allow read-only access to all protected properties.
    if (isset($this->{$name})) {
      return $this->{$name};
    }
  }

  /**
   * Magic method to allow certain private properties to be set.
   *
   * To make a private property setttable through this magic method, implement a
   * "set_NAME" method where NAME is the private property to set.
   *
   * @return Boolean
   *    TRUE if the property was set.
   */
  function __set($name, $value) {
    $func = "set_$name";
    if ($return = method_exists($this, $func)) {
      $this->$func($value);
    }
    return $return;
  }

  /**
   * Set the "access_point" pseudo-property.
   *
   * Internal use only.  Set the "access_point" property instead;
   * @code $this->access_point = 'https://example.com';
   *
   * @param $url String
   *    The URI representing the base access point.
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  protected function set_access_point($url) {
    if ($url = parse_url($url)) {
      // Do not set the path component of the URL if it is single forward slash.
      if (isset($url['path']) && $url['path'] == '/') {
        $url['path'] = '';
      }

      $map = array(
        'user' => 'username',
        'pass' => 'password',
      );

      // Iterate over the URL components.
      foreach ($url as $property => $value) {

        // Map the URL components to rest_api_query properties, if necessary.
        if (isset($map[$property])) {
          $property = $map[$property];
        }
        $this->{$property} = $value;
      }
    }
    else {
      throw new Exception("Poorly formed URL for Rest API Query Access Point.");
    }

    // Allow chaining.
    return $this;
  }

  /**
   * Get the "access_point" pseudo-property.
   *
   * Internal use only.  Get the "access_point" property instead;
   * @code $this->access_point
   *
   * @return String
   *    The URI representing the base access point.
   */
  protected function get_access_point() {
    if (!$this->initialized || empty($this->scheme) || empty($this->host)) {
      return FALSE;
    }

    // Assemble the bits for the URL.
    // Most REST APIs allow authentication with the API key as the BasicAuth "user".
    if (!empty($this->api_key)) {
      $this->username = $this->api_key;
    }

    // Build the authentication component of the URL.
    $authentication = !empty($this->username) ? $this->username : '';
    if (!empty($this->password)) {
      $authentication .= ':' . $this->password;
    }
    if (!empty($authentication)) {
      $authentication .= '@';
    }

    // Build the port component of the URL, if it set.
    $port = !empty($this->port) ? ':' . $this->port : '';

    // Do not store the path if it is just the slash character.
    if ($this->path == '/') {
      $this->path = '';
    }

    // Build the whole URL from it's parts.
    return $this->scheme . '://' . $authentication . $this->host . $port . $this->path;
  }

  /**
   * Get the "url" pseudo-property.
   *
   * Internal use only.  Get the "url" property instead;
   * @code $this->url
   *
   * @return $url String
   *    The URI representing the full request.
   */
  protected function get_url() {
    $access_point = $this->__get('access_point');
    if (!$this->initialized || !$access_point) {
      // Do not execute if initialization failed.
      return FALSE;
    }

    // Assempble the parts of the URL.
    $url = array(
      $access_point,
      $this->resource,
    );
    if (isset($this->identifier)) {
      $url[] = $this->identifier;
    }

    // @todo Postponed: Consider adding support for methods.  E.g.
    // POST /issues/1234/close.json
    // @see rest_api_query()

    $url = implode('/', $url) . '.' . $this->format;

    // Have Drupal check the URL and handle the HTTP request.
    return url($url, array('query' => $this->parameters, 'external' => TRUE));
  }

  /**
   * Set or overwrite an individual query string parameter.
   *
   * @param $name String
   *    The key of the parameter to set.
   * @param $value String
   *    The value to set the parameter to.
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  public function set_parameter($name, $value) {
    $this->parameters[$name] = $value;

    // Allow chaining.
    return $this;
  }

  /**
   * Set or overwrite query string parameters.
   *
   * @param $parameters Array(String => String)
   *    The key of the parameter to set.
   * @param $value String
   *    The value to set the parameter to.
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  public function set_parameters(Array $parameters) {
    foreach ($parameters as $name => $value) {
      $this->set_parameter($name, $value);
    }

    // Allow chaining.
    return $this;
  }

  /**
   * Determine if this query may be cached.
   *
   * @return Boolean
   */
  public function may_cache() {
    // Cache will never be valid if the validity period is not positive.
    return $this->cache_validity > 0 && !empty($this->cache_table) && $this->method == 'GET';
  }

  /**
   * Allow any module to alter this query. (They are not alterable by default.)
   *
   * Consumers of rest_api_query objects (or sub-classes) must call this
   * explicitly if a query should be alterable.  This is most useful just before
   * execution.  E.g.
   *
   * @code $query->alter()->execute();
   *
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  public function alter() {
    // Give modules access to alter *all* implementations (e.g. extensions or sub
    // -classes) of rest_api_query.
    drupal_alter('rest_api_query', $this, $this->method, $this->resource, $this->identifier);

    // Allow modules to alter only a specified sub-class.
    drupal_alter(get_class($this), $this, $this->method, $this->resource, $this->identifier);

    // Allow chaining.
    return $this;
  }

  /**
   * Executes the query by requesting data via HTTP from the API's access point.
   *
   * @return $this rest_api_query
   *    The return value is chainable so that the "error", "code" and "success"
   *    properties can be readily accessible.  For example;
   *      @code
   *      if ($query->execute()->success) {
   *        // Success!
   *      }
   *      @endcode
   */
  public function execute() {
    // Prepare to be re-executed.
    $this->reset();

    // Get the URL using the getter method.
    $url = $this->__get('url');

    // Stop trying if getting the URL failed.
    if (empty($url)) {
      return $this;
    }

    if ($this->may_cache()) {
      // Generate a hash that will serve as a unique identifier for this query.
      $cache_id = $this->__get('cache_id');

      // Get the cached response.
      $cache = cache_get($cache_id, $this->cache_table);

      // Have we cached this query?  Is it less than one hour old?
      if (isset($cache->data) && $cache->created > (time() - $this->cache_validity)) {
        $this->response = $cache->data;

        // Include the cache info in the response, but do not duplicate the data.
        unset($cache->data);
        $this->cache = $cache;
      }
    }

    // Get the response, if it has not already been got.
    if (empty($this->response)) {
      // drupal_to_js() does not encode single-quote/apostrophe characters
      // correctly.  Use PHP's json_encode() instead.
      // @see http://drupal.org/node/1086098
      $data = empty($this->data) ? NULL : json_encode($this->data);

      // @todo Postponed: Support rest_client_request() in rest_client.module?
      $this->response = drupal_http_request($url, $this->headers, $this->method, $data);

      // Cache the response, but only if there is no error.
      if ($cache_id && empty($this->response->error)) {
        $cache = cache_set($cache_id, $this->response, $this->cache_table, CACHE_TEMPORARY);
      }

      // Anything other than a GET changes data on the remote data store.  Clear
      // any cached data of the resource type.  rest_api_query sub-classes are
      // responsible for clearing caches of any related resource types.
      if ($this->method != 'GET') {
        $this->clear_cache($this->resource);
      }
    }

    // Set this object's status code and error message, if any.
    $this->code = $this->response->code;
    if (isset($this->response->error)) {
      $this->error = $this->response->error;
    }

    // Set the success value.
    $this->success = empty($this->error);

    // If there is any data in the response, attempt to decode it.
    if (!empty($this->response->data)) {
      $this->result = json_decode($this->response->data);
    }

    // Allow chaining.
    return $this;
  }

  /**
   * Generates a string that serves as a unique identifier for this query.
   *
   * This is used primarily to cache responses.
   *
   * The string is of the form "URL:METHOD:HASH" where URL is the full URL for
   * this query, METHOD is the HTTP method (e.g. "GET") and HASH is an md5 hash
   * of the serialized headers and data property.
   *
   * @return String
   *    The uniquely identifying string.
   */
  public function get_cache_id() {
    // Generate a hash that will serve as a unique identifier for this query.
    $md5_hash = md5(serialize(array($this->headers, $this->data)));
    return implode(':', array($this->__get('url'), $this->method, $md5_hash));
  }

  /**
   * Clear cached query responses, optionally just for a given resource type.
   *
   * @param $resource_type String
   *    The resource type for which to clear cached responses.
   *
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  public function clear_cache($resource_type = NULL) {
    $cache_id = $this->__get('access_point');
    if ($resource_type) {
      $cache_id .= "/$resource_type";
    }
    cache_clear_all($cache_id, $this->cache_table, TRUE);

    // Allow chaining.
    return $this;
  }

  /**
   * Reset the rest_api_query object so that it may be re-executed.
   *
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  public function reset() {
    $this->response = NULL;
    $this->code = NULL;
    $this->error = FALSE;
    $this->result = NULL;
    $this->cache_id = NULL;

    // Allow chaining.
    return $this;
  }

  /**
   * Debug with rest_api_query_var_export() (aka rve()).
   *
   * @see rest_api_query_var_export()
   *
   * @param $variable Mixed
   *    What variable to debug:
   *    - FALSE (default);  Debug the request as a text string.
   *    - TRUE;  Debug the query variable as an object.
   *    - Otherwise;  The variable to be debugged.
   * @param $name Mixed
   *    The identifier to use for this variable in the debug output:
   *    - NULL (default);  Use the URL of the request as the identifier.
   *    - TRUE;  Identify the variable as 'rest_api_query::debug()'.
   *    - String;  A custom string to identify this debug message.
   * @return $this rest_api_query
   *    The return value is chainable.
   */
  public function debug($variable = FALSE, $name = NULL) {
    // Initialize the variable.
    if (is_bool($variable)) {
      $variable = $variable ? $this : $this->debug_string();
    }

    // Initialize the name.
    if (!$name) {
      $name = $this->__get('url');
    }
    else if ($name === TRUE) {
      $name = 'rest_api_query::debug()';
    }

    // Let rest_api_query_var_export() to render the debug output.
    rest_api_query_var_export($variable, $name);

    // Allow chaining.
    return $this;
  }

  /**
   * Build a text-only summary of the the request for debugging purposes.
   *
   * @return String
   *    A string representing the HTTP request and response.
   */
  public function debug_string() {
    $return = $this->response->request;
    if ($this->data) {
      $return .= "\n\n";
    }
    $return .= $this->code . ' ';
    if (!empty($this->error)) {
      $return .= $this->error . "\n";
    }
    if (isset($this->response->data)) {
      $return .= trim($this->response->data);
    }
    return $return;
  }
}

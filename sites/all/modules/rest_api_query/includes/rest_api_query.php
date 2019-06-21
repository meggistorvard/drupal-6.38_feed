<?php
/**
 * @file
 * Hook definitions and example implementations.
 */

/**
 * Defines REST API Schemas.
 *
 * @return Array(String => stdClass)
 *    The index is the machine name of the API schema.  The module name is
 *    usually suitable, unless the module exposes multiple schemas.  The value is
 *    an object with the following properties;
 *      - "types" Array(String => String):
 *          The index is *singular* name of the type.  The value is the *plural*
 *          name of the type.  "Type" is often referred to as "resource".
 *      - "class" optional String:
 *          The name of this module's class which inherits from the
 *          rest_api_query class.
 *          Defaults to "[schema]_rest_api_query" where "[schema]" is the machine
 *          name of the schema.
 *      - "file" optional String:
 *          The file path name where the class is defined, relative to the
 *          module's root directory.  E.g. "includes/foobar_query.class.api".
 *          Defaults to "[class].class.inc" where "[class]" is the name of the
 *          class.
 */
function hook_rest_api_schemas() {
  $schemas = array();

  $schemas['foobar'] = (Object) array(
    'name' => t('Foo Bar'),
    'class' => 'foobar_api_query',
    // class foobar_api_query is declared in classes.inc
    'file' => 'includes/classes.inc',
    'types' => array(
      // SINGULAR => PLURAL
      'foo' => 'foos',
      'bar' => 'bars',
      'barry' => 'barries',
    ),
  );

  return $schemas;
}

/**
 * Allows any module to modify any module's REST API Schemas.
 *
 * This can be used to override or extend a REST API with a different class or
 * override some details like the access point or port.
 *
 * @param &$schemas Array(stdClass Object)
 *    The schema objects, as per hook_rest_api_schemas().  Take it by reference
 *    and modify it.
 */
function hook_rest_api_schemas_alter(&$schemas) {
  // Use class custom_foobar_api_query instead of class foobar_api_query.
  $schemas['foobar']->class = 'custom_foobar_api_query';
  // custom_foobar_api_query is defined in custom_foobar_api_query.class.inc.
  $schemas['foobar']->file = 'custom_foobar_api_query.class.inc';
}

/**
 * Allows any module to alter REST API Queries.
 *
 * This is not called by default.  The consumer of a rest_api_query object (or a
 * sub-class) must explicitly call rest_api_query::alter() in order to allow
 * other modules to alter queries.
 *
 * rest_api_query() allows altering if $get_result = TRUE, just before it
 * executes a query.
 *
 * @param &$query rest_api_query object
 *    The query object to be altered.  Take it by reference and modify it.
 * @param $class String
 *    The name of the rest_api_query sub-class.
 * @param $http_method String
 *    E.g. GET, PUT, POST, DELETE
 * @param $api_resource String
 *    The type of resource that is being queried.  E.g. foo, bar, barry
 * @param $id String or Integer
 *    The ID of a specific resource that is being queried, for load, delete and
 *    update actions.
 */
function hook_rest_api_query_alter(&$query, $http_method, $api_resource, $id = NULL) {
  // Do not check $http_method.  rest_api_query::may_cache() already does that.
  if ($class == 'foobar_api_query' && $api_resource == 'foos') {
    // foos almost never change.  Cache them for a week.
    $query->cache_validity = 7 * 24 * 60 * 60;
  }
}

/**
 * Allows any module to alter queries for a specified sub-class.
 *
 * This is identical to hook_rest_api_query_alter, except it;
 *    - does not have the $class parameter
 *    - is named of the form hook_SUBCLASS_alter() where SUBCLASS is $class;  The
 *      name of the rest_api_query sub-class.
 *
 * Use this to alter queries belonging to a specific sub-class.
 *
 * @param &$query rest_api_query object
 *    The query object to be altered.  Take it by reference and modify it.
 * @param $http_method String
 *    E.g. GET, PUT, POST, DELETE
 * @param $api_resource String
 *    The type of resource that is being queried.  E.g. foo, bar, barry
 * @param $id String or Integer
 *    The ID of a specific resource that is being queried, for load, delete and
 *    update actions.
 */
function hook_foobar_api_query_alter(&$query, $http_method, $api_resource, $id = NULL) {
  // @see hook_rest_api_query_alter()
}

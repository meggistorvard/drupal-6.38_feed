<?php
/**
 * @file
 * Expose classes to facilitate running unit tests with SimpleTest module.
 */

/**
 * Extends Simpletest module's DrupalUnitTestCase for modules that implement RAQ.
 *
 * Unit tests are fast and cheap to spin up and tear down, but do not have
 * database tables, and thus do not support caching.  DrupalWebTestCase tests do
 * have tables but are far more expensive to run due to the large amounts of data
 * duplication required.  RestApiAbstractTestCase allows modules which consume
 * REST APIs to run functional unit tests, even without the cache tables by
 * temporarily disabling REST API Query module's caching features.
 */
abstract class RestApiAbstractTestCase extends DrupalUnitTestCase {
  /**
   * The schema that this test-case is unit-testing.
   *
   * The garbage collector and many methods require this.
   */
  protected $schema;

  /**
   * The default options for the various CRUD-related restApiTest*() methods.
   */
  protected $crud_test_default_options = array(
    'text_prop' => 'name',
    'text_len' => FALSE,
    'properties' => TRUE,
  );

  /**
   * A RestApiGarbageCan to collects REST API objects & delete them.
   */
  protected $garbage;

  function setUp() {
    $this->garbage = new RestApiGarbageCan($this->schema);
  }

  /**
   * Re-enable REST API Query's caching features.
   */
  function tearDown() {
    $this->garbage->tipOut($this);
  }

  /**
   * Gets the count of items of type $type, if such a feature is supported.
   *
   * Override this method if the count feature is supported.  Various CRUD tests
   * will use it to assert the count has changed as expected.  It is highly
   * recommended that REST API consumption modules implement this if they are
   * using RestApiAbstractTestCase methods for unit tests.
   *
   * @param $type String
   *    The resource type of the REST API consumption function.
   * @return Mixed
   *    Integer; The feature is supported. The return value is count of items.
   *    Boolean FALSE;  The feature is not supported.
   */
  function restApiCount($type) {
    return FALSE;
  }

  /**
   * Tests that each type has an index function which returns an array.
   *
   * @return $this
   *    The return value is chainable.
   */
  function restApiTestIndexes() {
    // Get plural types.
    foreach (get_rest_api_type($this->schema) as $type) {
      // Build the function name.
      $function = $this->schema . '_rest_api_' . $type;
      $this->assertTrue(is_array($function()),
        t('!function(), to index !type, returns an array.', array('!function' => $function, '!type' => $type)));
    }
    return $this;
  }

  /**
   * Tests for appropriate responses to requests to load invalid IDs.
   *
   * @return $this
   *    The return value is chainable.
   */
  function restApiTestInvalidIds() {
    $invalid_ids = array(-1, 0, 'random-test-id-' . rand());

    // Iterate over each invalid ID.
    foreach ($invalid_ids as $id) {
      // Prepare the message text.
      $case = t('invalid issue ID (!id)', array('!id' => $id));

      // Prepare the parameter.
      $param = $action == 'update' ? (Object) compact('id') : $id;

      // Get singular types and iterate.
      foreach (get_rest_api_type($this->schema, NULL, TRUE) as $type) {

        // Attempt all conventional actions that take an ID as the single parameter.
        foreach (array('load', 'update', 'delete') as $action) {
          $this->restApiTestInvalidId($type, $action, $param, $case);
        }
      }
    }

    return $this;
  }

  /**
   * Tests invocation of an API consumption-function with an invalid ID.
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $action String
   *    One of 'load', 'update' or 'delete'.
   * @param $param Mixed
   *    The parameter for restApiHelper(); An ID or an object with an "id"
   *    parameter.
   *    @see restApiHelper()
   * @param $case String
   *    Translated text for this case.
   *    @see restApiHelper()
   * @return $this
   *    The return value is chainable.
   */
  function restApiTestInvalidId($type, $action, $param, $case) {
    // "load" should return NULL when it fails.  "delete" and "update" should
    // return FALSE.
    $expect = $action == 'load' ? 'NULL' : 'FALSE';
    return $this->restApiHelper($type, $action, $param, $expect, $case);
  }

  /**
   * Tests item create, load (read), update and optionally, delete.
   *
   * Example usage of this method:
   * @code
   *  $create = array(
   *    'distance' => 99.9,
   *    'terrain' => 'mountainous',
   *  );
   *
   *  $update = array(
   *    'distance' => 1009.99,
   *    'terrain' => 'flat',
   *  );
   *
   *  extract($this->restApiTestCrud('barry', $create, $update));
   * @endcode
   *
   * To test delete, the sub-class should implement restApiTestDelete() as an
   * alias of garbageDelete().  E.g.
   * @code
   *  function garbageDelete($type, $id, $object) {
   *    $this->restApiTestDelete($type, $id, $object);
   *  }
   * @endcode
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $create Array
   *    A REST API item to be created, as an array instead of a stdClass.  The
   *    "id" parameter must not be set.  The "name" parameter is optional.
   * @param $update Array
   *    A REST API item to update the created item, as an array instead of a
   *    stdClass.  The "id" parameter will be set internally.  The "name"
   *    parameter is optional.
   * @param $options Array
   *    This function will, by default, set the "name" parameter on both the
   *    update and create items to an automatically generated identifiable text
   *    string, if the "name" parameter is not already set.
   *    Options modifying the behaviour;
   *    - 'text_prop' Mixed;
   *      The property to set the automatic text to.  FALSE to disable automatic
   *      text-property setting.  Defaults to "name".
   *    - 'text_len' Mixed;
   *      The length to truncate the automatic text to.  FALSE to not truncate.
   *      Defaults to FALSE.
   *    - 'properties' Mixed;
   *      TRUE to test that all the properties both $create and $update match all
   *      the properties of the $created and $updated items.  Or an array of
   *      specific properties to match.
   *    @see $this->crud_test_default_options
   * @return Array
   *    The "created" and "updated" items, keyed respectively.
   */
  function restApiTestCrud($type, $create, $update, $options = array()) {
    // Test create.
    $created = $this->restApiTestCreate($type, $create, $options);

    // Test update, which also tests load.
    $update['id'] = $created->id;
    $updated = $this->restApiTestUpdate($type, $update, $options);

    // Allow sub-classes to test deletion via the Garbage API.
    // Do not add the item to the garbage collector if there is no delete
    // function for this type.
    if (function_exists($this->restApiConsumptionFunctionName($type, 'delete'))) {
      $this->garbage->garbage($type, $created->id, $updated);
    }

    return compact('created', 'updated');
  }

  /**
   * Test item-creation via the REST API.
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $array Array
   *    A REST API item to be created, as an array instead of a stdClass.  The
   *    "id" parameter must NOT be set.  The "name" parameter is optional.
   * @param $options Array
   *    @see restApiTestCrud()
   */
  function restApiTestCreate($type, $array, $options = array()) {
    $options = array_merge($this->crud_test_default_options, $options);

    // Get the count of items BEFORE adding the test item.
    $before = $this->restApiCount($type);

    // Set the name property of each object.
    $this->restApiGeneratedTextProperty($array, $type, FALSE, $options);

    // Create an item.
    // Object-SAVE with no ID should always work the same as object-CREATE.
    $created = $this->restApiTestSave($type, $array, $options);

    // Get the count of items AFTER creating the test item, and verify the count
    // has incremented.
    if (is_integer($before)) {
      $this->assertTrue($this->restApiCount($type) == 1 + $before,
        t('Creating one !type item increments the count of !type items by one.', array('!type' => $type)));
    }

    return $created;
  }

  /**
   * Test item-save via the REST API.
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $array Array
   *    A REST API item to save, as an array instead of a stdClass.  The "id"
   *    parameter MAY be set.  The "name" parameter is optional.
   * @param $options Array
   *    @see restApiTestCrud()
   */
  function restApiTestSave($type, $array, $options = array()) {
    $options = array_merge($this->crud_test_default_options, $options);

    // Save the object
    $save = (Object) $array;
    $saved = $this->restApiConsumptionFunctionInvoke($type, 'save', $save);
    $this->restApiAssertSavedIdAndProperties($saved, $array, $options['properties'], $type);

    // Allow the caller to do more tests on the response object.
    return $saved;
  }

  /**
   * Test item-update via the REST API.
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $array Array
   *    A REST API item to update, as an array instead of a stdClass.  The "id"
   *    parameter MUST be set.  The "name" parameter is optional.
   * @param $options Array
   *    @see restApiTestCrud()
   */
  function restApiTestUpdate($type, $array, $options = array()) {
    $options = array_merge($this->crud_test_default_options, $options);

    // Get the count of items BEFORE updating the test item.
    $before = $this->restApiCount($type);

    // Set the name property of each object.
    $this->restApiGeneratedTextProperty($array, $type, TRUE, $options);

    $update = (Object) $array;

    // Test the return value of the save function for an update.
    $this->restApiHelper($type, 'save', $update, 'TRUE', t('a valid issue-update'));

    // Get the count of items AFTER updating the test item, and verify the count
    // has NOT changed.
    if (is_integer($before)) {
      $this->assertTrue($this->restApiCount($type) == $before,
        t('Creating one !type item does not change the count of !type items.', array('!type' => $type)));
    }

    // Load the updated item and test it's properties.
    $updated = $this->restApiConsumptionFunctionInvoke($type, 'load', $update->id);
    $this->restApiAssertSavedIdAndProperties($updated, $array, $options['properties'], $type);

    return $updated;
  }

  /**
   * Test item-delete via the REST API.
   *
   * A sub-class may set this as an alias of garbageDelete() to test item-deletion using garbage->tipOut()
   *
   * @see garbageDelete()
   */
  function restApiTestDelete($type, $id, $object = NULL) {
    // Get the count of items BEFORE deleting the item.
    $before = $this->restApiCount($type);

    // Prepare the text for this case.
    $case = t('a deleted !type', array('!type' => $type));

    // Deleting the item should be successful the first time.
    $this->restApiHelper($type, 'delete', $id, 'TRUE', $case)
      // Deleting the same item should fail on subsequent attempts.
      ->restApiHelper($type, 'delete', $id, 'FALSE', $case);

    // Get the count of items AFTER deleting the item, and verify the count has
    // NOT changed.
    if (is_integer($before)) {
      $this->assertTrue($this->restApiCount($type) + 1 == $before,
        t('Deleting one !type item decrements the count of !type items by one.', array('!type' => $type)));
    }

    return $this;
  }

  /**
   * *Strictly* check to see if a value is FALSE.
   *
   * DrupalTestCase::assertFalse() only checks that $value is FALSEish.
   *
   * @param $value
   *   The value on which the assertion is to be done.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  function assertFalse($value, $message = '', $group = 'Other') {
    return $this->assert(FALSE === $value, $message ? $message : t('Value is FALSE'), $group);
  }

  /**
   * Strictly check to see if a value is integer zero.
   *
   * @param $value
   *   The value on which the assertion is to be done.
   * @param $message
   *   The message to display along with the assertion.
   * @param $group
   *   The type of assertion - examples are "Browser", "PHP".
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  function assertZero($value, $message = '', $group = 'Other') {
    return $this->assert(0 === $value, $message ? $message : t('Value is zero'), $group);
  }

  /**
   * Assert that an object has an ID attribute that is a positive integer.
   *
   * @param $object stdClass
   *    Object representing a resource type.
   * @param $function_call String
   *    The function-call which returned $object.  This is the function-call
   *    which is being unit-tested.  E.g. "custom_foo_load()"
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertId($object, $function_call) {
    $args = array('!function' => $function_call);
    $message = t('!function returns an object with an "id" parameter that is a positive integer.', $args);
    $this->assertTrue(is_integer($object->id) && $object->id > 0, $message);
    return $this;
  }

  /**
   * Asserts that a date-time strings is "now", or close to now.
   *
   * @param $time String
   *    The date-time value in any string format supported by strtottime()
   *    @see strtotime()
   * @param $message String
   *    The translated text string for this test/assertion.
   * @param $fuzziness Boolean or Integer
   *    Since there maybe significant network delays and/or time-zone differences
   *    between Drupal and the REST API service, $fuzziness allows for there to
   *    be some discrepancy between $time and "now".
   *
   *    Set to FALSE to disable allowance for any difference at all between "now"
   *    and $time.  The caller is responsible for handing any network delays or
   *    differences in timezones.  The $now parameter might be useful for this.
   *
   *    Set to an Integer to override the amount of difference that is permitted.
   *
   *    By default, the allowed time difference is 2 days for time-zone
   *    differences.
   * @param $now String or Integer
   *    $now defaults to time().  Optionally set $now to use something else.
   *    Both unix timestamps and strings are supported.  Unix timestamps must be
   *    actual integers, not numeric strings.
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertTimeNow($time, $message, $fuzziness = TRUE, $now = NULL) {
    return $this->restApiAssertEqualTime($now ? $now : time(), $time, $message, $fuzziness);
  }

  /**
   * Asserts that two strings each representing a date-time value are equal.
   *
   * Date-time values represented by strings can be formatted in an almost
   * unlimited number of unique formats.  So comparing the strings alone can not
   * determine equality.  PHP's strtottime() function parses most common
   * date-time string formats to unix timestamps so that they can be compared as
   * integers.  This is a reliable method of asserting that two date-time strings
   * are equal or not, as long as the times that the strings represent are a
   * point in time that is covered by the unix epochs.
   *
   * @todo Postponed: Use PHP DateTime objects instead so that points in time outside the
   * unix epoch can be supported.
   *
   * @param $time_a String or Integer
   *    The first date-time value to compare.
   *    Integer is for internal use only.
   *    @see rest_api_query::restApiAssertTimeNow()
   * @param $time_b String
   *    The second date-time value to compare.
   * @param $message String
   *    The translated text string for this test/assertion.
   * @param $fuzziness Boolean or Integer
   *    This is primarily for internal use only.
   *    @see rest_api_query::restApiAssertTimeNow()
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertEqualTime($time_a, $time_b, $message, $fuzziness = FALSE) {
    $a = is_integer($time_a) ? $time_a : strtotime($time_a);
    $b = strtotime($time_b);
    $this->assertTrue($a !== FALSE && $b !== FALSE,
      t('Invalid times for: !message', array('!message' => $message)));

    // Do the times need to match exactly?
    if (!$fuzziness) {
      // Yes.
      $this->assertEqual($a, $b, $message);
    }
    else {
      // No;  Allow for slight differences in the times.
      if (!is_numeric($fuzziness)) {
        // Default fuzziness of 2 days
        $fuzziness = 2 * 24 * 60 * 60;
      }

      // Check A is no more than $fuzziness greater than nor less than B.
      $this->assertTrue(($a - $fuzziness) < $b && $b < ($a + $fuzziness), $message);
    }

    return $this;
  }

  /**
   * Asserts that a date-time property of two objects represents the same value.
   *
   * @see restApiAssertEqualTime()
   *
   * @param $object_a Array
   *    Array representing an item exposed via the REST API.
   * @param $object_b stdClass
   *    Another object of the same REST API type.
   * @param $property String
   *    The property name to compare on each object.
   * @param $function String
   *    The function which took $object_a as a parameter and returned or changed
   *    $object_b.  This is the function which is being unit-tested.
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertEqualTimeProperty($object_a, $object_b, $property, $function, $fuzziness = FALSE) {
    // Prepare arguments for t().
    $args = array(
      '!function' => $function,
      '!property' => $property,
    );
    $message = t('!function() returns an object with a "!property" date/time property that matches the parameter\'s "!property" date/time property.', $args);

    // Assert the properties are actually set.
    $this->assertTrue(isset($object_a[$property]) && isset($object_b->{$property}), $message);

    // Assert the properties are equal.
    $this->restApiAssertEqualTime($object_a[$property], $object_b->{$property}, $message, $fuzziness);

    return $this;
  }

  /**
   * Assert that two objects have a property and that the property is identical.
   *
   * @param $object_a stdClass
   *    Object representing an item exposed via the REST API.
   * @param $object_b stdClass
   *    Another object of the same REST API type.
   * @param $properties Array
   *    Property names to compare on each object.  Keys of the array are ignored.
   * @param $function String
   *    The function which took $object_a as a parameter and returned or changed
   *    $object_b.  This is the function which is being unit-tested.
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertEqualProperties($object_a, $object_b, $properties, $function) {
    // Prepare arguments for t().
    $args = array('!function' => $function);

    // For each of the properties that needs to be tested.
    foreach ($properties as $property) {
      $args['!property'] = $property;
      $message = t('!function() returns an object with a "!property" property that matches the parameter\'s "!property" property.', $args);

      // Assert the properties are actually set.
      $this->assertTrue(isset($object_a->{$property}) && isset($object_b->{$property}), $message);

      // Assert the properties are equal.
      $this->assertEqual($object_a->{$property}, $object_b->{$property}, $message);
    }

    return $this;
  }

  /**
   * Asserts that properties of an object are arrays.
   *
   * @param $object stdClass
   *    Object representing an item exposed via the REST API.
   * @param $properties Array
   *    Property names to compare on each object.  Keys of the array are ignored.
   * @param $function_call String
   *    The function-call which returned $object.  This is the function-call
   *    which is being unit-tested.  E.g. "custom_foo_load()"
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertArrayProperties($object, $properties, $function_call) {
    $args = array('!function' => $function_call);
    foreach ($properties as $property) {
      $args['!property'] = $property;
      $this->assertTrue(is_array($object->{$property}),
        t('!function returns an object with a "!property" property which is an array.', $args));
    }
    return $this;
  }

  /**
   * Asserts that a saved item has the correct properties.
   *
   * @see restApiAssertEqualProperties()
   * @see restApiAssertId()
   * a valid ID and the same properties as the parameter that created/updated it.
   *
   * @param $saved stdClass
   *    An item returned by a REST API consumption function.
   * @param $parameter_array Array
   *    The parameter to the REST API save, create or update function, as an
   *    array instead of an object.
   * @param $properties Mixed
   *    TRUE: Check all of the properties of $parameter_array
   *    Array: @see restApiAssertEqualProperties()
   * @param $type String
   *    The type of item.  This is just used to generate the SAVE function name
   *    for the Simpletest case text.
   * @return $this
   *    The return value is chainable.
   */
  function restApiAssertSavedIdAndProperties($saved, $parameter_array, $properties, $type) {
    // Should ALL properties from the create object should be equal?
    if ($properties === TRUE) {
      // Test all properties of the saved object.
      $properties = array_keys($parameter_array);
    }

    // Test the updated item.
    $parameter = (Object) $parameter_array;
    $function = $this->restApiConsumptionFunctionName($type, 'save');
    $this->restApiAssertId($saved, "$function()")
      ->restApiAssertEqualProperties($saved, $parameter, $properties, $function);

    return $this;
  }

  /**
   * Tests type of the return value for conventional REST API consumer functions.
   *
   * Uses basic asserts like assertNull, assertTrue. assertFalse.  The caller is
   * responsible for ensuring the function exists.
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $action String
   *    The REST API Query action.  E.g. "load", "index", "save", "delete" etc.
   * @param $parameters Mixed
   *    @see restApiConsumptionFunctionInvoke()
   * @param $expect String
   *    The expected return value as a *STRING*.  E.g. "NULL", "FALSE" or "TRUE",
   *    and not NULL, FALSE or TRUE.
   * @param $case_text String
   *    Translated text about the uniqueness of this test case.  This becomes
   *    "!case" in t("!function() returns !expected for !case.").
   * @return $this
   *    The return value is chainable.
   */
  function restApiHelper($type, $action, $parameters, $expect, $case_text) {
    // Get the function name and call it.
    $response = $this->restApiConsumptionFunctionInvoke($type, $action, $parameters);

    $args = array(
      '!function' => $this->restApiConsumptionFunctionName($type, $action),
      '!expected' => $expect,
      '!case' => $case_text,
    );
    $message = t('!function() returns !expected for !case.', $args);

    $method = 'assert' . ucfirst(strtolower($expect));
    $this->$method($response, $message);

    return $this;
  }

  /**
   * Invoke a REST API consumption function.
   *
   * @param $parameters Mixed
   *    Arguments to pass to the SCHEMA_TYPE_ACTION() function that is being
   *    tested.  If $parameters is not an array, $parameters is assumed to be the
   *    first and only parameter.  Thus, for SCHEMA_TYPE_ACTION() functions which
   *    take only one parameter for ID, $parameters can simply be the ID of the
   *    item to test.  This is usually a positive integer, but some types have
   *    machine name identifiers as well as numeric IDs.
   * @param $type String
   *    @see restApiConsumptionFunctionName()
   * @param $action String
   *    @see restApiConsumptionFunctionName()
   * @return Mixed
   *    The return value of the REST API consumption function.  Usually stdClass,
   *    TRUE.  Or if there is an error; FALSE or NULL.
   */
  function restApiConsumptionFunctionInvoke($type, $action, $parameters = NULL) {
    // Normalize $parameters.
    if (!isset($parameters)) {
      $parameters = array();
    }
    else if (!is_array($parameters)) {
      $parameters = array($parameters);
    }

    // Get the name of the function to invoke.
    $function = $this->restApiConsumptionFunctionName($type, $action);

    // Call it.
    return call_user_func_array($function, $parameters);
  }

  /**
   * Assembles the coventional name of a REST API consuming function.
   *
   * @param $type String
   *    The resource type of the REST API consumption function.
   * @param $action String
   *    The action of the REST API consumption function.
   * @return String
   *    The conventional name of the function.
   */
  function restApiConsumptionFunctionName($type, $action) {
    return "{$this->schema}_rest_api_{$type}_{$action}";
  }

  /**
   * Helper function to set a "name" property of an array.
   *
   * @param $type String
   *    The resource type of the REST API consumption function.
   * @param $array Array
   *    The array upon which to set the generated text property.
   * @param $update Boolean
   *    TRUE if the text is for an update instead of a create.
   * @param $options Array
   *    Two options are supported;
   *    - 'text_prop' Mixed;
   *      The property to set the automatic text to.  FALSE to disable automatic
   *      text-property setting.  Defaults to "name".
   *    - 'text_len' Mixed;
   *      The length to truncate the automatic text to.  FALSE to not truncate.
   *      Defaults to FALSE.
   * @return $this
   *    The return value is chainable.
   */
  function restApiGeneratedTextProperty(&$array, $type, $update = FALSE, $options = array()) {
    $options = array_merge($this->crud_test_default_options, $options);

    // Get the options.
    $key = $options['text_prop'];
    $leng = $options['text_len'];

    // Set the name if it is not already set, and the $len parameter is not 0
    // or FALSEish.
    if ($key && !$array[$key]) {
      $args = array(
        '!type' => $type,
        '!action' => $update ? t('updated') : t('created'),
        '!random' => rand(0, 99999),
        '!schema' => get_rest_api_schema($this->schema, 'name'),
      );

      // Set the name of the item.
      $array[$key] = t("Test !type #!random, !action by the !schema REST API to Drupal integration module's unit testing framework", $args);

      // Is there a limit to the name length?
      if ($len) {
        // Truncate the name to length $len.
        $array[$key] = substr($array[$key], 0, $len);
      }
    }

    return $this;
  }
}

/**
 * A garbage can is for items that are to be deleted when tests complete.
 */
class RestApiGarbageCan {
  public $bin = array();
  public $schema;

  function __construct($schema) {
    $this->schema = $schema;

    // The order is significant.  At least for Redmine's REST API, because
    // deleting a project deletes the project's issues.
    foreach (get_rest_api_type($this->schema, NULL, TRUE) as $type) {
      $this->bin[$type] = array();
    }
  }

  /**
   * Add an item to the garbage bin.
   *
   * @param $type String
   *    The resource type of the item.
   * @param $id String or Integer
   *    The ID of the item.
   * @param $object stdClass
   *    The actual object being garbage-collected.
   * @return $this
   *    The return value is chainable.
   */
  function garbage($type, $id, $object = TRUE) {
    $this->bin[$type][$id] = $object;
    return $this;
  }

  /**
   * Empty the garbage bin.
   *
   * "empty" is a better name for this method, but it can not be used since it is
   * a keyword in PHP.
   *
   * @return $this
   *    The return value is chainable.
   */
  function tipOut(DrupalTestCase $manager = NULL) {
    // Respect the order of the types to support any dependencies across
    // different types.
    // @see $this->setUp()
    foreach ($this->bin as $type => $items) {

      // Delete in reverse order to respect any dependencies between items of the
      // same type.
      foreach (array_reverse($items, TRUE) as $id => $object) {
        if (method_exists($manager, 'garbageDelete')) {
          $manager->garbageDelete($type, $id, $object);
        }
        else {
          $this->delete($type, $id, $object);
        }
      }
    }

    // Reset the garbage bin.
    $this->bin = array();
    return $this;
  }

  /**
   * Delete an item.  This is primarily used by the garbage collector.
   *
   * @param $type String
   *    The type supported by the schema.  E.g. "issue", "project", "user" etc.
   * @param $id String or Integer
   *    The ID of the item.
   * @param $discard Mixed
   *    The default implementation does not utilize this parameter, but
   *    sub-classes of RestApiAbstractTestCase are likely to use it to turn
   *    garbageDelete() into an alias of restApiTestDelete() in order to test
   *    item deletion.
   * @return $this
   *    The return value is chainable.
   */
  function delete($type, $id, $discard) {
    // Ignore any failures.
    rest_api_query($this->schema, $type, 'delete', $id);
    return $this;
  }
}

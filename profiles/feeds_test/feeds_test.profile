<?php
// $Id: feeds_test.profile,v 1.8 2010/09/12 04:59:05 alexb Exp $

/**
 * Return an array of the modules to be enabled when this profile is installed.
 */
function feeds_test_profile_modules() {
  return array(
    'help',
    'menu',
    'taxonomy',
    'dblog',
    'ctools',
    'libraries',
    'job_scheduler',
    'feeds',
    'feeds_ui',
    'feeds_fast_news',
    'feeds_import',
    'feeds_news',
    'schema',
    'data',
    'data_ui',
    'content',
    'text',
    'number',
    'date_api',
    'date',
    'link',
    'locale', // If locale is not enabled on host system, locale mapping tests fail with exceptions :P
    'views',
    'views_ui',
    'devel',
  );
}

/**
 * Return a description of the profile for the initial installation screen.
 */
function feeds_test_profile_details() {
  return array(
    'name' => 'Feeds Test Site',
    'description' => 'A site designed for testing Feeds module.'
  );
}

/**
 * Return a list of tasks that this profile supports.
 */
function feeds_test_profile_task_list() {
}

/**
 * Implementation of hook_profile_tasks().
 */
function feeds_test_profile_tasks(&$task, $url) {
  drupal_set_message(t('IMPORTANT: Please manually install Simpletest now. Follow install instrutions in its INSTALL.txt file!'));
  // Update the menu router information.
  menu_rebuild();
}

/**
 * Implementation of hook_form_alter().
 */
function feeds_test_form_alter(&$form, $form_state, $form_id) {
  if ($form_id == 'install_configure') {
    // Set default for site name field.
    $form['site_information']['site_name']['#default_value'] = 'Feeds Test Site';
    $form['site_information']['site_mail']['#default_value'] = 'admin@'. $_SERVER['HTTP_HOST'];
    $form['admin_account']['account']['name']['#default_value'] = 'admin';
    $form['admin_account']['account']['mail']['#default_value'] = 'admin@'. $_SERVER['HTTP_HOST'];
  }
}

<?php

/**
 * @file
 * Theme hooks.
 */

/**
 * Theme implementation for boston_college_video.
 */
function theme_boston_college_video($variables) {
  module_load_include('inc', 'islandora', 'includes/metadata');
  $object = &$variables['object'];
  $variables['metadata'] = islandora_retrieve_metadata_markup($object);
  $variables['description'] = islandora_retrieve_description_markup($object);
  // Core fieldset JS.
  drupal_add_js('misc/form.js');
  drupal_add_js('misc/collapse.js');
  if (isset($object['STREAM'])) {
    $viewer = "<iframe src=\"{$object['STREAM']->url}\" width=\"400px\" height=\"305px\"></iframe>";
  }
  else {
    $viewer = '';
  }
  return array(
    // Magic dimensions to work with STREAM. Using markup to avoid firefox self
    // closing iframe bug.
    'viewer' => array(
      '#markup' => $viewer,
    ),
    'description' => array('#markup' => $variables['description']),
    'metadata' => array('#markup' => $variables['metadata']),
  );
}

<?php

/**
 * @file
 * Handles the display/submission of the admin settings form for this module.
 */

/**
 * Defines the admin settings form.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The Drupal form definition.
 */
function boston_college_admin_settings_form(array $form, array &$form_state) {
  form_load_include($form_state, 'inc', 'boston_college', 'includes/admin.form');
  module_load_include('inc', 'islandora', 'includes/utilities');
  $get_default_value = function ($name, $default) use (&$form_state) {
    return isset($form_state['values'][$name]) ? $form_state['values'][$name] : variable_get($name, $default);
  };

  $fop = $get_default_value('boston_college_fop_executable', '/usr/bin/fop');
  $pdftk = $get_default_value('boston_college_pdftk_executable', '/usr/bin/pdftk');

  $form['#prefix'] = '<div id="boston-college-admin-wrapper">';
  $form['#suffix'] = '</div>';
  $form['boston_college_fop_executable'] = array(
    '#type' => 'textfield',
    '#title' => t('Apache FOP Executable'),
    '#description' => t('Apache FOP is used to transform MODS XML to produce PDF splash pages.<br/>!msg', array(
                      '!msg' => islandora_executable_available_message($fop))),
    '#default_value' => $fop,
    '#ajax' => array(
      'callback' => 'boston_college_admin_settings_form_ajax_callback',
      'wrapper' => 'boston-college-admin-wrapper',
      'effect' => 'fade',
      'event' => 'change',
    ),
  );
  $form['boston_college_splash_xsl'] = array(
    '#type' => 'textfield',
    '#title' => t('Splash Page FOP XSL'),
    '#description' => t('Path of the FOP XSL used to generate PDF splash pages.'),
    '#default_value' => variable_get('boston_college_splash_xsl', '/opt/BC-Islandora-Implementation/SplashPageStylesheets/splash.xsl'),
  );
  $form['boston_college_pdftk_executable'] = array(
    '#type' => 'textfield',
    '#title' => t('PDF Toolkit Executable'),
    '#description' => t('The PDF Toolkit is used to concatenate the PDF splash page with the archival PDF.<br/>!msg', array(
                      '!msg' => islandora_executable_available_message($pdftk))),
    '#default_value' => $pdftk,
    '#ajax' => array(
      'callback' => 'boston_college_admin_settings_form_ajax_callback',
      'wrapper' => 'boston-college-admin-wrapper',
      'effect' => 'fade',
      'event' => 'change',
    ),
  );

  return system_settings_form($form);
}

/**
 * Ajax callback for the executable textfields.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The element to render as part the ajax callback.
 */
function boston_college_admin_settings_form_ajax_callback(array $form, array $form_state) {
  return $form;
}

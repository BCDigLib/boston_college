<?php

/**
 * @file
 * Document ingest step callback and friends.
 */

/**
 * Form building function; build the basic tab form.
 */
function boston_college_document_upload_ingest_step_form($form, &$form_state) {
  module_load_include('inc', 'boston_college', 'includes/document.utilities');
  module_load_include('inc', 'islandora', 'ingest.form');

  $form['upload_pdf_checkbox'] = array(
    '#type' => 'checkbox',
    '#title' => t('Upload PDF document?'),
  );
  $step_storage = islandora_ingest_form_get_step_storage($form_state);
  boston_college_document_upload_form_base($form, isset($step_storage['values']) ? $step_storage['values'] : array());
  $form['upload_document'] = array(
    '#type' => 'fieldset',
    '#states' => array(
      'visible' => array(
        ':input[name="upload_pdf_checkbox"]' => array('checked' => TRUE),
      ),
    ),
  ) + $form['upload_document'];

  // XXX: Since '#states' and '#required' do not really place nice, let us
  // things to act on later.
  $step_storage =& islandora_ingest_form_get_step_storage($form_state);
  $step_storage['validate'] = array();
  foreach (element_children($form['upload_document']) as $element_key) {
    $element =& $form['upload_document'][$element_key];
    if (isset($element['#required']) && $element['#required']) {
      $step_storage['validate'][] = $element_key;
      $element['#required'] = FALSE;
      $element['#original_title'] = $element['#title'];
      $element = boston_college_appear_to_be_required($element);
    }
  }

  return $form;
}

/**
 * Make a field appear required.
 */
function boston_college_appear_to_be_required($element) {
  $element['#title'] .= <<<EOHTML
<span title="This field is required." class="form-required">*</span>
EOHTML;

  return $element;
}

/**
 * Form validation callback.
 *
 * Since '#required' and '#states' do not really play well together, let us
 * break it apart.
 */
function boston_college_document_upload_ingest_step_form_validate(&$form, &$form_state) {
  module_load_include('inc', 'boston_college', 'includes/document.utilities');
  module_load_include('inc', 'islandora', 'ingest.form');
  if ($form_state['values']['upload_pdf_checkbox']) {
    $step_storage = islandora_ingest_form_get_step_storage($form_state);
    foreach ($step_storage['validate'] as $validate) {
      if (empty($form_state['values']['upload_document'][$validate])) {
        $element = $form['upload_document'][$validate];
        form_error($element, t('%title field is required.', array(
          '%title' => $element['#original_title'],
        )));
      }
    }
  }
}

/**
 * Form submission callback; save the PDF to the object.
 */
function boston_college_document_upload_ingest_step_form_submit(&$form, &$form_state) {
  if ($form_state['values']['upload_pdf_checkbox']) {
    module_load_include('inc', 'boston_college', 'includes/document.utilities');
    $object = islandora_ingest_form_get_object($form_state);
    $values = $form_state['values']['upload_document'];
    boston_college_upload_document_apply($values, $object);
    $step_storage =& islandora_ingest_form_get_step_storage($form_state);
    $step_storage['modified'] = TRUE;
    $step_storage['values'] = array_filter($values);
  }
}

/**
 * Step "undo" callback; remove the PDF and the version info from the MODS.
 */
function boston_college_document_upload_ingest_step_undo_submit(&$form, &$form_state) {
  $step_storage =& islandora_ingest_form_get_step_storage($form_state);
  if (isset($step_storage['modified']) && $step_storage['modified']) {
    module_load_include('inc', 'boston_college', 'includes/document.utilities');
    $object = islandora_ingest_form_get_object($form_state);
    boston_college_upload_document_undo($object);
    unset($step_storage['modified']);
    unset($step_storage['values']);
  }
}

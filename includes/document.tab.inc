<?php
/**
 * @file
 * Functions pertaining to the PDF/Document upload tab.
 */

/**
 * Page callback to dump out the form.
 */
function boston_college_document_view(AbstractObject $object) {
  if (isset($object['MODS'])) {
    return drupal_get_form(isset($object[BOSTON_COLLEGE_ARCHIVAL_PDF_DATASTREAM_ID]) ? 'boston_college_document_delete_form' : 'boston_college_document_upload_form', $object->id);
  }
  return array(
    '#markup' => t('No MODS present on object; we need it to be able to modify it.'),
  );
}

/**
 * Form building function; build the basic tab form.
 */
function boston_college_document_upload_form($form, &$form_state, $object_id) {
  form_load_include($form_state, 'inc', 'boston_college', 'includes/document.utilities');

  boston_college_document_upload_form_base($form);
  $form['actions'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    ),
  );
  return $form;
}

/**
 * Form submission callback; save the PDF to the object.
 */
function boston_college_document_upload_form_submit(&$form, &$form_state) {
  list($object_id) = $form_state['build_info']['args'];
  $object = islandora_object_load($object_id);
  $values = $form_state['values']['upload_document'];
  boston_college_upload_document_apply($values, $object);
}

/**
 * Form building function; allow the viewing of and deletion of the document.
 */
function boston_college_document_delete_form($form, &$form_state, $object_id) {
  form_load_include($form_state, 'inc', 'boston_college', 'includes/document.utilities');

  $object = islandora_object_load($object_id);
  if (!$object) {
    drupal_set_message(t('Error: Failed to load object.'));
  }

  $form['version'] = array(
    '#type' => 'item',
    '#title' => t('Document Version'),
    '#markup' => $object ? boston_college_get_document_version($object) : t('Error: Failed to load object.'),
  );
  $form['actions'] = array(
    '#type' => 'actions',
    'delete' => array(
      '#type' => 'submit',
      '#value' => t('Delete PDF'),
    ),
  );

  return $form;
}

/**
 * Form submission callback; remove the PDF and the version info from the MODS.
 */
function boston_college_document_delete_form_submit(&$form, &$form_state) {
  list($object_id) = $form_state['build_info']['args'];
  $object = islandora_object_load($object_id);
  boston_college_upload_document_undo($object);
}

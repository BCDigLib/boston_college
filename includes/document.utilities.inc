<?php
/**
 * @file
 * Helper/shared functions used in the document/PDF upload process.
 */

/**
 * Build up the form base in the given form.
 *
 * Makes the form '#tree'd, and created the "upload_document" element.
 *
 * @param array $form
 *   A reference to the form into which we will add elements.
 * @param array $defaults
 *   An optional associative array of default values for the form elements we
 *   will add, possibly including:
 *   - file: A file ID.
 *   - version: A document version string.
 *   - agree: Boolean indicating agreement.
 */
function boston_college_document_upload_form_base(&$form, $defaults = array()) {
  $form['#tree'] = TRUE;
  $form['upload_document'] = array(
    '#type' => 'markup',
  );
  $upload =& $form['upload_document'];
  $upload['statement'] = array(
    '#type' => 'item',
    '#markup' => t('This submission form applies only to text-based faculty publications. Please upload the associated document in PDF format and indicate the version of the document by checking one of the boxes below. If you are interested in depositing other materials (such as videos or datasets), please contact !jane_link or !emily_link.', array(
      '!jane_link' => l(t('Jane Morris'), 'mailto:jane.morris@bc.edu'),
      '!emily_link' => l(t('Emily Toner'), 'mailto:emily.toner@bc.edu'),
    )),
  );
  $upload['file'] = array(
    '#type' => 'managed_file',
    '#title' => t('PDF File'),
    '#required' => TRUE,
    '#upload_validators' => array(
      'file_validate_extensions' => array('pdf'),
    ),
  );
  $options = array(
    'Pre-print' => t('Pre-print: The version of the document originally submitted to the publisher, prior to peer review (also known as "Author\'s Submitted Manuscript").'),
    'Post-print' => t('Post-print: The version of the document with changes from the peer-review process, i.e. the final manuscript but without the publisher\'s formatting (also known as "Author\'s Accepted Manuscript").'),
    'Version of record' => t('Published PDF: The final formatted version of the document.'),
    'Other' => t('Unpublished documents, working papers, etc.'),
  );
  $upload['version'] = array(
    '#type' => 'radios',
    '#title' => t('Document Version'),
    '#options' => $options,
    '#required' => TRUE,
  );
  $upload['agreement'] = array(
    '#type' => 'item',
    '#title' => t('License Agreement'),
    '#markup' => t('I grant to Boston College the right to include and preserve the submission above in the Boston College institutional repository. I will retain copyright ownership but I hereby grant to Boston College the non-exclusive, world-wide, royalty-free right to use, copy, distribute, and publicly perform and display my submission in any format or medium including, but not limited to, audio or video, for any educational, non-commercial purposes.<br/><br/>
These rights include, without limitation, the right to maintain one or more copies of the submission in multiple formats for security, back-up, and preservation purposes, and to allow a third party to hold one or more copies solely for such purposes.<br/><br/>
I represent and warrant that the submission is my original work, that I have the right to grant the permission in this agreement, and that, to the best of my knowledge, the submission will not infringe upon anyones intellectual property rights. I have obtained all necessary permissions to include in my submission any materials created or owned by third parties and any such third party material is clearly identified and acknowledged within the content of the submission.<br/><br/>
If the submission is based upon work that has been sponsored or supported by an organization or agency other than Boston College, I certify that I have fulfilled any right of review or other obligations required by any contract or agreement with such agency or organization.<br/><br/>
Boston College will clearly identify my/our name(s) as the author(s) or owner(s) of the submission, and will not edit the text of the submission.'),
  );
  $upload['agree'] = array(
    '#type' => 'checkbox',
    '#title' => t('I agree to all of the statements above.'),
    '#required' => TRUE,
  );
  $upload['note'] = array(
    '#type' => 'markup',
    '#markup' => t('<strong>Note</strong>:  If you have retained full rights to your work, we encourage you to use a Creative Commons license when submitting your work. To allow Boston College to display a Creative Commons license with your submission, please contact !email_link. Keep in mind that if you have signed a Copyright Transfer Agreement when publishing your work, you may not be able to do so.', array(
      '!email_link' => l(t('Jane Morris'), 'mailto:jane.morris@bc.edu'),
    )),
  );
  $statement = <<<EOSTMT
These materials are made available for use in research, teaching and private study, pursuant to U.S. Copyright Law. The user must assume full responsibility for any use of the materials, including but not limited to, infringement of copyright and publication rights of reproduced materials. Any materials used for academic research or otherwise should be fully credited with the source. The publisher or original authors may retain copyright to the materials.
EOSTMT;
  $upload['usage_statement'] = array(
    '#type' => 'value',
    '#value' => $statement,
  );

  foreach (array_intersect_key($defaults, $upload) as $field => $default) {
    $upload[$field]['#default_value'] = $default;
  }
}

/**
 * Update the given object when uploading a PDF and settings the version.
 *
 * @param array $values
 *   An associative array containing:
 *   - version: The version of the document (a string), to throw into the MODS.
 *   - file: A Drupal file ID we will load to get the PDF.
 *   - usage_statement: A string containing the usage statement to add into the
 *     MODS.
 * @param AbstractObject $object
 *   The object to update. Must not contain a "PDF" datastream, and must
 *   contain a "MODS" datastream.
 *
 * @throws Exception
 *   If there is a PDF, no MODS, or the MODS does not appear to be valid, a
 *   generic Exception (with only a message set) will be thrown.
 */
function boston_college_upload_document_apply(array $values, AbstractObject $object) {
  if (isset($object['PDF']) || !isset($object['MODS'])) {
    throw new Exception(t('Object being submitted must not have a PDF, and have MODS.'));
  }

  // Add the PDF.
  $pdf_file = file_load($values['file']);
  $pdf_datastream = $object->constructDatastream('PDF', 'M');
  $pdf_datastream->mimetype = 'application/pdf';
  $pdf_datastream->label = 'PDF Datastream';
  $pdf_datastream->setContentFromFile($pdf_file->uri);
  $object->ingestDatastream($pdf_datastream);

  // Update the MODS.
  $mods_datastream = $object['MODS'];
  $mods_doc = new DOMDocument();
  $mods_doc->loadXML($mods_datastream->content);
  $mods_xpath = new DOMXPath($mods_doc);
  $mods_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $mods_root = boston_college_get_mods_root($mods_xpath);
  boston_college_clear_versions($mods_xpath, $mods_root);
  $no_map = array(
    'Other',
  );
  if (!in_array($values['version'], $no_map)) {
    $document_version = $mods_doc->createElement('note');
    $document_version->setAttribute('type', 'version information');
    $document_version->nodeValue = $values['version'];
    $mods_root->appendChild($document_version);
  }
  $statement_list = boston_college_get_usage_statement_node_list($mods_xpath, $mods_root);
  if ($statement_list->length === 0) {
    $usage_statement = $mods_doc->createElement('accessCondtion');
    $usage_statement->setAttribute('type', 'useAndReproduction');
    $usage_statement->nodeValue = $values['usage_statement'];
    $mods_root->appendChild($usage_statement);
  }
  $mods_datastream->content = $mods_doc->saveXML();
}

/**
 * Delete PDF and "version" description.
 *
 * @param AbstractObject $object
 *   The object to affect.
 */
function boston_college_upload_document_undo(AbstractObject $object) {
  $object->purgeDatastream('PDF');

  // Update the MODS.
  $mods_datastream = $object['MODS'];
  $mods_doc = new DOMDocument();
  $mods_doc->loadXML($mods_datastream->content);
  $mods_xpath = new DOMXPath($mods_doc);
  $mods_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $mods_root = boston_college_get_mods_root($mods_xpath);
  boston_college_clear_versions($mods_xpath, $mods_root);
  $mods_datastream->content = $mods_doc->saveXML();
}


/**
 * Helper function; get the root "MODS" element.
 */
function boston_college_get_mods_root(DOMXPath $mods_xpath) {
  $mods_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $mods_root_list = $mods_xpath->query('/mods:mods | /mods:modsCollection/mods:mods[1]');
  if ($mods_root_list->length == 0) {
    throw new Exception('MODS being submitted is not valid!');
  }
  return $mods_root_list->item(0);
}

/**
 * Helper function; get a list of all the "version" elements.
 */
function boston_college_get_version_node_list(DOMXPath $mods_xpath, DOMElement $mods_root) {
  return $mods_xpath->query('mods:note[@type="version information"]', $mods_root);
}

/**
 * Helper function; get a list of all the usage statement elements.
 */
function boston_college_get_usage_statement_node_list(DOMXPath $mods_xpath, DOMElement $mods_root) {
  return $mods_xpath->query('mods:accessCondition[@type="useAndReproduction"]', $mods_root);
}

/**
 * Helper function; clear "version" stuff out of MODS.
 */
function boston_college_clear_versions(DOMXPath $mods_xpath, DOMElement $mods_root) {
  $document_version_list = boston_college_get_version_node_list($mods_xpath, $mods_root);
  if ($document_version_list->length > 0) {
    // Clear out any pre-existing values.
    $elements_to_remove = array();
    foreach ($document_version_list as $document_version_element) {
      $elements_to_remove[] = $document_version_element;
    }
    foreach ($elements_to_remove as $element_to_remove) {
      $mods_root->removeChild($element_to_remove);
    }
  }
}

/**
 * Get the version of the document.
 *
 * @param AbstractObject $object
 *   The object for which to get the document version.
 *
 * @return string
 *   A string indicating the version. In addition to values actually in the
 *   MODS, may also be:
 *   - "Unknown version": If there is no non-empty version present
 *   - "Unknown version, multiple present?: <comma-separated list of
 *     versions>": If there is muliple non-empty versions present.
 */
function boston_college_get_document_version(AbstractObject $object) {
  $mods_datastream = $object['MODS'];
  $mods_doc = new DOMDocument();
  $mods_doc->loadXML($mods_datastream->content);
  $mods_xpath = new DOMXPath($mods_doc);
  $mods_root = boston_college_get_mods_root($mods_xpath);
  $document_version_list = boston_college_get_version_node_list($mods_xpath, $mods_root);
  if ($document_version_list->length == 0) {
    return t('Unknown version');
  }

  $values = array();
  foreach ($document_version_list as $version_element) {
    $values[] = $version_element->textContent;
  }
  $values = array_filter(array_map('trim', $values));
  switch (count($values)) {
    case 0:
      return t('Unknown version');

    case 1:
      return reset($values);

    default:
      return t('Unknown version, multiple present?: @versions', array(
        '@versions' => implode(', ', $values),
      ));

  }
}

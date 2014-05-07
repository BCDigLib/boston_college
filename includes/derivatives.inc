<?php
/**
 * @file
 * Defines functions used when constructing derivatives.
 */

/**
 * Updates the label of an object from the DC title field.
 *
 * Force isn't checked here as this action will always occur depending on
 * configuration.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param bool $force
 *   Whether the derivative is being forcefully created or not.
 *
 * @return array
 *   An array describing the outcome of the label updation.
 *
 * @see hook_islandora_derivative()
 */
function boston_college_update_object_label_from_mods_derivative(AbstractObject $object, $force = FALSE) {
  // XXX: This is a pretty bad hack to get around a low level Tuque 409 problem.
  $pid = $object->id;
  $tuque = islandora_get_tuque_connection();
  $tuque->cache->delete($pid);
  $non_cached_object = islandora_object_load($pid);
  $mods_xml = new SimpleXMLElement($non_cached_object['MODS']->content);
  $title = $mods_xml->xpath('/mods:mods/mods:titleInfo/mods:title');
  $title = reset($title);
  if ($title && $non_cached_object->label != $title) {
    $non_cached_object->label = $title;
  }
  return array(
    'success' => TRUE,
    'messages' => array(
      array(
        'message' => t('Updated the label for @pid.'),
        'message_sub' => array(
          '@pid' => $pid,
        ),
        'type' => 'dsm',
      ),
    ),
  );
}

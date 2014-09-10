<?php
/**
 * @file
 * Migration batching code for Boston using the Batch interface.
 */

class BostonCollegeBatchPreprocessor extends IslandoraScanBatch {
  /**
   * Get the pattern to sniff files for.
   *
   * @see IslandoraScanBatch::getPattern()
   */
  protected static function getPattern() {
    return '/\.xml$/';
  }

  /**
   * Get the associated object class.
   *
   * @see IslandoraScanBatch:getObjectClass()
   */
  protected static function getObjectClass() {
    return 'BostonCollegeBatchObject';
  }
}

class BostonCollegeBatchObject extends IslandoraScanBatchObject {
  protected $sourceXML;
  protected $type;
  protected $pdf_uri;
  protected static $typeMappings;
  /**
   * Constructs an object for Boston College.
   *
   * @see IslandoraScanBatch::__construct
   */
  public function __construct(IslandoraTuque $connection, $base_name, $object_info, $preprocessor_parameters) {
    parent::__construct($connection, $base_name, $object_info, $preprocessor_parameters);
    if (!static::$typeMappings) {
      static::$typeMappings = array(
        'dc' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/DC_MODS3-4_XSLT1-0.xsl',
        ),
        'etd-ms' => array(
          'content_model' => 'ir:graduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/EDTMS_MODS3-4_XSLT1-0.xsl',
        ),
        'mods' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => 'MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
      );
    }
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $type = $this->determineType($xml);
    $this->type = $type;
    if ($type) {
      if (isset(static::$typeMappings[$type])) {
        $this->preprocessorParameters['content_models'] = array(static::$typeMappings[$type]['content_model']);
      }
    }
    else {
      drush_log(t('Unable to determine the type for @file.', array('@file' => $this->objectInfo['xml']->uri)));
    }
    $archival_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/stream_ref[file_extension/text() = "pdf"]/file_name');
    if ($archival_xpath) {
      $pdf_name = (string) reset($archival_xpath);
      $this->pdf_uri = $this->preprocessorParameters['target'] . '/' . $pdf_name;
    }
    else {
      drush_log(t('Unable to match a PDF for @file', array('@file' => $this->objectInfo['xml']->uri)));
    }
  }

  /**
   * Wakey, wakey.
   */
  public function __wakeup() {
    parent::__wakeup();
    if (!static::$typeMappings) {
      static::$typeMappings = array(
        'dc' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/DC_MODS3-4_XSLT1-0.xsl',
        ),
        'etd-ms' => array(
          'content_model' => 'ir:graduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/EDTMS_MODS3-4_XSLT1-0.xsl',
        ),
        'mods' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => 'MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
      );
    }
  }

  /**
   * Process the batch.
   */
  public function batchProcess() {
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $this->sourceXML = $xml;
    $this->getArchivalPDF();
    return parent::batchProcess();
  }

  /**
   * Determines the content model based upon the source XML from DigiTools.
   *
   * @param SimpleXMLElement $xml
   *   A SimpleXMLElement containing our loaded source xml.
   *
   * @return bool|string
   *   FALSE if the XPath fails, the content model string otherwise.
   */
  public function determineType($xml) {
    $type_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[name/text() = "descriptive"]/type');
    if ($type_xpath) {
      $type = (string) reset($type_xpath);
      return $type;
    }
    return FALSE;
  }

  /**
   * Go out and create the MODS.
   */
  protected function getMods() {
    if (!isset($this['MODS'])) {
      // Get the data to be transformed.
      $source_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[name/text() = "descriptive"]/value');
      if ($source_xpath) {
        // Take the source XML and transform it using the supplied XSLs.
        $source = (string) reset($source_xpath);
        $mods_xml = static::runXslTransform(array(
          'input' => $source,
          'xsl' => static::$typeMappings[$this->type]['transform'],
        ));

        $mods_doc = new DOMDocument();
        $mods_doc->loadXML($mods_xml);

        $mods_to_add = array();

        // Go get the Handle value from PREMIS if it exists and append.
        $premis_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[type/text() = "preservation_md"]/value');
        if ($premis_xpath) {
          $premis = (string) reset($premis_xpath);
          $premis_xml = simplexml_load_string($premis);
          $premis_xml->registerXPathNamespace('premis', 'http://www.loc.gov/standards/premis');
          $handle_result = $premis_xml->xpath('/premis:object/premis:objectIdentifier[premis:objectIdentifierType/text() = "handle"]/premis:objectIdentifierValue/text()');
          if ($handle_result) {
            $handle = (string) reset($handle_result);
            $mods_to_add[] = array(
              'field' => 'mods:identifier',
              'attribute' => array(
                'name' => 'type',
                'value' => 'hdl',
              ),
              'value' => "http://hdl.handle.net/$handle",
            );
          }
        }
        // Finally, add the digiTool PID as an identifier.
        $digi_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/pid/text()');
        if ($digi_xpath) {
          $digi_pid = (string) reset($digi_xpath);
          $mods_to_add[] = array(
            'field' => 'mods:identifier',
            'attribute' => array(
              'name' => 'type',
              'value' => 'digitool',
            ),
            'value' => $digi_pid,
          );
        }

        foreach ($mods_to_add as $values) {
          $element = $mods_doc->createElement($values['field']);
          if (isset($values['attribute'])) {
            $attribute = $mods_doc->createAttribute($values['attribute']['name']);
            $attribute->value = $values['attribute']['value'];
            $element->appendChild($attribute);
          }
          $text = $mods_doc->createTextNode($values['value']);
          $element->appendChild($text);
          $mods_doc->documentElement->appendChild($element);
        }
        $mods_xml = $mods_doc->saveXML();
        $mods_datastream = $this->constructDatastream('MODS', 'M');
        $mods_datastream->mimetype = 'text/xml';
        $mods_datastream->label = 'MODS Record';
        $mods_datastream->content = $mods_xml;
        $this->ingestDatastream($mods_datastream);
        return $this['MODS']->content;
      }
      else {
        drush_log(t('Unable to retrieve source metadata for @file.', array('@file' => $this->objectInfo['xml']->uri)));
      }
    }
    else {
      return $this['MODS']->content;
    }
  }

  /**
   * Gets the archival PDF to be added.
   */
  protected function getArchivalPDF() {
    if ($this->pdf_uri) {
      $pdf_datastream = $this->constructDatastream(BOSTON_COLLEGE_ARCHIVAL_PDF_DATASTREAM_ID, 'M');
      $pdf_datastream->mimetype = 'application/pdf';
      $pdf_datastream->label = 'PDF Datastream';
      $pdf_datastream->setContentFromFile($this->pdf_uri);
      $this->ingestDatastream($pdf_datastream);
    }
  }
  /**
   * Add relationships to the object.
   *
   * @see IslandoraScanBatch::addRelationships()
   */
  public function addRelationships() {
    parent::addRelationships();
    // Grab the digiTools creation date to slam into the RELS-EXT.
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $creation_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/creation_date/text()');
    if ($creation_xpath) {
      $date = (string) reset($creation_xpath);
      $formatted_date = gmdate("Y-m-d\TH:i:s\Z", strtotime($date));
      $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'digiToolCreationDate', $formatted_date, RELS_TYPE_DATETIME);
    }
    else {
      drush_log(t('Unable to add creation date to the RELS-EXT for @file', array('@file' => $this->objectInfo['xml']->uri)));
    }
  }

  public function getResources() {
    return array();
  }

}